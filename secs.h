/* secs.h -- https://github.com/takeiteasy/secs
 
 secs -- Simple Entity Component System
 
 Copyright (C) 2024  George Watson

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
 
 Acknowledgements

 - Built using [imap](https://github.com/billziss-gh/imap), an exellent int->int map */

#ifndef __BLOCKS__
#define SECS_NO_BLOCKS
#endif

#ifndef SECS_HEADER
#define SECS_HEADER
#ifdef SECS_NO_BOOL
#define bool int
#define true 1
#define false 0
#else
#include <stdbool.h>
#endif
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

typedef union {
    struct {
        uint32_t id;
        uint16_t version;
        uint8_t alive;
        uint8_t type;
    };
    uint64_t value;
} entity_t;

enum ecs_entity_type : uint8_t {
    ECS_TYPE_ENTITY,
    ECS_TYPE_COMPONENT,
    ECS_TYPE_SYSTEM
};

const uint64_t ecs_nil = 0xFFFFFFFFull;
const entity_t ecs_nil_entity = {.value = ecs_nil};

typedef struct ecs world_t;

#ifndef SECS_NO_BLOCKS
typedef void(^ecs_callback_t)(entity_t);
typedef bool(^ecs_filter_t)(entity_t);
#else
typedef void(*ecs_callback_t)(entity_t);
typedef bool(*ecs_filter_t)(entity_t);
#endif

world_t* ecs_create(void);
void ecs_destroy(world_t *world);

entity_t ecs_spawn(world_t *world);
void ecs_delete(world_t *world, entity_t entity);

bool ecs_cmp(entity_t a, entity_t b);
bool ecs_isvalid(world_t *world, entity_t entity);
bool ecs_isnil(entity_t entity);

entity_t ecs_component(world_t *world, size_t component_size);
entity_t ecs_system(world_t *world, ecs_callback_t callback, ecs_filter_t filter, int component_count, ...);

void* ecs_attach(world_t *world, entity_t entity, entity_t component);
bool ecs_has(world_t *world, entity_t entity, entity_t component);
void ecs_detach(world_t *world, entity_t entity, entity_t component);
void* ecs_get(world_t *world, entity_t entity, entity_t component);
void ecs_set(world_t *world, entity_t entity, entity_t component, void *data);

void ecs_step(world_t *world);

entity_t* ecs_find(world_t *world, ecs_filter_t filter, int *result_count, int component_count, ...);
void ecs_query(world_t *world, ecs_callback_t callback, ecs_filter_t filter, int component_count, ...);

#endif // SECS_HEADER

#ifdef SECS_IMPLEMENTATION
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef IMAP_IMPLEMENTATION
#define IMAP_IMPLEMENTATION
typedef struct {
    union {
        uint32_t vec32[16];
        uint64_t vec64[8];
    };
} imap_node_t;

#define imap__tree_root__           0
#define imap__tree_resv__           1
#define imap__tree_mark__           2
#define imap__tree_size__           3
#define imap__tree_nfre__           4
#define imap__tree_vfre__           5

#define imap__slot_pmask__          0x0000000f
#define imap__slot_node__           0x00000010
#define imap__slot_scalar__         0x00000020
#define imap__slot_value__          0xffffffe0
#define imap__slot_shift__          6
#define imap__slot_boxed__(sval)    (!((sval) & imap__slot_scalar__) && ((sval) >> imap__slot_shift__))

typedef struct {
    uint32_t stack[16];
    uint32_t stackp;
} imap_iter_t;

typedef struct {
    uint64_t x;
    uint32_t *slot;
} imap_pair_t;

#define imap__pair_zero__           ((imap_pair_t){0})
#define imap__pair__(x, slot)       ((imap_pair_t){(x), (slot)})
#define imap__node_zero__           ((imap_node_t){{{0}}})

#if defined(_MSC_VER)
static inline uint32_t imap__bsr__(uint64_t x) {
    return _BitScanReverse64((unsigned long *)&x, x | 1), (unsigned long)x;
}
#else
static inline uint32_t imap__bsr__(uint64_t x) {
    return 63 - __builtin_clzll(x | 1);
}
#endif

static inline uint32_t imap__xpos__(uint64_t x) {
    return imap__bsr__(x) >> 2;
}

static inline uint64_t imap__ceilpow2__(uint64_t x) {
    return 1ull << (imap__bsr__(x - 1) + 1);
}

