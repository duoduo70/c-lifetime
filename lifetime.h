#ifndef _LIFETIME_H_
#define _LIFETIME_H_

#include <stdio.h>
#include <stdlib.h>

#define lftm(__size, __block)                                                  \
        do {                                                                   \
                __lftm_start(__size);                                          \
                do                                                             \
                        __block while (0);                                     \
                __lftm_end();                                                  \
        } while (0);

#define lftm_export(__ptr) __lftmmap_export(lftm_stack_last(), __ptr)

enum PointerMapType {
        LFTMMAP_TYPE_HASHMAP,
        LFTMMAP_TYPE_LINKLIST,
        LFTMMAP_TYPE_VLA
};

#define LFTM_HASHMAP 32
#define LFTM_LINKLIST 8
#define LFTM_VLA 4

struct PointerMap {
        size_t hashmapsize; // hashmapsize == 0 <==> typeof map == LINKLIST
                            // hashmapsize == -1 <==> typeof map == VLA
        void **map;
};

struct PointerMap lftmmap_init(size_t maxsize);
void __lftmmap_free(struct PointerMap map);

struct PointerMap lftm_stack_last();

enum PointerMapType lftmmap_gettype(struct PointerMap *map);
void lftmmap_insert(struct PointerMap *map, void *ptr);
void lftmmap_foreach(struct PointerMap *map, void (*f)(void *));

void *lftm_malloc(size_t size);
void __lftmmap_export(struct PointerMap map, void *ptr);

void __lftm_start(size_t maxsize);
void __lftm_end();

#endif
