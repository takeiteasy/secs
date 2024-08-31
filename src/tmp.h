//
//  tmp.c
//  secs
//
//  Created by George Watson on 31/08/2024.
//

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
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef union {
    struct {
        uint32_t id;
        uint16_t version;
        uint8_t unused;
        uint8_t flag;
    };
    uint64_t value;
} entity_t;

extern const uint64_t ecs_nil;
extern const entity_t ecs_nil_entity;
typedef struct ecs ecs_t;
typedef ecs_t world_t;

typedef struct {
    entity_t id;
    size_t size;
    const char *name;
} component_t;

typedef struct {
    entity_t *components;
    size_t component_count;
} component_list_t;

typedef struct {
    entity_t entity;
    component_list_t components;
    void **component_data;
} view_t;

typedef component_list_t prefab_t;

#ifndef SECS_NO_BLOCKS
typedef void(^system_cb_t)(view_t*);
#else
typedef void(*system_cb_t)(view_t*);
#endif

typedef struct {
    system_cb_t callback;
    component_list_t components;
} system_t;

typedef struct {
    entity_t object, relation, relation_type;
} relation_t;

extern entity_t ECS_SYSTEM;
extern entity_t ECS_PREFAB;
extern entity_t ECS_RELATION;

typedef enum : int {
    ECS_TYPE_ENTITY = 0,
    ECS_TYPE_COMPONENT,
    ECS_TYPE_SYSTEM,
    ECS_TYPE_PREFAB,
    ECS_TYPE_RELATION,
} ecs_entity_type_t;

world_t* ecs_create(void);
void ecs_destroy(world_t*);
void ecs_step(world_t*);
entity_t ecs_spawn(world_t*);
entity_t ecs_delete(world_t*, entity_t);
bool ecs_isvalid(world_t*, entity_t);
bool ecs_isa(world_t*, entity_t a, ecs_entity_type_t type);
bool ecs_isnil(world_t*, entity_t a);
bool ecs_cmp(entity_t, entity_t);
void ecs_give(world_t*, entity_t entity, entity_t component);
void ecs_remove(world_t*, entity_t entity, entity_t component);
bool ecs_has(world_t*, entity_t entity, entity_t component);
void* ecs_get(world_t*, entity_t entity, entity_t component);
void ecs_set(world_t*, entity_t entity, entity_t component, const void *data);
void ecs_associate(world_t*, entity_t object, entity_t relation, entity_t relation_type);
void ecs_disassociate(world_t*, entity_t entity, entity_t relation_type);
bool ecs_has_relation(world_t*, entity_t entity, entity_t relation_type);
void ecs_relations(world_t*, entity_t entity, entity_t relation_type, system_cb_t);

// Taken from: https://gist.github.com/61131/7a22ac46062ee292c2c8bd6d883d28de
#define N_ARGS(...) _NARG_(__VA_ARGS__, _RSEQ())
#define _NARG_(...) _SEQ(__VA_ARGS__)
#define _SEQ(_1, _2, _3, _4, _5, _6, _7, _8, _9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64,_65,_66,_67,_68,_69,_70,_71,_72,_73,_74,_75,_76,_77,_78,_79,_80,_81,_82,_83,_84,_85,_86,_87,_88,_89,_90,_91,_92,_93,_94,_95,_96,_97,_98,_99,_100,_101,_102,_103,_104,_105,_106,_107,_108,_109,_110,_111,_112,_113,_114,_115,_116,_117,_118,_119,_120,_121,_122,_123,_124,_125,_126,_127,N,...) N
#define _RSEQ() 127,126,125,124,123,122,121,120,119,118,117,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

void __ecs_query(world_t*, system_cb_t, int n, ...);
#define ecs_query(WORLD, CB, COMPONENTS) __ecs_query((WORLD), (CB), N_ARGS(__VA_ARGS__), __VA_ARGS__)

entity_t __ecs_new_component(world_t*, size_t);
#define ecs_component(WORLD, COMPONENT) __ecs_new_component((WORLD), sizeof((COMPONENT)))
#define ecs_tag(WORLD) __ecs_new_component((WORLD), 0)

entity_t __ecs_new_system(world_t*, system_cb_t, int, ...);
#define ecs_system(WORLD, CB, ...) __ecs_new_system(WORLD, CB, N_ARGS(__VA_ARGS__), __VA_ARGS__)

entity_t __ecs_new_prefab(world_t*, int, ...);
#define ecs_prefab(WORLD, ...) __ecs_new_prefab(WORLD, N_ARGS(__VA_ARGS__), __VA_ARGS__);

entity_t __ecs_new_relation(world_t*);
#define ecs_relation(WORLD) __ecs_new_relation((WORLD))

#endif // SECS_HEADER

#ifdef SECS_IMPLEMENTATION
#include <stdio.h>
#include <assert.h>

const uint64_t ecs_nil = 0xFFFFFFFFull;
const entity_t ecs_nil_entity = {.value = ecs_nil};

typedef struct {
    entity_t *entities;
    size_t count;
} ecs_array;

typedef struct {
    entity_t component;
    void *data;
    size_t data_size;
    size_t component_size;
    ecs_array sparse;
    ecs_array dense;
} ecs_storage;

static void dump_entity(entity_t e) {
    printf("(entity %llx: %d, %d, %d)\n", e.value, e.id, e.version, e.flag);
}