static inline void *imap__aligned_alloc__(uint64_t alignment, uint64_t size) {
    void *p = malloc(size + sizeof(void *) + alignment - 1);
    if (!p)
        return p;
    void **ap = (void**)(((uint64_t)p + sizeof(void *) + alignment - 1) & ~(alignment - 1));
    ap[-1] = p;
    return ap;
}

static inline void imap__aligned_free__(void *p) {
    if (p)
        free(((void**)p)[-1]);
}

#define IMAP_ALIGNED_ALLOC(a, s)    (imap__aligned_alloc__(a, s))
#define IMAP_ALIGNED_FREE(p)        (imap__aligned_free__(p))

static inline imap_node_t* imap__node__(imap_node_t *tree, uint32_t val) {
    return (imap_node_t*)((uint8_t*)tree + val);
}

static inline uint32_t imap__node_pos__(imap_node_t *node) {
    return node->vec32[0] & 0xf;
}

static inline uint64_t imap__extract_lo4_port__(uint32_t vec32[16]) {
    union {
        uint32_t *vec32;
        uint64_t *vec64;
    } u;
    u.vec32 = vec32;
    return
        ((u.vec64[0] & 0xf0000000full)) |
        ((u.vec64[1] & 0xf0000000full) << 4) |
        ((u.vec64[2] & 0xf0000000full) << 8) |
        ((u.vec64[3] & 0xf0000000full) << 12) |
        ((u.vec64[4] & 0xf0000000full) << 16) |
        ((u.vec64[5] & 0xf0000000full) << 20) |
        ((u.vec64[6] & 0xf0000000full) << 24) |
        ((u.vec64[7] & 0xf0000000full) << 28);
}

static inline void imap__deposit_lo4_port__(uint32_t vec32[16], uint64_t value) {
    union {
        uint32_t *vec32;
        uint64_t *vec64;
    } u;
    u.vec32 = vec32;
    u.vec64[0] = (u.vec64[0] & ~0xf0000000full) | ((value) & 0xf0000000full);
    u.vec64[1] = (u.vec64[1] & ~0xf0000000full) | ((value >> 4) & 0xf0000000full);
    u.vec64[2] = (u.vec64[2] & ~0xf0000000full) | ((value >> 8) & 0xf0000000full);
    u.vec64[3] = (u.vec64[3] & ~0xf0000000full) | ((value >> 12) & 0xf0000000full);
    u.vec64[4] = (u.vec64[4] & ~0xf0000000full) | ((value >> 16) & 0xf0000000full);
    u.vec64[5] = (u.vec64[5] & ~0xf0000000full) | ((value >> 20) & 0xf0000000full);
    u.vec64[6] = (u.vec64[6] & ~0xf0000000full) | ((value >> 24) & 0xf0000000full);
    u.vec64[7] = (u.vec64[7] & ~0xf0000000full) | ((value >> 28) & 0xf0000000full);
}

static inline void imap__node_setprefix__(imap_node_t *node, uint64_t prefix) {
    imap__deposit_lo4_port__(node->vec32, prefix);
}

static inline uint64_t imap__node_prefix__(imap_node_t *node) {
    return imap__extract_lo4_port__(node->vec32);
}

static inline uint32_t imap__xdir__(uint64_t x, uint32_t pos) {
    return (x >> (pos << 2)) & 0xf;
}

static inline uint32_t imap__popcnt_hi28_port__(uint32_t vec32[16], uint32_t *p) {
    uint32_t pcnt = 0, sval, dirn;
    *p = 0;
    for (dirn = 0; 16 > dirn; dirn++)
    {
        sval = vec32[dirn];
        if (sval & ~0xf)
        {
            *p = sval;
            pcnt++;
        }
    }
    return pcnt;
}

static inline uint32_t imap__node_popcnt__(imap_node_t *node, uint32_t *p) {
    return imap__popcnt_hi28_port__(node->vec32, p);
}

static inline uint32_t imap__alloc_node__(imap_node_t *tree) {
    uint32_t mark = tree->vec32[imap__tree_nfre__];
    if (mark)
        tree->vec32[imap__tree_nfre__] = *(uint32_t*)((uint8_t *)tree + mark);
    else {
        mark = tree->vec32[imap__tree_mark__];
        assert(mark + sizeof(imap_node_t) <= tree->vec32[imap__tree_size__]);
        tree->vec32[imap__tree_mark__] = mark + sizeof(imap_node_t);
    }
    return mark;
}

