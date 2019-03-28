import pexpect


if __name__ == '__main__':
    child = pexpect.spawn('./thingsdb -c ../itest/testdir/ti0.conf')
    child.expect('running')
