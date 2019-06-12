class ArrayOf(list):
    pass


def array_of(type_):
    return type('ArrayOf', (ArrayOf, ), {'type_': type_})
