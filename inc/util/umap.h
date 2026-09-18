#ifndef UMAP_H_
#define UMAP_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct umap_node_s umap_node_t;
typedef struct umap_s umap_t;

typedef void (* umap_destroy_cb)(void * data);

struct umap_node_s
{
    uint8_t * suffix;     /* compressed trailing bytes/nibbles */
    uint8_t suffix_len;   /* number of nibbles in suffix */
    uint8_t sz;           /* active child count */
    uint8_t key;          /* single child index (0..15) or 16 when full */
    void * data;          /* value */
    umap_node_t * nodes;  /* array of 1 or 16 child nodes */
};

struct umap_s
{
    size_t n;
    umap_node_t root;
};
typedef int (*umap_cb)(const uint8_t uuid[16], void * data, void * arg);

umap_t * umap_create(void);
void umap_destroy(umap_t * map, umap_destroy_cb cb);
void umap_clear(umap_t * map, umap_destroy_cb cb);

void * umap_set(umap_t * map, const uint8_t uuid[16], void * data);
void * umap_get(umap_t * map, const uint8_t uuid[16]);
void * umap_pop(umap_t * map, const uint8_t uuid[16]);

int umap_walk(imap_t * imap, umap_cb cb, void * arg);

#endif /* UMAP_H_ */