static inline void imap__free_node__(imap_node_t *tree, uint32_t mark) {
    *(uint32_t *)((uint8_t *)tree + mark) = tree->vec32[imap__tree_nfre__];
    tree->vec32[imap__tree_nfre__] = mark;
}

static inline uint64_t imap__xpfx__(uint64_t x, uint32_t pos) {
    return x & (~0xfull << (pos << 2));
}

static imap_node_t* imap_ensure(imap_node_t *tree, size_t capacity) {
    if (!capacity)
        return NULL;
    imap_node_t *newtree;
    uint32_t hasnfre, hasvfre, newmark, oldsize, newsize;
    uint64_t newsize64;
    if (!tree) {
        hasnfre = 0;
        hasvfre = 1;
        newmark = sizeof(imap_node_t);
        oldsize = 0;
    } else {
        hasnfre = !!tree->vec32[imap__tree_nfre__];
        hasvfre = !!tree->vec32[imap__tree_vfre__];
        newmark = tree->vec32[imap__tree_mark__];
        oldsize = tree->vec32[imap__tree_size__];
    }
    newmark += (capacity * 2 - hasnfre) * sizeof(imap_node_t) + (capacity - hasvfre) * sizeof(uint64_t);
    if (newmark <= oldsize)
        return tree;
    newsize64 = imap__ceilpow2__(newmark);
    if (0x20000000 < newsize64)
        return NULL;
    newsize = (uint32_t)newsize64;
    newtree = (imap_node_t*)IMAP_ALIGNED_ALLOC(sizeof(imap_node_t), newsize);
    if (!newtree)
        return newtree;
    if (tree) {
        memcpy(newtree, tree, tree->vec32[imap__tree_mark__]);
        IMAP_ALIGNED_FREE(tree);
        newtree->vec32[imap__tree_size__] = newsize;
    } else {
        newtree->vec32[imap__tree_root__] = 0;
        newtree->vec32[imap__tree_resv__] = 0;
        newtree->vec32[imap__tree_mark__] = sizeof(imap_node_t);
        newtree->vec32[imap__tree_size__] = newsize;
        newtree->vec32[imap__tree_nfre__] = 0;
        newtree->vec32[imap__tree_vfre__] = 3 << imap__slot_shift__;
        newtree->vec64[3] = 4 << imap__slot_shift__;
        newtree->vec64[4] = 5 << imap__slot_shift__;
        newtree->vec64[5] = 6 << imap__slot_shift__;
        newtree->vec64[6] = 7 << imap__slot_shift__;
        newtree->vec64[7] = 0;
    }
    return newtree;
}

static inline uint32_t imap__alloc_val__(imap_node_t *tree) {
    uint32_t mark = imap__alloc_node__(tree);
    imap_node_t *node = imap__node__(tree, mark);
    mark <<= 3;
    tree->vec32[imap__tree_vfre__] = mark;
    node->vec64[0] = mark + (1 << imap__slot_shift__);
    node->vec64[1] = mark + (2 << imap__slot_shift__);
    node->vec64[2] = mark + (3 << imap__slot_shift__);
    node->vec64[3] = mark + (4 << imap__slot_shift__);
    node->vec64[4] = mark + (5 << imap__slot_shift__);
    node->vec64[5] = mark + (6 << imap__slot_shift__);
    node->vec64[6] = mark + (7 << imap__slot_shift__);
    node->vec64[7] = 0;
    return mark;
}

static void imap_setval64(imap_node_t *tree, uint32_t *slot, uint64_t y) {
    assert(!(*slot & imap__slot_node__));
    uint32_t sval = *slot;
    if (!(sval >> imap__slot_shift__)) {
        sval = tree->vec32[imap__tree_vfre__];
        if (!sval)
            sval = imap__alloc_val__(tree);
        assert(sval >> imap__slot_shift__);
        tree->vec32[imap__tree_vfre__] = (uint32_t)tree->vec64[sval >> imap__slot_shift__];
    }
    assert(!(sval & imap__slot_node__));
    assert(imap__slot_boxed__(sval));
    *slot = (*slot & imap__slot_pmask__) | sval;
    tree->vec64[sval >> imap__slot_shift__] = y;
}

