#include "secs.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(DEBUG)
#include <assert.h>
#include <stdio.h>
#else
#define assert(X)
#endif

#define SAFE_FREE(X)    \
    do                  \
    {                   \
        if ((X))        \
        {               \
            free((X));  \
            (X) = NULL; \
        }               \
    } while (0)

#define ECS_DEFINE_DTOR(T, ...) \
    void Delete##T(T **p)       \
    {                           \
        T *_p = *p;             \
        if (!p || !_p)          \
            return;             \
        __VA_ARGS__             \
        SAFE_FREE(_p);          \
        *p = NULL;              \
    }

const uint64_t EcsNil = 0xFFFFFFFFull;
const Entity EcsNilEntity = { .id = EcsNil };

static uint64_t MurmurHash3_x86_128(const void *key, const int len, uint32_t seed) {
#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))
#define FMIX32(h) h^=h>>16; h*=0x85ebca6b; h^=h>>13; h*=0xc2b2ae35; h^=h>>16;
    const uint8_t * data = (const uint8_t*)key;
    const int nblocks = len / 16;
    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;
    uint32_t c1 = 0x239b961b;
    uint32_t c2 = 0xab0e9789;
    uint32_t c3 = 0x38b34ae5;
    uint32_t c4 = 0xa1e38b93;
    const uint32_t * blocks = (const uint32_t *)(data + nblocks*16);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i*4+0];
        uint32_t k2 = blocks[i*4+1];
        uint32_t k3 = blocks[i*4+2];
        uint32_t k4 = blocks[i*4+3];
        k1 *= c1; k1  = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
        h1 = ROTL32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;
        k2 *= c2; k2  = ROTL32(k2,16); k2 *= c3; h2 ^= k2;
        h2 = ROTL32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;
        k3 *= c3; k3  = ROTL32(k3,17); k3 *= c4; h3 ^= k3;
        h3 = ROTL32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;
        k4 *= c4; k4  = ROTL32(k4,18); k4 *= c1; h4 ^= k4;
        h4 = ROTL32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
    }
    const uint8_t * tail = (const uint8_t*)(data + nblocks*16);
    uint32_t k1 = 0;
    uint32_t k2 = 0;
    uint32_t k3 = 0;
    uint32_t k4 = 0;
    switch(len & 15) {
    case 15: k4 ^= tail[14] << 16;
    case 14: k4 ^= tail[13] << 8;
    case 13: k4 ^= tail[12] << 0;
             k4 *= c4; k4  = ROTL32(k4,18); k4 *= c1; h4 ^= k4;
    case 12: k3 ^= tail[11] << 24;
    case 11: k3 ^= tail[10] << 16;
    case 10: k3 ^= tail[ 9] << 8;
    case  9: k3 ^= tail[ 8] << 0;
             k3 *= c3; k3  = ROTL32(k3,17); k3 *= c4; h3 ^= k3;
    case  8: k2 ^= tail[ 7] << 24;
    case  7: k2 ^= tail[ 6] << 16;
    case  6: k2 ^= tail[ 5] << 8;
    case  5: k2 ^= tail[ 4] << 0;
             k2 *= c2; k2  = ROTL32(k2,16); k2 *= c3; h2 ^= k2;
    case  4: k1 ^= tail[ 3] << 24;
    case  3: k1 ^= tail[ 2] << 16;
    case  2: k1 ^= tail[ 1] << 8;
    case  1: k1 ^= tail[ 0] << 0;
             k1 *= c1; k1  = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };
    h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    FMIX32(h1); FMIX32(h2); FMIX32(h3); FMIX32(h4);
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    char out[16];
    ((uint32_t*)out)[0] = h1;
    ((uint32_t*)out)[1] = h2;
    ((uint32_t*)out)[2] = h3;
    ((uint32_t*)out)[3] = h4;
    return *(uint64_t*)out;
}

typedef struct {
    uint64_t hash:48;
    uint64_t dib:16;
} Bucket;

static void* BucketItem(Bucket *bucket) {
    return ((char*)bucket)+sizeof(Bucket);
}

