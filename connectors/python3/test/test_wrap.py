import sys
import unittest
sys.path.insert(0, '../')
from thingsdb.wrap import wrap  # nopep8


def fmt_wrap(wrapped):
    return f'x={wrapped!r}'


def val_wrap(val, blobs=None):
    if blobs is None:
        blobs = []
    return fmt_wrap(wrap(val, blobs))


class TestWrap(unittest.TestCase):

    def test_wrap_int(self):
        self.assertEqual('x=10', val_wrap(10))
        self.assertEqual('x=-10', val_wrap(-10))

    def test_wrap_float(self):
        self.assertEqual('x=0.5', val_wrap(0.5))
        self.assertEqual('x=-0.5', val_wrap(-0.5))

    def test_wrap_bool(self):
        self.assertEqual('x=true', val_wrap(True))
        self.assertEqual('x=false', val_wrap(False))

    def test_wrap_nil(self):
        self.assertEqual('x=nil', val_wrap(None))

    def test_wrap_string(self):
        self.assertEqual("x='Iris'", val_wrap('Iris'))
        self.assertEqual("x=''", val_wrap(''))

    def test_wrap_list(self):
        self.assertEqual("x=[6,'Iris']", val_wrap([6, 'Iris']))
        self.assertEqual("x=[[6,'Iris']]", val_wrap([[6, 'Iris']]))

    def test_wrap_dict(self):
        self.assertEqual(r"x={}", val_wrap({}))
        self.assertEqual(r"x={name:'Iris'}", val_wrap({'name': 'Iris'}))
        self.assertEqual(r"x={age:6}", val_wrap({'age': 6}))

    def test_wrap_nested(self):
        self.assertEqual(
            r"x=[{stuff:[{lang:'C'},{more:['Python','C',true]}]}]",
            val_wrap([{
                'stuff': [{
                    'lang': 'C',
                }, {
                    'more': ['Python', 'C', True]
                }]
            }])
        )

    def test_blobs(self):
        blobs = []
        self.assertEqual(r"x=[blob(0),blob(1)]", val_wrap([b'a', b'b'], blobs))
        self.assertEqual(blobs, [b'a', b'b'])


if __name__ == '__main__':
    unittest.main()
