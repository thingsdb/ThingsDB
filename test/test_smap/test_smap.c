/*
 * test_smap.c
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */


#include "../test.h"
#include <util/smap.h>

const unsigned int num_entries = 14;
char * entries[] = {
    "Zero",
    "First entry",
    "Second entry",
    "Third entry",
    "Fourth entry",
    "Fifth entry",
    "Sixth entry",
    "Seventh entry",
    "8",
    "9",
    "entry 10",
    "entry 11",
    "entry 12",
    "entry-last"
};

int test_item_cb(
        const char * key,
        size_t n,
        void * data,
        void * arg)
{
    /* test if n is at least > 0 */
    assert (n > 0);

    /* test if n is correct */
    assert (strlen(data) == n);

    /* test if key is correct */
    assert (strncmp(key, data, n) == 0);

    /* increment counter */
    unsigned int * i = (unsigned int *) arg;
    (*i)++;

    return 0;
}

int test_item_stop_cb(
        const char * key __attribute__((unused)),
        size_t n __attribute__((unused)),
        void * data,
        void * arg)
{
    int * i = (int *) arg;
    return (data == entries[*i]) ? *i : 0;
}

int test_val_cb(
        void * data,
        void * arg)
{
    unsigned int n;
    for (n = 0; n < num_entries; n++)
    {
        if (data == entries[n]) break;
    }

    /* increment counter */
    unsigned int * i = (unsigned int *) arg;
    (*i) += n;

    return 0;
}

int test_val_stop_cb(
        void * data,
        void * arg)
{
    int * i = (int *) arg;
    return (data == entries[*i]) ? *i : 0;
}

int main()
{
    test_start("smap");

    smap_t * smap = smap_create();

    /* test adding values */
    {
        for (unsigned int i = 0; i < num_entries; i++)
        {
            assert (smap_add(smap, entries[i], entries[i]) == 0);
        }
    }

    /* test is the length is correct */
    {
        assert (smap->n == num_entries);
    }

    /* test if a duplicate key cannot be inserted */
    {
        assert (smap_add(smap, entries[2], "Duplicate") == SMAP_ERR_EXIST);
        assert (smap_get(smap, entries[2]) == entries[2]);
    }

    /* test if empty keys return correct error code */
    {
        assert (smap_add(smap, "", "empty key") == SMAP_ERR_EMPTY_KEY);
    }

    /* test longest key length */
    {
        assert (smap_longest_key_size(smap) == strlen(entries[7]));
    }

    /* test gets */
    {
        for (unsigned int i = 0; i < num_entries; i++)
        {
            assert (smap_get(smap, entries[i]) == entries[i]);
        }
    }

    /* test get non existing keys */
    {
        assert (smap_get(smap, "entry does not exist") == NULL);
        assert (smap_get(smap, "") == NULL);
    }

    /* test get addresses */
    for (unsigned int i = 0; i < num_entries; i++)
    {
        assert (*smap_getaddr(smap, entries[i]) == entries[i]);
    }

    /* test get addresses for non existing keys */
    {
        assert (smap_getaddr(smap, "entry does not exist") == NULL);
        assert (smap_getaddr(smap, "") == NULL);
    }

    /* test looping over all items */
    {
        char buf[smap_longest_key_size(smap)];
        unsigned int arg = 0;
        assert (smap_items(smap, buf, test_item_cb, &arg) == 0);
        assert (arg == num_entries);
    }

    /* test if looping items stops at non zero return code */
    {
        char buf[smap_longest_key_size(smap)];
        int stop = 4;
        assert (smap_items(smap, buf, test_item_stop_cb, &stop) == stop);
    }

    /* test looping over all values */
    {
        unsigned int arg = 0;
        unsigned int expect_sum = 0;
        for (unsigned int i = 0; i < num_entries; i++) expect_sum += i;
        assert (smap_values(smap, test_val_cb, &arg) == 0);
        assert (arg == expect_sum);
    }

    /* test looping values stops at non zero return code */
    {
        int stop = 0;
        assert (smap_values(smap, test_val_stop_cb, &stop) == stop);
    }

    /* test popping an non existing value */
    {
        assert (smap_pop(smap, "non existing") == NULL);
        assert (smap_pop(smap, "") == NULL);
    }

    /* test popping values */
    {
        for (unsigned int i = 0; i < num_entries; i++)
        {
            assert (smap_pop(smap, entries[i]) == entries[i]);
        }
    }

    /* test is the length is correct and nodes are cleaned */
    {
        assert (smap->n == 0);
        assert (smap->nodes == NULL);
    }

    smap_destroy(smap, NULL);

    test_end(0);
    return 0;
}
