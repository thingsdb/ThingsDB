/*
 * smap.h
 */
#ifndef SMAP_H_
#define SMAP_H_

#include <stdint.h>
#include <stddef.h>

enum
{
    SMAP_SUCCESS        =0,
    SMAP_ERR_ALLOC      =-1,
    SMAP_ERR_EXIST      =-2,
    SMAP_ERR_EMPTY_KEY  =-3,
};

typedef struct smap_s smap_t;
typedef struct smap_node_s smap_node_t;
typedef struct smap_node_s * smap_nodes_t[32];

typedef int (*smap_item_cb)(
        const char * key,
        size_t n,
        void * data,
        void * arg);
typedef int (*smap_val_cb)(void * data, void * arg);
typedef void (*smap_destroy_cb)(void * data);

smap_t * smap_create(void);
void smap_destroy(smap_t * smap, smap_destroy_cb cb);
int smap_add(smap_t * smap, const char * key, void * data);
void * smap_get(smap_t * node, const char * key);
void ** smap_getaddr(smap_t * smap, const char * key);
void * smap_getn(smap_t * smap, const char * key, size_t n);
void * smap_pop(smap_t * smap, const char * key);
size_t smap_longest_key_size(smap_t * smap);
int smap_items(smap_t * smap, char * buf, smap_item_cb cb, void * arg);
int smap_values(smap_t * smap, smap_val_cb cb, void * arg);

struct smap_s
{
    uint8_t offset;         /* start of the nodes */
    uint8_t sz;             /* size of this node, starting at offset  */
    uint16_t pad0;
    uint32_t n;             /* total number of items */
    smap_nodes_t * nodes;
};

struct smap_node_s
{
    uint8_t offset;         /* start of the nodes */
    uint8_t sz;             /* size of this node, starting at offset  */
    uint8_t size;           /* number of child nodes */
    uint8_t pad0;
    uint32_t n;             /* total number of items, children included */
    smap_nodes_t * nodes;
    char * key;
    void * data;
};

#endif /* SMAP_H_ */
