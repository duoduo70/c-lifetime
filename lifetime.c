#include "lifetime.h"

struct PointerLinklistNode {
        void *ptr;
        struct PointerLinklistNode *next;
};

struct PointerHashmapNode {
        void *ptr;
        struct PointerHashmapNode *next;
};

struct PointerVLA {
        size_t capacity;
        size_t len;
        void **array;
};

static size_t lftm_hash(void *ptr, size_t mapsize)
{
        return ((size_t)ptr / sizeof(size_t)) % mapsize;
}

static void **lftm_vla_init(size_t capacity)
{
        struct PointerVLA *vla = malloc(sizeof(struct PointerVLA));
        vla->capacity = capacity;
        vla->len = 0;
        vla->array = malloc(capacity * sizeof(capacity));
        return (void **)vla;
}

static void lftm_vla_insert(struct PointerVLA *vla, void *ptr)
{
        if (vla->len >= vla->capacity) {
                vla->array = realloc(vla->array, vla->capacity << 2);
                vla->capacity = vla->capacity << 2;
        }
        vla->array[vla->len] = ptr;
        vla->len++;
}

static void lftm_vla_export(struct PointerVLA *vla, void *ptr)
{
        size_t idx = 0;
        while (idx < vla->len) {
                if (vla->array[idx] == ptr) {
                        vla->array[idx] = NULL;
                        return;
                }

                idx += 1;
        }
}

static void lftm_vla_foreach(struct PointerVLA *vla, void (*f)(void *))
{
        size_t idx = 0;
        while (idx < vla->len) {
                if (vla->array[idx] != NULL) {
                        f(vla->array[idx]);
                }
                idx++;
        }
}

static void lftm_vla_free(struct PointerVLA *vla)
{
        size_t idx = 0;
        while (idx < vla->len) {
                if (vla->array[idx] != NULL) {
                        free(vla->array[idx]);
                }
                idx += 1;
        }

        free(vla->array);
        free(vla);
}

static struct PointerLinklistNode *
lftm_linklist_insert(struct PointerLinklistNode *linklist, void *ptr)
{
        if (linklist == NULL) {
                linklist = malloc(sizeof(struct PointerLinklistNode));
                linklist->ptr = ptr;
                linklist->next = NULL;
        } else {
                struct PointerLinklistNode *prev = linklist;
                linklist = malloc(sizeof(struct PointerLinklistNode));
                linklist->ptr = ptr;
                linklist->next = prev;
        }
        return linklist;
}

static void lftm_linklist_foreach(struct PointerLinklistNode *linklist,
                                  void (*f)(void *))
{
        struct PointerLinklistNode *node = linklist;
start:
        if (node != NULL) {
                if (node->ptr != NULL) {
                        f(node->ptr);
                }
                node = node->next;
                goto start;
        }
}

static void lftm_linklist_export(struct PointerLinklistNode *linklist,
                                 void *ptr)
{
        struct PointerLinklistNode *node = linklist;
start:
        if (node != NULL) {
                if (node->ptr == ptr) {
                        node->ptr = NULL;
                        return;
                }
                node = node->next;
                goto start;
        }
}

static void lftm_linklist_free(struct PointerLinklistNode *linklist)
{
        struct PointerLinklistNode *node = linklist;
        struct PointerLinklistNode *next;
start:

        if (node != NULL) {
                next = node->next;
                free(node->ptr);
                free(node);
                node = next;
                goto start;
        }
}

static void lftm_hashmap_insert(struct PointerHashmapNode **hashmap,
                                size_t mapsize, void *ptr)
{
        size_t idx = lftm_hash(ptr, mapsize);

        struct PointerHashmapNode *tmp =
            malloc(sizeof(struct PointerHashmapNode));
        tmp->ptr = ptr;
        tmp->next = hashmap[idx];

        hashmap[idx] = tmp;
}

static void lftm_hashmap_foreach(struct PointerHashmapNode **hashmap,
                                 size_t size, void (*f)(void *))
{
        int i = 0;
        while (i < size) {
                if (hashmap[i] != NULL && hashmap[i]->ptr != NULL) {
                        lftm_linklist_foreach(
                            (struct PointerLinklistNode *)hashmap[i], f);
                }
                i++;
        }
}

static void lftm_hashmap_free(struct PointerHashmapNode **hashmap, size_t size)
{
        int i = 0;
        while (i < size) {
                if (hashmap[i] != NULL) {
                        lftm_linklist_free(
                            (struct PointerLinklistNode *)hashmap[i]);
                }
                i++;
        }
        free(hashmap);
}

static void lftm_hashmap_export(struct PointerHashmapNode **hashmap,
                                size_t mapsize, void *ptr)
{
        size_t idx = lftm_hash(ptr, mapsize);
        struct PointerHashmapNode *tmp;

linklist_start:

        tmp = hashmap[idx];
        if (tmp == NULL) {
                return;
        } else if (tmp->ptr == ptr) {
                tmp->ptr = NULL;
                return;
        } else {
                goto linklist_start;
        }
}