static uint64_t imap_getval(imap_node_t *tree, uint32_t *slot) {
    assert(!(*slot & imap__slot_node__));
    uint32_t sval = *slot;
    if (!imap__slot_boxed__(sval))
        return sval >> imap__slot_shift__;
    else
        return tree->vec64[sval >> imap__slot_shift__];
}

static void imap_delval(imap_node_t *tree, uint32_t *slot) {
    assert(!(*slot & imap__slot_node__));
    uint32_t sval = *slot;
    if (imap__slot_boxed__(sval)) {
        tree->vec64[sval >> imap__slot_shift__] = tree->vec32[imap__tree_vfre__];
        tree->vec32[imap__tree_vfre__] = sval & imap__slot_value__;
    }
    *slot &= imap__slot_pmask__;
}

static void imap_remove(imap_node_t *tree, uint64_t x) {
    uint32_t *slotstack[16 + 1];
    uint32_t stackp;
    imap_node_t *node = tree;
    uint32_t *slot;
    uint32_t sval, pval, posn = 16, dirn = 0;
    stackp = 0;
    for (;;) {
        slot = &node->vec32[dirn];
        sval = *slot;
        if (!(sval & imap__slot_node__)) {
            if ((sval & imap__slot_value__) && imap__node_prefix__(node) == (x & ~0xfull)) {
                assert(0 == posn);
                imap_delval(tree, slot);
            }
            while (stackp) {
                slot = slotstack[--stackp];
                sval = *slot;
                node = imap__node__(tree, sval & imap__slot_value__);
                posn = imap__node_pos__(node);
                if (!!posn != imap__node_popcnt__(node, &pval))
                    break;
                imap__free_node__(tree, sval & imap__slot_value__);
                *slot = (sval & imap__slot_pmask__) | (pval & ~imap__slot_pmask__);
            }
            return;
        }
        node = imap__node__(tree, sval & imap__slot_value__);
        posn = imap__node_pos__(node);
        dirn = imap__xdir__(x, posn);
        slotstack[stackp++] = slot;
    }
}

static imap_pair_t imap_iterate(imap_node_t *tree, imap_iter_t *iter, int restart) {
    imap_node_t *node;
    uint32_t *slot;
    uint32_t sval, dirn;
    if (restart) {
        iter->stackp = 0;
        sval = dirn = 0;
        goto enter;
    }
    // loop while stack is not empty
    while (iter->stackp) {
        // get slot value and increment direction
        sval = iter->stack[iter->stackp - 1]++;
        dirn = sval & 31;
        if (15 < dirn) {
            // if directions 0-15 have been examined, pop node from stack
            iter->stackp--;
            continue;
        }
    enter:
        node = imap__node__(tree, sval & imap__slot_value__);
        slot = &node->vec32[dirn];
        sval = *slot;
        if (sval & imap__slot_node__)
            // push node into stack
            iter->stack[iter->stackp++] = sval & imap__slot_value__;
        else if (sval & imap__slot_value__)
            return imap__pair__(imap__node_prefix__(node) | dirn, slot);
    }
    return imap__pair_zero__;
}
#endif // IMAP_IMPLEMENTATION

typedef struct {
    imap_node_t *tree;
    size_t count, capacity;
} storage_t;

static uint32_t* storage_find(storage_t *map, uint64_t x, int ensure) {
    uint32_t *slotstack[16 + 1];
    uint32_t posnstack[16 + 1];
    uint32_t stackp, stacki;
    imap_node_t *newnode, *node = map->tree;
    uint32_t *slot;
    uint32_t newmark, sval, diff, posn = 16, dirn = 0;
    uint64_t prfx;
    stackp = 0;
    for (;;) {
        slot = &node->vec32[dirn];
        sval = *slot;
        slotstack[stackp] = slot;
        posnstack[stackp++] = posn;
        if (!(sval & imap__slot_node__)) {
            prfx = imap__node_prefix__(node);
            if (!ensure || (!posn && prfx == (x & ~0xfull)))
                return slot;
            if (++map->count > map->capacity) {
                map->capacity *= 2;
                map->tree = imap_ensure(map->tree, map->capacity);
            }
            diff = imap__xpos__(prfx ^ x);
            assert(diff < 16);
            for (stacki = stackp; diff > posn;)
                posn = posnstack[--stacki];
            if (stacki != stackp) {
                slot = slotstack[stacki];
                sval = *slot;
                assert(sval & imap__slot_node__);
                newmark = imap__alloc_node__(map->tree);
                *slot = (*slot & imap__slot_pmask__) | imap__slot_node__ | newmark;
                newnode = imap__node__(map->tree, newmark);
                *newnode = imap__node_zero__;
                newmark = imap__alloc_node__(map->tree);
                newnode->vec32[imap__xdir__(prfx, diff)] = sval;
                newnode->vec32[imap__xdir__(x, diff)] = imap__slot_node__ | newmark;
                imap__node_setprefix__(newnode, imap__xpfx__(prfx, diff) | diff);
            } else {
                newmark = imap__alloc_node__(map->tree);
                *slot = (*slot & imap__slot_pmask__) | imap__slot_node__ | newmark;
            }
            newnode = imap__node__(map->tree, newmark);
            *newnode = imap__node_zero__;
            imap__node_setprefix__(newnode, x & ~0xfull);
            return &newnode->vec32[x & 0xfull];
        }
        node = imap__node__(map->tree, sval & imap__slot_value__);
        posn = imap__node_pos__(node);
        dirn = imap__xdir__(x, posn);
    }
}

