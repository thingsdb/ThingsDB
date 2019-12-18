import sys
import unittest
sys.path.insert(0, '../')
from thingsdb.util import fmt  # nopep8
from thingsdb.model import Thing, Collection  # nopep8


def fmt_fmt(formatted):
    return f'x={formatted}'


def val_fmt(val, blobs=None):
    if blobs is None:
        blobs = []
    return fmt_fmt(fmt(val, blobs))


class TestWrap(unittest.TestCase):

    def test_fmt_int(self):
        self.assertEqual('x=10', val_fmt(10))
        self.assertEqual('x=-10', val_fmt(-10))

    def test_fmt_float(self):
        self.assertEqual('x=0.5', val_fmt(0.5))
        self.assertEqual('x=-0.5', val_fmt(-0.5))

    def test_fmt_bool(self):
        self.assertEqual('x=true', val_fmt(True))
        self.assertEqual('x=false', val_fmt(False))

    def test_fmt_nil(self):
        self.assertEqual('x=nil', val_fmt(None))

    def test_fmt_string(self):
        self.assertEqual("x='Iris'", val_fmt('Iris'))
        self.assertEqual("x=''", val_fmt(''))

    def test_fmt_list(self):
        self.assertEqual("x=[6,'Iris']", val_fmt([6, 'Iris']))
        self.assertEqual("x=[[6,'Iris']]", val_fmt([[6, 'Iris']]))

    def test_fmt_dict(self):
        self.assertEqual(r"x={}", val_fmt({}))
        self.assertEqual(r"x={name:'Iris'}", val_fmt({'name': 'Iris'}))
        self.assertEqual(r"x={age:6}", val_fmt({'age': 6}))

    def test_fmt_nested(self):
        self.assertEqual(
            r"x=[{stuff:[{lang:'C'},{more:['Python','C',true]}]}]",
            val_fmt([{
                'stuff': [{
                    'lang': 'C',
                }, {
                    'more': ['Python', 'C', True]
                }]
            }])
        )

    def test_blobs(self):
        blobs = {}
        self.assertEqual(r"x=[blob0,blob1]", val_fmt([b'a', b'b'], blobs))
        self.assertEqual(blobs, {'blob0': b'a', 'blob1': b'b'})

    def test_fmt_thing(self):
        t = Thing(Collection(), 42)
        self.assertEqual("x=#42", val_fmt(t))
        self.assertEqual("x=#42", val_fmt({'#': 42, "test": "xyz"}))


if __name__ == '__main__':
    unittest.main()