typedef uint64_t(*MapHashCb)(const void *item, uint32_t seed);
typedef bool(*MapCmpCb)(const void*, const void*);
typedef void(*MapFreeCb)(void*);
typedef bool(*MapIterCb)(void*);

typedef struct {
    uint32_t seed;
    
    MapHashCb hashCb;
    MapCmpCb compareCb;
    MapFreeCb freeCb;
    
    size_t bucketsz,
           nbuckets,
           count,
           mask,
           growat,
           shrinkat,
           elsize,
           cap;
    
    void *buckets,
         *spare,
         *edata;
} Map;

void InitMap(Map *map, MapHashCb hashCb, MapCmpCb compareCb, MapFreeCb freeCb, size_t sizeOfElements) {
    size_t bucketsz = sizeof(Bucket) + sizeOfElements;
    while (bucketsz & (sizeof(uintptr_t)-1))
        bucketsz++;
    memset(map, 0, sizeof(Map));
    
    map->elsize = sizeOfElements;
    map->bucketsz = bucketsz;
    map->spare = ((char*)map)+sizeof(Map);
    map->edata = (char*)map->spare+bucketsz;
    map->cap = map->nbuckets = 16;
    map->mask = map->nbuckets-1;
    
    map->buckets = malloc(map->bucketsz*map->nbuckets);
    memset(map->buckets, 0, map->bucketsz*map->nbuckets);
    map->growat = map->nbuckets*0.75;
    map->shrinkat = map->nbuckets*0.10;
    
    map->hashCb = hashCb;
    map->compareCb = compareCb;
    map->freeCb = freeCb;
    
    uint32_t buffer[17];
    uint32_t seed = (uint32_t)clock();
    for (uint32_t i = 0; i < 17; i++)
        buffer[i] = seed *= 0xac564b05 + 1;
    map->seed = buffer[0] = ROTL32(buffer[10], 13) + ROTL32(buffer[0], 9);
}

static Bucket* BucketAt(Map *map, size_t index) {
    return (Bucket*)(((char*)map->buckets)+(map->bucketsz*index));
}

static void ResizeMap(Map *map, size_t new_cap) {
    Map map2;
    InitMap(&map2, NULL, NULL, NULL, map->elsize);
        
    for (size_t i = 0; i < map->nbuckets; i++) {
        Bucket *entry = BucketAt(map, i);
        if (!entry->dib)
            continue;
        entry->dib = 1;
        size_t j = entry->hash & map2.mask;
        for (;;) {
            Bucket *bucket = BucketAt(&map2, j);
            if (bucket->dib == 0) {
                memcpy(bucket, entry, map->bucketsz);
                break;
            }
            if (bucket->dib < entry->dib) {
                memcpy(map2.spare, bucket, map->bucketsz);
                memcpy(bucket, entry, map->bucketsz);
                memcpy(entry, map2.spare, map->bucketsz);
            }
            j = (j + 1) & map2.mask;
            entry->dib += 1;
        }
    }
    
    free(map->buckets);
    map->buckets = map2.buckets;
    map->nbuckets = map2.nbuckets;
    map->mask = map2.mask;
    map->growat = map2.growat;
    map->shrinkat = map2.shrinkat;
}

static uint64_t MapHash(Map *map, const void *key) {
    return map->hashCb(key, map->seed) << 16 >> 16;
}

void* MapSet(Map *map, void *item) {
    if (map->count == map->growat)
        ResizeMap(map, map->nbuckets*2);
    
    Bucket *entry = map->edata;
    entry->hash = MapHash(map, item);
    entry->dib = 1;
    memcpy(BucketItem(entry), item, map->elsize);
    
    size_t i = entry->hash & map->mask;
    for (;;) {
        Bucket *bucket = BucketAt(map, i);
        if (!bucket->dib) {
            memcpy(bucket, entry, map->bucketsz);
            map->count++;
            return NULL;
        }
        if (entry->hash == bucket->hash && !map->compareCb(BucketItem(entry), BucketItem(bucket))) {
            memcpy(map->spare, BucketItem(bucket), map->elsize);
            memcpy(BucketItem(bucket), BucketItem(entry), map->elsize);
            return map->spare;
        }
        if (bucket->dib < entry->dib) {
            memcpy(map->spare, bucket, map->bucketsz);
            memcpy(bucket, entry, map->bucketsz);
            memcpy(entry, map->spare, map->bucketsz);
        }
        i = (i + 1) & map->mask;
        entry->dib += 1;
    }
}