static void dump_storage(ecs_storage *storage) {
    printf("*** DUMP STORAGE ***\n");
    printf("componentId: %u, sizeOfData: %zu, sizeOfComponent: %zu\n",
           storage->component.id, storage->data_size, storage->component_size);
    for (int i = 0; i < storage->sparse.count; i++)
        dump_entity(storage->sparse.entities[i]);
    printf("Dense Contents:\n");
    for (int i = 0; i < storage->dense.count; i++)
        dump_entity(storage->dense.entities[i]);
    printf("*** END STORAGE DUMP ***\n");
}

static ecs_storage* make_storage(entity_t entity, size_t component_size) {
    ecs_storage *result = malloc(sizeof(ecs_storage));
    result->component = entity;
    result->component_size = component_size;
    result->data_size = 0;
    result->data = NULL;
    memset(&result->sparse, 0, sizeof(ecs_array));
    memset(&result->dense, 0, sizeof(ecs_array));
    return result;
}

static bool storage_has(ecs_storage *storage, entity_t entity) {
    return entity.id < storage->sparse.count && storage->sparse.entities[entity.id].id != ecs_nil;
}

static void* storage_emplace(ecs_storage *storage, entity_t entity) {
    storage->data = realloc(storage->data, (storage->data_size + 1) * sizeof(char) * storage->component_size);
    storage->data_size++;
    void *result = &((char*)storage->data)[(storage->data_size - 1) * sizeof(char) * storage->component_size];
    
    if (entity.id >= storage->sparse.count) {
        const size_t newSize = entity.id + 1;
        storage->sparse.entities = realloc(storage->sparse.entities, newSize * sizeof * storage->sparse.entities);
        for (size_t i = storage->sparse.count; i < newSize; i++)
            storage->sparse.entities[i] = ecs_nil_entity;
        storage->sparse.count = newSize;
    }
    storage->sparse.entities[entity.id] = (entity_t){.id=(uint32_t)storage->sparse.count};
    storage->dense.entities = realloc(storage->dense.entities, (storage->dense.count + 1) * sizeof * storage->dense.entities);
    storage->dense.entities[storage->dense.count++] = entity;
    
    return result;
}

static void storage_remove(ecs_storage *storage, entity_t entity) {
    assert(storage_has(storage, entity));
    
    uint32_t pos = storage->sparse.entities[entity.id].id;
    entity_t other = storage->dense.entities[storage->dense.count-1];
    
    storage->sparse.entities[other.id] = (entity_t){.id=pos};
    storage->dense.entities[pos] = other;
    storage->sparse.entities[entity.id] = ecs_nil_entity;
    storage->dense.entities = realloc(storage->dense.entities, --storage->dense.count * sizeof * storage->dense.entities);
    
    memmove(&((char*)storage->data)[pos * sizeof(char) * storage->component_size],
            &((char*)storage->data)[(storage->data_size - 1) * sizeof(char) * storage->component_size],
            storage->component_size);
    storage->data = realloc(storage->data, --storage->data_size * sizeof(char) * storage->component_size);
}

static void* storage_at(ecs_storage *storage, size_t pos) {
    assert(pos < storage->data_size);
    return &((char*)storage->data)[pos * sizeof(char) * storage->component_size];
}

static void* storage_get(ecs_storage *storage, entity_t entity) {
    assert(storage_has(storage, entity));
    return storage_at(storage, storage->sparse.entities[entity.id].id);
}

struct ecs {
    ecs_storage **storages;
    size_t storage_count;
    entity_t *entities;
    size_t entity_count;
    uint32_t *recyclable;
    size_t recyclable_count;
    uint32_t next_id;
};

world_t* ecs_create(void) {
    world_t *result = malloc(sizeof(world_t));
    memset(result, 0, sizeof(world_t));
    result->next_id = ecs_nil;
    return result;
}

void ecs_destroy(world_t*);
void ecs_step(world_t*);
entity_t ecs_spawn(world_t*);
entity_t ecs_delete(world_t*, entity_t);
bool ecs_isvalid(world_t*, entity_t);
bool ecs_isa(world_t*, entity_t a, ecs_entity_type_t type);
bool ecs_isnil(world_t*, entity_t a);
bool ecs_cmp(entity_t, entity_t);
void ecs_give(world_t*, entity_t entity, entity_t component);
void ecs_remove(world_t*, entity_t entity, entity_t component);
bool ecs_has(world_t*, entity_t entity, entity_t component);
void* ecs_get(world_t*, entity_t entity, entity_t component);
void ecs_set(world_t*, entity_t entity, entity_t component, const void *data);
void ecs_associate(world_t*, entity_t object, entity_t relation, entity_t relation_type);
void ecs_disassociate(world_t*, entity_t entity, entity_t relation_type);
bool ecs_has_relation(world_t*, entity_t entity, entity_t relation_type);
void ecs_relations(world_t*, entity_t entity, entity_t relation_type, system_cb_t);

void __ecs_query(world_t*, system_cb_t, int n, ...);
entity_t __ecs_new_component(world_t*, size_t);
entity_t __ecs_new_system(world_t*, system_cb_t, int, ...);
entity_t __ecs_new_prefab(world_t*, int, ...);
entity_t __ecs_new_relation(world_t*);

#endif // SECS_IMPLEMENTATION
