
class Scope:
    def __init__(self, n):
        self.n = n

    def __str__(self):
        return 'Scope{}'.format(self.n)


class OprMul:
    order = 0

    def __init__(self, a, b):
        self.a = a
        self.b = b

    def __str__(self):
        return '({} * {})'.format(self.a, self.b)


class OprAnd:
    order = 1

    def __init__(self, a, b):
        self.a = a
        self.b = b

    def __str__(self):
        return '({} && {})'.format(self.a, self.b)


class OprOr:
    order = 2

    def __init__(self, a, b):
        self.a = a
        self.b = b

    def __str__(self):
        return '({} || {})'.format(self.a, self.b)



def sortopr(x, parent):
    if isinstance(x, Scope):
        return False

    sortopr(x.a, x)
    if sortopr(x.b, x):
        parent.b = x.b
        tmp = x.b.a
        x.b.a = x
        x.b = tmp
        x = parent.b

    if parent and x.order > parent.order:
        return True
    return False


if __name__ == '__main__':

    tree = OprOr(
        Scope(0),
        OprAnd(
            Scope(1),
            OprAnd(
                Scope(2),
                OprOr(
                    Scope(3),
                    Scope(4)
                )
            )
        )
    )

    print(tree)
    sortopr(tree, None)

    expect = OprOr(
        Scope(0),
        OprOr(
            OprAnd(
                Scope(1),
                OprAnd(
                    Scope(2),
                    Scope(3)
                ),
            ),
            Scope(4)
        )
    )
    print(tree)
    print(expect)

    import re
    r = re.compile(r'[-+]?[0-9]*\.?[0-9]+')
    print (r.match('1.2'))