void* MapGet(Map *map, const void *key) {
    uint64_t hash = MapHash(map, key);
    size_t i = hash & map->mask;
    for (;;) {
        Bucket *bucket = BucketAt(map, i);
        if (!bucket->dib)
            return NULL;
        if (bucket->hash == hash && !map->compareCb(key, BucketItem(bucket)))
            return BucketItem(bucket);
        i = (i + 1) & map->mask;
    }
}

void* MapDel(Map *map, const void *key) {
    uint64_t hash = MapHash(map, key);
    size_t i = hash & map->mask;
    for (;;) {
        Bucket *bucket = BucketAt(map, i);
        if (!bucket->dib)
            return NULL;
        
        if (bucket->hash == hash && !map->compareCb(key, BucketItem(bucket))) {
            memcpy(map->spare, BucketItem(bucket), map->elsize);
            bucket->dib = 0;
            for (;;) {
                Bucket *prev = bucket;
                i = (i + 1) & map->mask;
                bucket = BucketAt(map, i);
                if (bucket->dib <= 1) {
                    prev->dib = 0;
                    break;
                }
                memcpy(prev, bucket, map->bucketsz);
                prev->dib--;
            }
            map->count--;
            if (map->nbuckets > map->cap && map->count <= map->shrinkat)
                ResizeMap(map, map->nbuckets/2);
            return map->spare;
        }
        i = (i + 1) & map->mask;
    }
}

static void FreeMapItems(Map *map) {
    if (!map->freeCb)
        return;
    for (size_t i = 0; i < map->nbuckets; i++) {
        Bucket *bucket = BucketAt(map, i);
        if (bucket->dib)
            map->freeCb(BucketItem(bucket));
    }
}

void EmptyMap(Map *map) {
    map->count = 0;
    FreeMapItems(map);
    if (map->nbuckets != map->cap) {
        void *new_buckets = malloc(map->bucketsz*map->cap);
        if (new_buckets) {
            free(map->buckets);
            map->buckets = new_buckets;
        }
        map->nbuckets = map->cap;
    }
    memset(map->buckets, 0, map->bucketsz*map->nbuckets);
    map->mask = map->nbuckets-1;
    map->growat = map->nbuckets*0.75;
    map->shrinkat = map->nbuckets*0.10;
}

void DestroyMap(Map *map) {
    FreeMapItems(map);
    free(map->buckets);
}

void MapEach(Map *map, MapIterCb cb) {
    for (size_t i = 0; i < map->nbuckets; i++) {
        Bucket *bucket = BucketAt(map, i);
        if (!bucket->dib)
            continue;
        if (!cb(BucketItem(bucket)))
            return;
    }
}

typedef struct Archetype Archetype;

typedef struct {
  uint32_t capacity;
  uint32_t count;
  Entity *elements;
} Type;

typedef struct {
  uint32_t count;
  Entity components[];
} Signature;

typedef struct {
  Archetype *archetype;
  Signature *sig;
    SystemCb run;
} System;

typedef struct {
  Entity component;
  Archetype *archetype;
} Edge;

typedef struct {
  uint32_t capacity;
  uint32_t count;
  Edge *edges;
} EdgeList;

typedef struct {
  Archetype *archetype;
  uint32_t row;
} Record;

struct Archetype {
  uint32_t capacity;
  uint32_t count;
  Type *type;
  Entity *entity_ids;
  void **components;
  EdgeList *left_edges;
  EdgeList *right_edges;
};

typedef struct {
    Entity e;
    Record record;
} EntityIndexPair;

static uint64_t EntityIndexHash(const void *item, uint32_t seed) {
    return 0;
}

static bool EntityIndexCmp(const void *a, const void *b) {
    return false;
}

typedef struct {
    Entity e;
    size_t sizeOfComponent;
} ComponentIndexPair;

