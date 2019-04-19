import subprocess
from .vars import THINGSDB_TERMINAL
from .vars import THINGSDB_TERM_GEOMETRY
from .vars import THINGSDB_TERM_KEEP
from .vars import THINGSDB_MEMCHECK
from .vars import THINGSDB_BIN

class Node:
    def __init__(self):
        pass

    def init(self):
        if THINGSDB_TERMINAL == 'xterm':
            self.proc = subprocess.Popen(
               f'xterm {"-hold " if THINGSDB_TERM_KEEP else ""}'
               f'-title {self.name} '
               f'-geometry {THINGSDB_TERM_GEOMETRY} '
               f'-e "{THINGSDB_MEMCHECK}{THINGSDB_BIN} '
               f'--config {self.cfgfile}"',
               shell=True
            )
        elif self.TERMINAL is None:
            errfn = f'testdir/{self.test_title}-{self.name}-err.log'
            outfn = f'testdir/{self.test_title}-{self.name}-out.log'
            with open(errfn, 'a') as err:
                with open(outfn, 'a') as out:
                    self.proc = subprocess.Popen(
                        f'{THINGSDB_MEMCHECK}{THINGSDB_BIN} '
                        f'--config {self.cfgfile}',
                        stderr=err,
                        stdout=out,
                        shell=True
                    )

        await asyncio.sleep(5)