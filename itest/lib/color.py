
NORMAL = '\x1B[0m'
RED = '\x1B[31m'
GREEN = '\x1B[32m'
YELLOW = '\x1B[33m'
LYELLOW = '\x1b[93m'

BGRED = "\x1b[41m"
BGGREEN = "\x1b[42m"
BGYELLOW = "\x1b[43m"
BGBLUE = "\x1b[44m"
BGMAGENTA = "\x1b[45m"
BGCYAN = "\x1b[46m"
BGWHITE = "\x1b[47m"


class Color:

    @staticmethod
    def success(text):
        return f'{GREEN}{text}{NORMAL}'

    @staticmethod
    def warning(text):
        return f'{YELLOW}{text}{NORMAL}'

    @staticmethod
    def error(text):
        return f'{RED}{text}{NORMAL}'

    @staticmethod
    def info(text):
        return f'{LYELLOW}{text}{NORMAL}'

    @staticmethod
    def node(n, text):
        color = [BGRED, BGMAGENTA, BGBLUE, BGCYAN, BGGREEN, BGYELLOW][n]
        return f'{color}NODE {n}{NORMAL}: {text}'
