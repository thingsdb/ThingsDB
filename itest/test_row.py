#!/usr/bin/env python
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.client import Client
from thingsdb.room import Room
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import MaxQuotaError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import OperationError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import SyntaxError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import ValueError


class TestRow(TestBase):

    title = 'Test row with columns'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
    async def async_run(self):

        self.node0.version()

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node otherwise backups are not possible
        if hasattr(self, 'node1'):
            await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_row(self, client: Client):
        await client.query("""//ti
            set_type('Book', {
                id: '#',
                title: 'str',
                pages: 'int',
            });
            set_type('Collection', {
                i: '#', // rename -> id
                b: '[Book]', // rename -> books, mod -> &[Book]
                t: |this| this.b.sum(|b| b.pages), // total_pages & fix
            });
            .collection = Collection{
                b: [
                    Book{title: 'a', pages: 42},
                    Book{title: 'b', pages: 13},
                    Book{title: 'c', pages: 99},
                ]
            };
        """)
        with self.assertRaisesRegex(
                NumArgumentsError,
                r"function `mod_type` requires at least 3 arguments "
                r"but 2 were given; "):
            await client.query("""//ti
                mod_type('Book', 'row');
            """)
        with self.assertRaisesRegex(
                NumArgumentsError,
                r"function `mod_type` with task `row` takes 3 arguments "
                r"but 4 were given"):
            await client.query("""//ti
                mod_type('Book', 'row', nil, nil);
            """)
        with self.assertRaisesRegex(
                TypeError,
                r"function `mod_type` with task `row` expects argument 3 to "
                r"be of type `list`, `tuple` or `nil` but got type `int` "
                r"instead;"):
            await client.query("""//ti
                mod_type('Book', 'row', 123);
            """)
        with self.assertRaisesRegex(
                ValueError,
                r"at least one column is required for a row definition; "
                "use `nil` to remove the row;"):
            await client.query("""//ti
                mod_type('Book', 'row', []);
            """)
        with self.assertRaisesRegex(
                TypeError,
                r"each column in a row must be specified as type `str`, "
                r"found type `nil`;"):
            await client.query("""//ti
                mod_type('Book', 'row', [nil]);
            """)
        with self.assertRaisesRegex(
                ValueError,
                r"each column in a row must follow the naming rules;"):
            await client.query("""//ti
                mod_type('Book', 'row', ["this is a test"]);
            """)
        with self.assertRaisesRegex(
                LookupError,
                r"type `Book` has no property `vvv`; "
                r"each column in a row must refer to an existing "
                r"field or method;"):
            await client.query("""//ti
                mod_type('Book', 'row', ["id", "vvv"]);
            """)

        c_id, (alice_id, bob_id, charlie_id) = await client.query("""//ti
            [.collection.id(), .collection.b.map_id()];
        """)
        await client.query("""//ti
            mod_type('Book', 'row', ["id", "title", "pages"]);
            mod_type('Collection', 'row', ["i", "b", "t"]);
        """)
        res = await client.query(r"""//ti
            .collection.wrap();
        """)
        self.assertEqual(res, [
            c_id,
            [{'id': alice_id}, {'id': bob_id}, {'id': charlie_id}],
            154,
        ])

        with self.assertRaisesRegex(
                OperationError,
                r"cannot delete a field or method with a corresponding "
                r"column in row; you might want to create a new row without "
                r"this property: `mod_type\(\"Book\", \"row\", \[\.\.\.\]\);"):
            await client.query("""//ti
                mod_type('Book', 'del', 'title');
            """)
        with self.assertRaisesRegex(
                OperationError,
                r"cannot delete a field or method with a corresponding "
                r"column in row; you might want to create a new row without "
                r"this property: `mod_type\(\"Book\", \"row\", \[\.\.\.\]\);"):
            await client.query("""//ti
                mod_type('Book', 'del', 'id');
            """)
        with self.assertRaisesRegex(
                OperationError,
                r"cannot delete a field or method with a corresponding "
                r"column in row; you might want to create a new row without "
                r"this property: "
                r"`mod_type\(\"Collection\", \"row\", \[\.\.\.\]\);"):
            await client.query("""//ti
                mod_type('Collection', 'del', 't');
            """)
        await client.query("""//ti
            mod_type('Collection', 'ren', "i", "id");
            mod_type('Collection', 'ren', "b", "books");
            mod_type('Collection', 'ren', "t", "total_pages");
        """)

        res = await client.query(r"""//ti
            .collection.wrap();
        """)
        self.assertEqual(res, [
            c_id,
            [{'id': alice_id}, {'id': bob_id}, {'id': charlie_id}],
            'type `Collection` has no property or method `b`',
        ])
        await client.query("""//ti
            mod_type('Collection', 'mod', "books", "&[Book]");
            mod_type('Collection', 'mod', "total_pages", |this| {
                this.books.sum(|b| b.pages);
            });
            mod_type('Book', 'mod', "title", "str<2:>", |book| {
                {
                    a: "Alice",
                    b: "Bob",
                    c: 'Charlie'
                }[book.title];
            });
        """)
        res = await client.query(r"""//ti
            .collection.wrap();
        """)
        self.assertEqual(res, [
            c_id,
            [
                [alice_id, "Alice", 42],
                [bob_id, "Bob", 13],
                [charlie_id, "Charlie", 99],
            ],
            154,
        ])

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        for cl in (client, client1):
            res = await cl.query(r"""//ti
                type_info('Collection').load().row;
            """)
            self.assertEqual(res, ['id', 'books', 'total_pages'])

        for _ in range(10):
            await client.query("""//ti
                Book(id).pages += 1;
            """, id=alice_id)

        await self.node0.shutdown()
        await self.node0.run()
        await self.wait_nodes_ready()

        res = await client.query(r"""//ti
            .collection.books.map_wrap();
        """)
        self.assertEqual(res, [
            [alice_id, "Alice", 52],
            [bob_id, "Bob", 13],
            [charlie_id, "Charlie", 99],
        ])

        res = await client.query("""//ti
        {
            id: 123,  // not a real id
            title: "My Book",
            pages: 9.5,   // float, half a page
        }.wrap('Book');
        """)
        self.assertEqual(res, [None, "My Book", None])

        res = await client.query("""//ti
        .collection.wrap('Book');
        """)
        self.assertEqual(res, [c_id, None, None])

        res = await client.query("""//ti
            Book(id).wrap('Collection');
        """, id=alice_id)
        self.assertEqual(res, [
            alice_id,
            None,
            'type `Book` has no property or method `books`'
        ])
        res = await client.query("""//ti
            export();
        """)
        self.maxDiff = 1000
        self.assertEqual(res, """
try(commit('Source: collection `stuff`'));

new_type('Book');
new_type('Collection');


set_type('Book', {
  id: '#',
  title: 'str<2:>',
  pages: 'int',
});
set_type('Collection', {
  id: '#',
  books: '&[Book]',
  total_pages: |this| {
    this.books.sum(|b| b.pages);
  },
});


mod_type('Book', 'row', [
  'id',
  'title',
  'pages',
]);
mod_type('Collection', 'row', [
  'id',
  'books',
  'total_pages',
]);




'DONE';
""".lstrip().replace('  ', '\t'))


if __name__ == '__main__':
    run_test(TestRow())