static storage_t* make_storage(void) {
    storage_t *result = malloc(sizeof(storage_t));
    result->capacity = 8;
    result->count = 0;
    result->tree = imap_ensure(NULL, 8);
    return result;
}

static int storage_set(storage_t *map, uint64_t key, void *item) {
    uint32_t *slot = storage_find(map, key, 1);
    if (!slot)
        return 0;
    imap_setval64(map->tree, slot, (uint64_t)item);
    return 1;
}

static void* storage_get(storage_t *map, uint64_t key) {
    uint32_t *slot = storage_find(map, key, 0);
    return slot ? (void*)imap_getval(map->tree, slot) : NULL;
}

static void* storage_del(storage_t *map, uint64_t key) {
    if (!map->count)
        return NULL;
    uint32_t *slot = storage_find(map, key, 0);
    if (!slot)
        return NULL;
    void* val = (void*)imap_getval(map->tree, slot);
    imap_remove(map->tree, key);
    map->count--;
    return val;
}

static void destroy_storage(storage_t *map) {
    IMAP_ALIGNED_FREE(map->tree);
    free(map);
}

static int entity_storage_set(storage_t *map, uint64_t key, entity_t item) {
    return storage_set(map, key, (void*)item.value);
}

static entity_t entity_storage_get(storage_t *map, uint64_t key) {
    void *result = storage_get(map, key);
    return result ? (entity_t){.value=(uint64_t)result} : ecs_nil_entity;
}

typedef struct {
    entity_t *entities;
    size_t entity_count;
} entity_array_t;

static void entity_array_append(entity_array_t *arr, entity_t entity) {
    arr->entities = realloc(arr->entities, ++arr->entity_count * sizeof(entity_t));
    arr->entities[arr->entity_count-1] = entity;
}

static entity_t entity_array_pop(entity_array_t *arr) {
    if (!arr->entities || !arr->entity_count)
        return ecs_nil_entity;
    entity_t entity = arr->entities[arr->entity_count-1];
    arr->entities = realloc(arr->entities, --arr->entity_count * sizeof(entity_t));
    return entity;
}

void destroy_entity_array(entity_array_t *arr) {
    if (arr->entities)
        free(arr->entities);
}

struct ecs {
    storage_t *entities;
    storage_t *components;
    storage_t *systems;
    size_t entity_count;
    entity_array_t recyclable;
    uint32_t next_id;
};

static entity_t make_entity(world_t *world, enum ecs_entity_type type) {
    if (world->recyclable.entity_count) {
        entity_t old = entity_array_pop(&world->recyclable);
        entity_t copy = entity_storage_get(world->entities, old.id);
        copy.alive = 1;
        copy.type = type;
        entity_storage_set(world->entities, copy.id, copy);
        return copy;
    } else {
        entity_t e = (entity_t) {
            .id = (uint32_t)world->entity_count++,
            .version = 0,
            .alive = 1,
            .type = type
        };
        entity_storage_set(world->entities, e.id, e);
        return e;
    }
}

world_t* ecs_create(void) {
    world_t *world = malloc(sizeof(world_t));
    memset(world, 0, sizeof(world_t));
    world->next_id = ecs_nil;
    world->entities = make_storage();
    world->components = make_storage();
    world->systems = make_storage();
    return world;
}

