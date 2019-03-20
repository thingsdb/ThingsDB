import sys
sys.path.insert(0, '../')
from thingsdb.wrap import wrap  # nopep8


if __name__ == '__main__':
    blobs = []
    wrapped = wrap([
        {
            'user': 'Iris',
            'age': 6,
            'likes': [
                {
                    'candy': 'a lot',
                    'listening': 'less'
                }
            ],
            'friends': ['Cato', 'Lena']

        }
    ], blobs)

    print(f'x={wrapped!r}')