static uint64_t ComponentIndexHash(const void *item, uint32_t seed) {
    return 0;
}

static bool ComponentIndexCmp(const void *a, const void *b) {
    return false;
}

typedef struct {
    Entity e;
    size_t sizeOfComponent;
} SystemIndexPair;

static uint64_t SystemIndexHash(const void *item, uint32_t seed) {
    return 0;
}

static bool SystemIndexCmp(const void *a, const void *b) {
    return false;
}

typedef struct {
    Type *type;
    Archetype *archetype;
} TypeIndexPair;

static uint64_t TypeIndexHash(const void *item, uint32_t seed) {
    return 0;
}

static bool TypeIndexCmp(const void *a, const void *b) {
    return false;
}

static void TypeIndexFree(void *element) {
    
}

static Type* NewType(uint32_t capacity) {
    Type *result = malloc(sizeof(Type));
    result->capacity = capacity;
    result->count = 0;
    return result;
}

static EdgeList* NewEdgeList(void) {
    EdgeList* result = malloc(sizeof(EdgeList));
    return result;
}

static void ResizeArchetypeComponentArray(Archetype *archetype, Map *componentIndex, uint32_t capacity) {
    for (int i = 0; i < archetype->type->count; i++) {
        Entity e = archetype->type->elements[i];
        ComponentIndexPair *componentPair = MapGet(componentIndex, &(ComponentIndexPair) { .e = e });
        assert(!!componentPair);
        archetype->components[i] = realloc(archetype->components[i], componentPair->sizeOfComponent * capacity);
        archetype->capacity = capacity;
    }
}

static Archetype* NewArchetype(Type *type, Map *componentIndex, Map *typeIndex) {
    Archetype *result = malloc(sizeof(Archetype));
    result->capacity = MAX_ECS_COMPONENTS;
    result->count = 0;
    result->type = type;
    result->entity_ids = malloc(sizeof(Entity) * MAX_ECS_COMPONENTS);
    result->components = calloc(type->count, sizeof(void*));
    result->left_edges = NewEdgeList();
    result->right_edges = NewEdgeList();
    
    ResizeArchetypeComponentArray(result, componentIndex, MAX_ECS_COMPONENTS);
    MapSet(typeIndex, &(TypeIndexPair) {
        .type = type,
        .archetype = result
    });
    return result;
}

struct World {
    Map entityIndex,    // entity -> record
        componentIndex, // entity -> size_t
        systemIndex,    // entity -> system
        typeIndex;      // type   -> archetype
    Archetype *root;
    uint32_t *recyclable;
    size_t sizeOfRecyclable;
    uint32_t nextAvailableId;
};

World* EcsWorld(void) {
    World *result = malloc(sizeof(World));
    *result = (World){0};
    result->recyclable = NULL;
    result->nextAvailableId = EcsNil;
    
    InitMap(&result->entityIndex, EntityIndexHash, EntityIndexCmp, NULL, sizeof(EntityIndexPair));
    InitMap(&result->componentIndex, ComponentIndexHash, ComponentIndexCmp, NULL, sizeof(ComponentIndexPair));
    InitMap(&result->systemIndex, SystemIndexHash, SystemIndexCmp, NULL, sizeof(SystemIndexPair));
    InitMap(&result->typeIndex, TypeIndexHash, TypeIndexCmp, TypeIndexFree, sizeof(TypeIndexPair));
    
    Type *rootType = NewType(0);
    result->root = NewArchetype(rootType, &result->componentIndex, &result->typeIndex);
    
    return result;
}

#define X(NAME, _) ECS_ID(NAME) = EcsNilEntity;
ECS_DEFINE_DTOR(World, {
    DestroyMap(&_p->entityIndex);
    DestroyMap(&_p->componentIndex);
    DestroyMap(&_p->systemIndex);
    DestroyMap(&_p->typeIndex);
    SAFE_FREE(_p->recyclable);
});
#undef X

bool EcsIsValid(World *world, Entity e) {
    assert(world);
    uint32_t id = ENTITY_ID(e);
    return id < world->sizeOfEntities && ECS_CMP(world->entities[id], e);
}