void ecs_destroy(world_t *world) {
    destroy_storage(world->entities);
    destroy_entity_array(&world->recyclable);
    destroy_storage(world->components);
    destroy_storage(world->systems);
    free(world);
}

entity_t ecs_spawn(world_t *world) {
    return make_entity(world, ECS_TYPE_ENTITY);
}

typedef struct {
    storage_t *data;
    size_t data_size;
} component_data;

typedef struct {
    ecs_callback_t callback;
    ecs_filter_t filter;
    entity_array_t components;
} system_data;

void ecs_delete(world_t *world, entity_t entity) {
    assert(ecs_isvalid(world, entity));
    entity_t e = entity_storage_get(world->entities, entity.id);
    switch (e.type) {
        case ECS_TYPE_ENTITY:
            break;
        case ECS_TYPE_COMPONENT: {
            component_data *cd = storage_del(world->components, entity.id);
            assert(cd);
            destroy_storage(cd->data);
            free(cd);
            break;
        }
        case ECS_TYPE_SYSTEM: {
            system_data *sd = storage_del(world->systems, entity.id);
            assert(sd);
            destroy_entity_array(&sd->components);
            free(sd);
            break;
        }
    }
    e.version++;
    e.alive = 0;
    e.type = 0;
    entity_array_append(&world->recyclable, e);
    entity_storage_set(world->entities, entity.id, e);
}

bool ecs_cmp(entity_t a, entity_t b) {
    return a.value == b.value;
}

bool ecs_isvalid(world_t *world, entity_t entity) {
    if (!entity.alive)
        return false;
    entity_t e = entity_storage_get(world->entities, entity.id);
    return ecs_isnil(e) ? false : ecs_cmp(entity, e);
}

bool ecs_isnil(entity_t entity) {
    return entity.value == ecs_nil;
}

bool ecs_isa(world_t *world, entity_t entity, enum ecs_entity_type type) {
    return ecs_isvalid(world, entity) && entity.type == type;
}

entity_t ecs_component(world_t *world, size_t component_size) {
    entity_t component = make_entity(world, ECS_TYPE_COMPONENT);
    void *found = storage_get(world->components, component.id);
    if (found) {
        storage_del(world->components, component.id);
        component_data *cd = found;
        destroy_storage(cd->data);
        free(found);
    }
    component_data *cd = malloc(sizeof(component_data));
    cd->data = make_storage();
    cd->data_size = component_size;
    storage_set(world->components, component.id, cd);
    return component;
}

entity_t ecs_system(world_t *world, ecs_callback_t callback, ecs_filter_t filter, int component_count, ...) {
    entity_t system = make_entity(world, ECS_TYPE_SYSTEM);
    void *found = storage_get(world->systems, system.id);
    if (found) {
        storage_del(world->systems, system.id);
        system_data *sd = found;
        destroy_entity_array(&sd->components);
        free(found);
    }
    system_data *sd = malloc(sizeof(system_data));
    sd->callback = callback;
    sd->filter = filter;
    memset(&sd->components, 0, sizeof(entity_array_t));
    va_list args;
    va_start(args, component_count);
    for (int i = 0; i < component_count; i++) {
        entity_t component = va_arg(args, entity_t);
        assert(ecs_isa(world, component, ECS_TYPE_COMPONENT));
        entity_array_append(&sd->components, component);
    }
    va_end(args);
    storage_set(world->systems, system.id, sd);
    return system;
}

void* ecs_attach(world_t *world, entity_t entity, entity_t component) {
    assert(ecs_isa(world, entity, ECS_TYPE_ENTITY));
    assert(ecs_isa(world, component, ECS_TYPE_COMPONENT));
    component_data *cd = storage_get(world->components, component.id);
    assert(cd);
    void *data = storage_get(cd->data, entity.id);
    assert(!data);
    void *mem = malloc(cd->data_size);
    storage_set(cd->data, entity.id, mem);
    return mem;
}

bool ecs_has(world_t *world, entity_t entity, entity_t component) {
    assert(ecs_isa(world, entity, ECS_TYPE_ENTITY));
    assert(ecs_isa(world, component, ECS_TYPE_COMPONENT));
    component_data *cd = storage_get(world->components, component.id);
    assert(cd);
    return !!storage_get(cd->data, entity.id);
}

