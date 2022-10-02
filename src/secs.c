#include "secs.h"
#include <stdlib.h>
#include <string.h>
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
static const Entity EcsNilEntity = { .parts = { .id = EcsNil } };
#define IS_NIL(E) ((E).parts.id == EcsNil)

typedef struct {
    Entity *sparse;
    Entity *dense;
    size_t sizeOfSparse;
    size_t sizeOfDense;
} EcsSparse;

typedef struct {
    Entity componentId;
    void *data;
    size_t sizeOfData;
    size_t sizeOfComponent;
    EcsSparse *sparse;
} EcsStorage;

#if defined(DEBUG)
static void DumpEntity(Entity e) {
    printf("(%llx: %d, %d, %d) ", e.id, e.parts.id, e.parts.version, e.parts.flags);
}

static void DumpSparse(EcsSparse *sparse) {
    printf("*** DUMP SPARSE ***\n");
    printf("sizeOfSparse: %zu, sizeOfDense: %zu\n", sparse->sizeOfSparse, sparse->sizeOfDense);
    printf("Sparse Contents:\n\t[");
    for (int i = 0; i < sparse->sizeOfSparse; i++)
        DumpEntity(sparse->sparse[i]);
    printf("]\nDense Contents:\n\t[");
    for (int i = 0; i < sparse->sizeOfDense; i++)
        DumpEntity(sparse->dense[i]);
    printf("]\n*** END SPARSE DUMP ***\n");
}

static void DumpStorage(EcsStorage *storage) {
    printf("*** DUMP STORAGE ***\n");
    printf("componentId: %u, sizeOfData: %zu, sizeOfComponent: %zu\n",
           storage->componentId.parts.id, storage->sizeOfData, storage->sizeOfComponent);
    DumpSparse(storage->sparse);
    printf("*** END STORAGE DUMP ***\n");
}
#endif

static EcsSparse* NewSparse(void) {
    EcsSparse *result = malloc(sizeof(EcsSparse));
    *result = (EcsSparse){0};
    return result;
}

static ECS_DEFINE_DTOR(EcsSparse, {
    SAFE_FREE(_p->sparse);
    SAFE_FREE(_p->dense);
});

static bool SparseHas(EcsSparse *sparse, Entity e) {
    assert(sparse);
    uint32_t id = ENTITY_ID(e);
    assert(id != EcsNil);
    return (id < sparse->sizeOfSparse) && (ENTITY_ID(sparse->sparse[id]) != EcsNil);
}

static void SparseEmplace(EcsSparse *sparse, Entity e) {
    assert(sparse);
    uint32_t id = ENTITY_ID(e);
    assert(id != EcsNil);
    if (id >= sparse->sizeOfSparse) {
        const size_t newSize = id + 1;
        sparse->sparse = realloc(sparse->sparse, newSize * sizeof * sparse->sparse);
        for (size_t i = sparse->sizeOfSparse; i < newSize; i++)
            sparse->sparse[i] = EcsNilEntity;
        sparse->sizeOfSparse = newSize;
    }
    sparse->sparse[id] = (Entity) { .parts = { .id = (uint32_t)sparse->sizeOfDense } };
    sparse->dense = realloc(sparse->dense, (sparse->sizeOfDense + 1) * sizeof * sparse->dense);
    sparse->dense[sparse->sizeOfDense++] = e;
}

static size_t SparseRemove(EcsSparse *sparse, Entity e) {
    assert(sparse);
    assert(SparseHas(sparse, e));
    
    const uint32_t id = ENTITY_ID(e);
    uint32_t pos = ENTITY_ID(sparse->sparse[id]);
    Entity other = sparse->dense[sparse->sizeOfDense-1];
    
    sparse->sparse[ENTITY_ID(other)] = (Entity) { .parts = { .id = pos } };
    sparse->dense[pos] = other;
    sparse->sparse[id] = EcsNilEntity;
    sparse->dense = realloc(sparse->dense, --sparse->sizeOfDense * sizeof * sparse->dense);
    
    return pos;
}

static size_t SparseAt(EcsSparse *sparse, Entity e) {
    assert(sparse);
    uint32_t id = ENTITY_ID(e);
    assert(id != EcsNil);
    return ENTITY_ID(sparse->sparse[id]);
}

static EcsStorage* NewStorage(Entity id, size_t sz) {
    EcsStorage *result = malloc(sizeof(EcsStorage));
    *result = (EcsStorage) {
        .componentId = id,
        .sizeOfComponent = sz,
        .sizeOfData = 0,
        .data = NULL,
        .sparse = NewSparse()
    };
    return result;
}

static ECS_DEFINE_DTOR(EcsStorage, {
    DeleteEcsSparse(&_p->sparse);
});

static bool StorageHas(EcsStorage *storage, Entity e) {
    assert(storage);
    assert(!IS_NIL(e));
    return SparseHas(storage->sparse, e);
}

static void* StorageEmplace(EcsStorage *storage, Entity e) {
    assert(storage);
    storage->data = realloc(storage->data, (storage->sizeOfData + 1) * sizeof(char) * storage->sizeOfComponent);
    storage->sizeOfData++;
    void *result = &((char*)storage->data)[(storage->sizeOfData - 1) * sizeof(char) * storage->sizeOfComponent];
    SparseEmplace(storage->sparse, e);
    return result;
}

static void StorageRemove(EcsStorage *storage, Entity e) {
    assert(storage);
    size_t pos = SparseRemove(storage->sparse, e);
    memmove(&((char*)storage->data)[pos * sizeof(char) * storage->sizeOfComponent],
            &((char*)storage->data)[(storage->sizeOfData - 1) * sizeof(char) * storage->sizeOfComponent],
            storage->sizeOfComponent);
    storage->data = realloc(storage->data, --storage->sizeOfData * sizeof(char) * storage->sizeOfComponent);
}

