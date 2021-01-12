import time


THINGSDB_EXT_NAME = 'PY_REQUEST'


def main(data):
    time.sleep(2)
    print('\n!!!!!\nHERE..1\n!!!!!!\n', data)
    time.sleep(2)
    print('\n!!!!!\nHERE..2\n!!!!!!\n', data)
    time.sleep(2)
    print('\n!!!!!\nHERE..3\n!!!!!!\n', data)
    return data * 7


if __name__ == '__main__':
    main(123)