static struct PointerMap lftmmap_init_hashmap(size_t mapsize)
{
        struct PointerMap map;
        map.hashmapsize = mapsize;
        map.map = calloc(mapsize, sizeof(size_t));
        return map;
}

static struct PointerMap lftmmap_init_linklist()
{
        struct PointerMap map;
        map.hashmapsize = 0;
        map.map = NULL;
        return map;
}

static struct PointerMap lftmmap_init_vla(size_t capacity)
{
        struct PointerMap map;
        map.hashmapsize = -1;
        map.map = lftm_vla_init(capacity);
        return map;
}

enum PointerMapType lftmmap_gettype(struct PointerMap *map)
{
        switch (map->hashmapsize) {
        case 0:
                return LFTMMAP_TYPE_LINKLIST;
        case -1:
                return LFTMMAP_TYPE_VLA;
        default:
                return LFTMMAP_TYPE_HASHMAP;
        }
}

void lftmmap_insert(struct PointerMap *map, void *ptr)
{
        switch (lftmmap_gettype(map)) {
        case LFTMMAP_TYPE_LINKLIST:
                map->map = (void **)lftm_linklist_insert(
                    (struct PointerLinklistNode *)map->map, ptr);
                break;
        case LFTMMAP_TYPE_HASHMAP:
                lftm_hashmap_insert((struct PointerHashmapNode **)map->map,
                                    map->hashmapsize, ptr);
                break;
        case LFTMMAP_TYPE_VLA:
                lftm_vla_insert((struct PointerVLA *)map->map, ptr);
                break;
        }
}

void lftmmap_foreach(struct PointerMap *map, void (*f)(void *))
{
        switch (lftmmap_gettype(map)) {
        case LFTMMAP_TYPE_LINKLIST:
                lftm_linklist_foreach((struct PointerLinklistNode *)map->map,
                                      f);
                break;
        case LFTMMAP_TYPE_HASHMAP:
                lftm_hashmap_foreach((struct PointerHashmapNode **)map->map,
                                     map->hashmapsize, f);
                break;
        case LFTMMAP_TYPE_VLA:
                lftm_vla_foreach((struct PointerVLA *)map->map, f);
                break;
        }
}

struct PointerMap lftmmap_init(size_t maxsize)
{
        if (maxsize >= LFTMMAP_TYPE_HASHMAP) {
                return lftmmap_init_hashmap(maxsize >> 2);
        } else if (maxsize >= LFTMMAP_TYPE_LINKLIST) {
                return lftmmap_init_linklist();
        } else {
                return lftmmap_init_vla(maxsize);
        }
}

void lftmmap_free(struct PointerMap map)
{
        switch (lftmmap_gettype(&map)) {
        case LFTMMAP_TYPE_LINKLIST:
                lftm_linklist_free((struct PointerLinklistNode *)map.map);
                break;
        case LFTMMAP_TYPE_HASHMAP:
                lftm_hashmap_free((struct PointerHashmapNode **)map.map,
                                  map.hashmapsize);
                break;
        case LFTMMAP_TYPE_VLA:
                lftm_vla_free((struct PointerVLA *)map.map);
                break;
        }
}

void __lftmmap_export(struct PointerMap map, void *ptr)
{
        switch (lftmmap_gettype(&map)) {
        case LFTMMAP_TYPE_LINKLIST:
                lftm_linklist_export((struct PointerLinklistNode *)map.map,
                                     ptr);
                break;
        case LFTMMAP_TYPE_HASHMAP:
                lftm_hashmap_export((struct PointerHashmapNode **)map.map,
                                    map.hashmapsize, ptr);
                break;
        case LFTMMAP_TYPE_VLA:
                lftm_vla_export((struct PointerVLA *)map.map, ptr);
                break;
        }
}

struct LifetimeStack {
        struct PointerMap lifetime;
        struct LifetimeStack *prev;
};

static struct LifetimeStack *lifetime_stack;

static void lftm_stack_push(struct PointerMap lifetime)
{
        struct LifetimeStack *next = malloc(sizeof(struct LifetimeStack));
        next->lifetime = lifetime;
        next->prev = lifetime_stack;
        lifetime_stack = next;
}

static void lftm_stack_pop()
{
        struct LifetimeStack *prev = lifetime_stack->prev;
        lftmmap_free(lifetime_stack->lifetime);
        free(lifetime_stack);
        lifetime_stack = prev;
}

struct PointerMap lftm_stack_last() { return lifetime_stack->lifetime; }

void *lftm_malloc(size_t size)
{
        void *ptr = malloc(size);
        if (lifetime_stack != NULL)
                lftmmap_insert(&lifetime_stack->lifetime, ptr);
        return ptr;
}

void __lftm_start(size_t maxsize) { lftm_stack_push(lftmmap_init(maxsize)); }

void __lftm_end() { lftm_stack_pop(); }