void ecs_detach(world_t *world, entity_t entity, entity_t component) {
    assert(ecs_isa(world, entity, ECS_TYPE_ENTITY));
    assert(ecs_isa(world, component, ECS_TYPE_COMPONENT));
    component_data *cd = storage_get(world->components, component.id);
    assert(cd);
    assert(storage_get(cd->data, entity.id));
    return free(storage_del(cd->data, entity.id));
}

void* ecs_get(world_t *world, entity_t entity, entity_t component) {
    assert(ecs_isa(world, entity, ECS_TYPE_ENTITY));
    assert(ecs_isa(world, component, ECS_TYPE_COMPONENT));
    component_data *cd = storage_get(world->components, component.id);
    assert(cd);
    void *data = storage_get(cd->data, entity.id);
    assert(data);
    return data;
}

void ecs_set(world_t *world, entity_t entity, entity_t component, void *data) {
    assert(ecs_isa(world, entity, ECS_TYPE_ENTITY));
    assert(ecs_isa(world, component, ECS_TYPE_COMPONENT));
    component_data *cd = storage_get(world->components, component.id);
    assert(cd);
    void *dst = storage_get(cd->data, entity.id);
    assert(dst);
    memcpy(dst, data, cd->data_size);
}

static void query(world_t *world, entity_array_t *components, entity_array_t *out) {
    component_data **storages = malloc(components->entity_count * sizeof(component_data*));
    for (int i = 0; i < components->entity_count; i++) {
        assert(ecs_isa(world, components->entities[i], ECS_TYPE_COMPONENT));
        storages[i] = storage_get(world->components, components->entities[i].id);
        assert(storages[i]);
    }
    imap_iter_t iter;
    imap_pair_t pair = imap_iterate(world->entities->tree, &iter, 1);
    for (;;) {
        if (!pair.slot)
            break;
        entity_t entity = (entity_t){.value=imap_getval(world->entities->tree, pair.slot)};
        if (ecs_isa(world, entity, ECS_TYPE_ENTITY)) {
            bool add_to_array = true;
            for (int i = 0; i < components->entity_count; i++) {
                component_data *cd = storages[i];
                if (!storage_get(cd->data, entity.id)) {
                    add_to_array = false;
                    break;
                }
            }
            if (add_to_array)
                entity_array_append(out, entity);
        }
        pair = imap_iterate(world->entities->tree, &iter, 0);
    }
    free(storages);
}

void ecs_step(world_t *world) {
    imap_iter_t iter;
    imap_pair_t pair = imap_iterate(world->systems->tree, &iter, 1);
    for (;;) {
        if (!pair.slot)
            break;
        system_data *sd = (system_data*)imap_getval(world->systems->tree, pair.slot);
        entity_array_t entities;
        query(world, &sd->components, &entities);
        for (int i = 0; i < entities.entity_count; i++) {
            if (sd->filter)
                if (!sd->filter(entities.entities[i]))
                    continue;
            sd->callback(entities.entities[i]);
        }
        destroy_entity_array(&entities);
        pair = imap_iterate(world->systems->tree, &iter, 0);
    }
}

static void components_to_array(world_t *world, entity_array_t *out, int n, va_list args) {
    for (int i = 0; i < n; i++) {
        entity_t component = va_arg(args, entity_t);
        assert(ecs_isa(world, component, ECS_TYPE_COMPONENT));
        entity_array_append(out, component);
    }
    va_end(args);
}

entity_t* ecs_find(world_t *world, ecs_filter_t filter, int *result_count, int component_count, ...) {
    entity_array_t components;
    va_list args;
    va_start(args, component_count);
    components_to_array(world, &components, component_count, args);
    
    entity_array_t tmp;
    query(world, &components, &tmp);
    if (result_count)
        *result_count = (int)tmp.entity_count;
    destroy_entity_array(&components);
    return tmp.entities;
}

void ecs_query(world_t *world, ecs_callback_t callback, ecs_filter_t filter, int component_count, ...) {
    entity_array_t components;
    memset(&components, 0, sizeof(entity_array_t));
    va_list args;
    va_start(args, component_count);
    components_to_array(world, &components, component_count, args);
    
    entity_array_t entities;
    query(world, &components, &entities);
    for (int i = 0; i < entities.entity_count; i++)
        callback(entities.entities[i]);
    destroy_entity_array(&components);
    destroy_entity_array(&entities);
}
#endif // SECS_IMPLEMENTATION
