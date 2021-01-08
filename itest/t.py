import threading
import time


def main(set_result, *args):
    print('args:', args)
    time.sleep(1)
    raise Exception('stop')
    # set_result(42)
    # set_result(43)


def _ti_set_result(pid, res):
    print('pid:', pid, 'result:', res)


class _TiCall:
    def __init__(self, pid):
        self.pid = pid

    def set_result(self, result):
        if self.pid is None:
            return
        pid = self.pid
        self.pid = None
        _ti_set_result(pid, result)


def _ti_work(pid, func, *args):
    ti_call = _TiCall(pid)
    try:
        func(ti_call.set_result, *args)
    except Exception as e:
        ti_call.set_result(e)
    finally:
        ti_call.set_result(None)


def _ti_call_ext(pid, func, *args):
    t = threading.Thread(target=_ti_work, args=(pid, func) + tuple(args))
    t.start()


_ti_call_ext(123, main)
_ti_call_ext(234, main)