static Entity EcsNewEntityType(World *world, uint8_t type) {
    assert(world);
    if (world->sizeOfRecyclable) {
        uint32_t idx = world->recyclable[world->sizeOfRecyclable-1];
        Entity e = world->entities[idx];
        Entity new = ECS_COMPOSE_ENTITY(ENTITY_ID(e), ENTITY_VERSION(e), type);
        world->entities[idx] = new;
        world->recyclable = realloc(world->recyclable, --world->sizeOfRecyclable * sizeof(uint32_t));
        return new;
    } else {
        world->entities = realloc(world->entities, ++world->sizeOfEntities * sizeof(Entity));
        Entity e = ECS_COMPOSE_ENTITY((uint32_t)world->sizeOfEntities-1, 0, type);
        world->entities[world->sizeOfEntities-1] = e;
        return e;
    }
}

Entity EcsNewEntity(World *world) {
    return EcsNewEntityType(world, EcsEntityType);
}

static EcsStorage* EcsFind(World *world, Entity e) {
    for (int i = 0; i < world->sizeOfStorages; i++)
        if (ENTITY_ID(world->storages[i]->componentId) == ENTITY_ID(e))
            return world->storages[i];
    return NULL;
}

static EcsStorage* EcsAssure(World *world, Entity componentId, size_t sizeOfComponent) {
    EcsStorage *found = EcsFind(world, componentId);
    if (found)
        return found;
    EcsStorage *new = NewStorage(componentId, sizeOfComponent);
    world->storages = realloc(world->storages, (world->sizeOfStorages + 1) * sizeof * world->storages);
    world->storages[world->sizeOfStorages++] = new;
    return new;
}

bool EcsHas(World *world, Entity entity, Entity component) {
    assert(world);
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, component));
    return StorageHas(EcsFind(world, component), entity);
}

Entity EcsNewComponent(World *world, size_t sizeOfComponent) {
    Entity e = EcsNewEntityType(world, EcsComponentType);
    return EcsAssure(world, e, sizeOfComponent) ? e : EcsNilEntity;
}

Entity EcsNewSystem(World *world, SystemCb fn, Entity *components, size_t sizeOfComponents) {
    Entity e = EcsNewEntityType(world, EcsSystemType);
    EcsAttach(world, e, EcsSystem);
    System *c = EcsGet(world, e, EcsSystem);
    c->callback = fn;
    c->sizeOfComponents = sizeOfComponents;
    for (int i = 0; i < MAX_ECS_COMPONENTS; i++)
        c->components[i].parts.id = EcsNil;
    memcpy(c->components, components, sizeOfComponents * sizeof(Entity));
    return e;
}

Entity EcsNewPrefab(World *world, Entity *components, size_t sizeOfComponents) {
    Entity e = EcsNewEntityType(world, EcsPrefabType);
    EcsAttach(world, e, EcsPrefab);
    Prefab *c = EcsGet(world, e, EcsPrefab);
    for (int i = 0; i < MAX_ECS_COMPONENTS; i++)
        (*c)[i] = EcsNilEntity;
    memcpy(c, components, sizeOfComponents * sizeof(Entity));
    return e;
}

void DeleteEntity(World *world, Entity e) {
    assert(world);
    assert(EcsIsValid(world, e));
    for (size_t i = world->sizeOfStorages; i; --i)
        if (world->storages[i - 1] && SparseHas(world->storages[i - 1]->sparse, e))
            StorageRemove(world->storages[i - 1], e);
    uint32_t id = ENTITY_ID(e);
    world->entities[id] = ECS_COMPOSE_ENTITY(id, ENTITY_VERSION(e) + 1, 0);
    world->recyclable = realloc(world->recyclable, ++world->sizeOfRecyclable * sizeof(uint32_t));
    world->recyclable[world->sizeOfRecyclable-1] = id;
}