static void* StorageAt(EcsStorage *storage, size_t pos) {
    assert(storage);
    assert(pos < storage->sizeOfData);
    return &((char*)storage->data)[pos * sizeof(char) * storage->sizeOfComponent];
}

static void* StorageGet(EcsStorage *storage, Entity e) {
    assert(storage);
    assert(!IS_NIL(e));
    return StorageAt(storage, SparseAt(storage->sparse, e));
}

struct World {
    EcsStorage **storages;
    size_t sizeOfStorages;
    Entity *entities;
    size_t sizeOfEntities;
    uint32_t nextAvailableId;
};

World* EcsWorld(void) {
    World *result = malloc(sizeof(World));
    *result = (World){0};
    result->nextAvailableId = EcsNil;
    return result;
}

ECS_DEFINE_DTOR(World, {
    if (_p->storages)
        for (int i = 0; i < _p->sizeOfStorages; i++)
            DeleteEcsStorage(&_p->storages[i]);
    SAFE_FREE(_p->storages);
    SAFE_FREE(_p->entities);
});

Entity EcsEntity(World *world) {
    assert(world);
    if (world->nextAvailableId == EcsNil) {
        world->entities = realloc(world->entities, ++world->sizeOfEntities * sizeof(Entity));
        Entity e = ECS_ENTITY((uint32_t)world->sizeOfEntities-1, 0, 0);
        world->entities[world->sizeOfEntities-1] = e;
        return e;
    } else {
        uint32_t id = world->nextAvailableId;
        world->nextAvailableId = ENTITY_ID(world->entities[id]);
        Entity new = ECS_ENTITY(id, world->entities[id].parts.version, 0);
        world->entities[id] = new;
        return new;
    }
}

static EcsStorage* EcsAssure(World *world, Entity componentId, size_t sizeOfComponent) {
    for (int i = 0; i < world->sizeOfStorages; i++)
        if (ENTITY_ID(world->storages[i]->componentId) == ENTITY_ID(componentId))
            return world->storages[i];
    EcsStorage *new = NewStorage(componentId, sizeOfComponent);
    world->storages = realloc(world->storages, (world->sizeOfStorages + 1) * sizeof * world->storages);
    world->storages[world->sizeOfStorages++] = new;
    return new;
}

static EcsStorage* EcsFind(World *world, Entity e) {
    for (int i = 0; i < world->sizeOfStorages; i++)
        if (ENTITY_ID(world->storages[i]->componentId) == ENTITY_ID(e))
            return world->storages[i];
    return NULL;
}

static bool EcsIsValid(World *world, Entity e) {
    assert(world);
    uint32_t id = ENTITY_ID(e);
    return id < world->sizeOfEntities && world->entities[id].parts.id == id;
}

bool EcsHas(World *world, Entity entity, Entity component) {
    assert(world);
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, component));
    return StorageHas(EcsFind(world, component), entity);
}

Entity EcsComponent(World *world, size_t sizeOfComponent) {
    Entity e = EcsEntity(world);
    return EcsAssure(world, e, sizeOfComponent) ? e : EcsNilEntity;
}

Entity EcsSystem(World *world, SystemCb fn, Entity *components, size_t sizeOfComponents) {
    Entity e = EcsEntity(world);
//    EcsAttach(world, e, EcsSystem);
//    System *c = EcsGet(world, e, EcsSystem);
//    c->callback = fn;
//    c->sizeOfComponents = sizeOfComponents;
//    for (int i = 0; i < MAX_ECS_COMPONENTS; i++)
//        c->components[i].parts.id = EcsNil;
//    memcpy(c->components, components, sizeOfComponents * sizeof(Entity));
    return e;
}

Entity EcsPrefab(World *world, Entity *components, size_t sizeOfComponents) {
    Entity e = EcsEntity(world);
//    EcsAttach(world, e, EcsPrefab);
//    Prefab *c = EcsGet(world, e, EcsPrefab);
//    for (int i = 0; i < MAX_ECS_COMPONENTS; i++)
//        (*c)[i].parts.id = EcsNil;
//    memcpy(c, components, sizeOfComponents * sizeof(Entity));
    return e;
}

void DeleteEntity(World *world, Entity e) {
    assert(world);
    assert(EcsIsValid(world, e));
    for (size_t i = world->sizeOfStorages; i; --i)
        if (world->storages[i - 1] && SparseHas(world->storages[i - 1]->sparse, e))
            StorageRemove(world->storages[i - 1], e);
    uint32_t id = ENTITY_ID(e);
    world->entities[id] = ECS_ENTITY(id, ENTITY_VERSION(e) + 1, 0);
    world->nextAvailableId = id;
}

void EcsAttach(World *world, Entity entity, Entity component) {
    EcsStorage *storage = EcsFind(world, component);
    assert(storage);
    StorageEmplace(storage, entity);
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

void EcsStep(World *world) {
//    EcsStorage *storage = world->storages[EcsSystem];
//    for (int i = 0; i < storage->sparse->sizeOfDense; i++) {
//        System *system = StorageGet(storage, storage->sparse->dense[i]);
//        EcsQuery(world, system->callback, system->components, system->sizeOfComponents);
//    }
}

void EcsQuery(World *world, SystemCb cb, Entity *components, size_t sizeOfComponents) { // This needs optimizing
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

void* EcsField(View *view, size_t index) {
    return index >= MAX_ECS_COMPONENTS || IS_NIL(view->componentIndex[index]) ? NULL : view->componentData[index];
}
