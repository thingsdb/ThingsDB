import sys
import fcntl
import os
import selectors
import signal
import asyncio
import logging
import msgpack
from .pkg import Pkg
from .ex import TiException
from .proto import (
    PROTO_MODULE_CONF,
    PROTO_MODULE_CONF_OK,
    PROTO_MODULE_CONF_ERR,
    PROTO_MODULE_REQ,
    PROTO_MODULE_RES,
    PROTO_MODULE_ERR,
)



class _Stop(Exception):
    pass


async def _connect_stdin_stdout(loop):

    reader = asyncio.StreamReader()
    protocol = asyncio.StreamReaderProtocol(reader)
    await loop.connect_read_pipe(lambda: protocol, sys.stdin)
    w_transport, w_protocol = await loop.connect_write_pipe(
        asyncio.streams.FlowControlMixin,
        sys.stdout)
    writer = asyncio.StreamWriter(w_transport, w_protocol, reader, loop)
    return reader, writer

def _stop(reader):
    reader.set_exception(_Stop())

def _write_data(writer, pid, tp, data):
    data = msgpack.packb(data, use_bin_type=True)
    header = Pkg.st_package.pack(
            len(data),
            pid,
            tp,
            tp ^ 0xff)
    writer.write(header + data)

def _write_conf_err(writer, pid, err_code, err_msg):
    _write_data(writer, pid, PROTO_MODULE_CONF_ERR, (err_code, err_msg))

def _write_conf_ok(writer, pid):
    _write_data(writer, pid, PROTO_MODULE_CONF_OK, None)

def _write_err(writer, pid, err_code, err_msg):
    _write_data(writer, pid, PROTO_MODULE_ERR, (err_code, err_msg))

def _write_resp(writer, pid, resp):
    _write_data(writer, pid, PROTO_MODULE_RES, resp)

async def _on_request(pkg, handler, writer):
    try:
        try:
            resp = await handler.on_request(pkg.data)
        except TiException as e:
            _write_err(writer, pkg.pid, e.err_code, str(e))
        except Exception as e:
            _write_err(
                writer,
                pkg.pid,
                Ex.Operation,
                f'unexpected module failure: {e}')
        else:
            _write_resp(writer, pkg.pid, resp)
    except Exception as e:
        handler.on_error(e)

async def _on_config(pkg, handler, writer):
    try:
        try:
            await handler.on_config(pkg.data)
        except TiException as e:
            _write_conf_err(writer, pkg.pid, e.err_code, str(e))
        except Exception as e:
            _write_conf_err(
                writer,
                pkg.pid,
                Ex.Operation,
                f'unexpected module failure: {e}')
        else:
            _write_conf_ok(writer, pkg.pid)
    except Exception as e:
        handler.on_error(e)

_PROTO_MAP = {
    PROTO_MODULE_CONF: _on_config,
    PROTO_MODULE_REQ: _on_request,
}

async def _module_loop(loop, handler):
    reader, writer = await _connect_stdin_stdout(loop)

    for signame in ('SIGHUP', 'SIGINT', 'SIGTERM', 'SIGQUIT'):
        loop.add_signal_handler(getattr(signal, signame), _stop, reader)

    pkg = None
    buf = bytearray()

    while True:
        try:
            data = await reader.read(1)
        except _Stop:
            break

        buf.extend(data)
        size = len(buf)
        while size >= Pkg.st_package.size:
            if pkg is None:
                try:
                    pkg = Pkg(buf)
                except Exception as e:
                    await handler.on_error(e)
                    return

            if size < pkg.total:
                break

            try:
                pkg.extract_data_from(buf)
            except KeyError as e:
                await handler.on_error(
                    KeyError(f'unsupported package received: {e}'))
                return
            except Exception as e:
                await handler.on_error(e)
                return

            try:
                callback = _PROTO_MAP[pkg.tp]
            except Exception as e:
                await handler.on_error(f'unexpected package type: {pkg.tp}')
                return

            asyncio.ensure_future(callback(pkg, handler, writer))

            pkg = None
            buf.clear()
            size = 0


def start_module(name, handler):
    loop = asyncio.get_event_loop()
    loop.run_until_complete(_module_loop(loop, handler))