void EcsAttach(World *world, Entity entity, Entity component) {
    switch (component.parts.flag) {
        case EcsPairType: {
            Entity o = EcsPairObject(world, component);
            Entity r = EcsPairRelation(world, component);
            EcsStorage *storage = EcsFind(world, o);
            break;
        }
        case EcsSystemType:
            assert(false); // NOTE: potentially could be used for some sort of event system
        case EcsPrefabType: {
            Prefab *c = EcsGet(world, component, EcsPrefab);
            for (int i = 0; i < MAX_ECS_COMPONENTS; i++) {
                if (ECS_ENTITY_IS_NIL((*c)[i]))
                    break;
                EcsAttach(world, entity, (*c)[i]);
            }
            break;
        }
        case EcsComponentType:
        default: {
            assert(EcsIsValid(world, entity));
            assert(EcsIsValid(world, component));
            EcsStorage *storage = EcsFind(world, component);
            assert(storage);
            StorageEmplace(storage, entity);
            break;
        }
    }
}

void EcsDetach(World *world, Entity entity, Entity component) {
    assert(world);
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, component));
    EcsStorage *storage = EcsFind(world, component);
    assert(storage);
    assert(StorageHas(storage, entity));
    StorageRemove(storage, entity);
}

void* EcsGet(World *world, Entity entity, Entity component) {
    assert(world);
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, component));
    EcsStorage *storage = EcsFind(world, component);
    assert(storage);
    return StorageHas(storage, entity) ? StorageGet(storage, entity) : NULL;
}

void EcsSet(World *world, Entity entity, Entity component, const void *data) {
    assert(world);
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, component));
    EcsStorage *storage = EcsFind(world, component);
    assert(storage);
    
    void *componentData = StorageHas(storage, entity) ?
                                    StorageGet(storage, entity) :
                                    StorageEmplace(storage, entity);
    assert(componentData);
    memcpy(componentData, data, storage->sizeOfComponent);
}

Entity EcsNewPair(World *world, Entity a, Entity b) {
    assert(world);
    assert(EcsIsValid(world, a));
    assert(ECS_ENTITY_ISA(a, Component));
    assert(EcsIsValid(world, b));
    return ECS_PAIR_ENTITY(ECS_PAIR(a, b));
}

Entity EcsPairObject(World *world, Entity pair) {
    assert(world);
    assert(ECS_ENTITY_ISA(pair, Pair));
    EntityPair p = ECS_ENTITY_PAIR(pair);
    Entity obj = world->entities[p.parts.object];
    assert(EcsIsValid(world, obj));
    return obj;
}

Entity EcsPairRelation(World *world, Entity pair) {
    assert(world);
    assert(ECS_ENTITY_ISA(pair, Pair));
    EntityPair p = ECS_ENTITY_PAIR(pair);
    Entity rel = world->entities[p.parts.relation];
    assert(EcsIsValid(world, rel));
    return rel;
}

void EcsStep(World *world) {
    EcsStorage *storage = world->storages[ENTITY_ID(EcsSystem)];
    for (int i = 0; i < storage->sparse->sizeOfDense; i++) {
        System *system = StorageGet(storage, storage->sparse->dense[i]);
        EcsQuery(world, system->callback, system->components, system->sizeOfComponents);
    }
}

void EcsQuery(World *world, SystemCb cb, Entity *components, size_t sizeOfComponents) {
    assert(sizeOfComponents < MAX_ECS_COMPONENTS);
    for (size_t e = 0; e < world->sizeOfEntities; e++) {
        assert(EcsIsValid(world, world->entities[e]));
        bool hasComponents = true;
        View view = { .entityId = world->entities[e] };
        for (int i = 0; i < MAX_ECS_COMPONENTS; i++) {
            view.componentIndex[i].parts.id = EcsNil;
            view.componentData[i] = NULL;
        }
        
        for (size_t i = 0; i < sizeOfComponents; i++) {
            EcsStorage *storage = EcsFind(world, components[i]);
            
            if (StorageHas(storage, world->entities[e])) {
                view.componentIndex[i] = components[i];
                view.componentData[i] = StorageGet(storage, world->entities[e]);
            } else {
                hasComponents = false;
                break;
            }
        }
        
        if (hasComponents)
            cb(&view);
    }
}

void* EcsViewField(View *view, size_t index) {
    return index >= MAX_ECS_COMPONENTS || ECS_ENTITY_IS_NIL(view->componentIndex[index]) ? NULL : view->componentData[index];
}
