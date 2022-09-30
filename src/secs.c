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
    void Delete##T(Ecs##T **T)  \
    {                           \
        Ecs##T *_##T = *T;      \
        if (!T || !_##T)        \
            return;             \
        __VA_ARGS__;            \
        SAFE_FREE(_##T);        \
        *T = NULL;              \
    }

const EcsEntity EcsNil = (EcsEntity)ENTITY_ID_MASK;

typedef struct {
    EcsEntity *sparse;
    EcsEntity *dense;
    size_t sizeOfSparse;
    size_t sizeOfDense;
} EcsSparse;

typedef struct {
    EcsEntity componentId;
    void *data;
    size_t sizeOfData;
    size_t sizeOfComponent;
    EcsSparse *sparse;
} EcsStorage;

#if defined(DEBUG)
static void DumpSparse(EcsSparse *sparse) {
    printf("*** DUMP SPARSE ***\n");
    printf("sizeOfSparse: %zu, sizeOfDense: %zu\n", sparse->sizeOfSparse, sparse->sizeOfDense);
    printf("Sparse Contents:\n\t[");
    for (int i = 0; i < sparse->sizeOfSparse; i++)
        printf("%llu, ", sparse->sparse[i]);
    printf("]\nDense Contents:\n\t[");
    for (int i = 0; i < sparse->sizeOfDense; i++)
        printf("%llu, ", sparse->dense[i]);
    printf("]\n*** END SPARSE DUMP ***\n");
}

static void DumpStorage(EcsStorage *storage) {
    printf("*** DUMP STORAGE ***\n");
    printf("componentId: %llu, sizeOfData: %zu, sizeOfComponent: %zu\n",
           storage->componentId, storage->sizeOfData, storage->sizeOfComponent);
    DumpSparse(storage->sparse);
    printf("*** END STORAGE DUMP ***\n");
}
#endif

static EcsSparse* NewSparse(void) {
    EcsSparse *result = malloc(sizeof(EcsSparse));
    *result = (EcsSparse){0};
    return result;
}

static ECS_DEFINE_DTOR(Sparse, {
    SAFE_FREE(_Sparse->sparse);
    SAFE_FREE(_Sparse->dense);
});

static bool SparseHas(EcsSparse *sparse, EcsEntity e) {
    assert(sparse);
    assert(e != EcsNil);
    const EcsEntity id = ENTITY_ID(e);
    return (id < sparse->sizeOfSparse) && (sparse->sparse[id] != EcsNil);
}

static void SparseEmplace(EcsSparse *sparse, EcsEntity e) {
    assert(sparse);
    assert(e != EcsNil);
    const EcsEntity id = ENTITY_ID(e);
    if (id >= sparse->sizeOfSparse) {
        const size_t newSize = id + 1;
        sparse->sparse = realloc(sparse->sparse, newSize * sizeof * sparse->sparse);
        for (size_t i = sparse->sizeOfSparse; i < newSize; i++)
            sparse->sparse[i] = EcsNil;
        sparse->sizeOfSparse = newSize;
    }
    sparse->sparse[id] = (EcsEntity)sparse->sizeOfDense;
    sparse->dense = realloc(sparse->dense, (sparse->sizeOfDense + 1) * sizeof * sparse->dense);
    sparse->dense[sparse->sizeOfDense++] = e;
}

static size_t SparseRemove(EcsSparse *sparse, EcsEntity e) {
    assert(sparse);
    assert(SparseHas(sparse, e));
    
    const EcsEntity id = ENTITY_ID(e);
    size_t pos = sparse->sparse[id];
    EcsEntity other = sparse->dense[sparse->sizeOfDense-1];
    
    sparse->sparse[ENTITY_ID(other)] = (EcsEntity)pos;
    sparse->dense[pos] = other;
    sparse->sparse[id] = EcsNil;
    sparse->dense = realloc(sparse->dense, --sparse->sizeOfDense * sizeof * sparse->dense);
    
    return pos;
}

static size_t SparseAt(EcsSparse *sparse, EcsEntity e) {
    assert(sparse);
    assert(e != EcsNil);
    return sparse->sparse[ENTITY_ID(e)];
}

static EcsStorage* NewStorage(EcsEntity id, size_t sz) {
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

static ECS_DEFINE_DTOR(Storage, {
    DeleteSparse(&_Storage->sparse);
});

static bool StorageHas(EcsStorage *storage, EcsEntity e) {
    assert(storage);
    assert(e != EcsNil);
    return SparseHas(storage->sparse, e);
}

static void* StorageEmplace(EcsStorage *storage, EcsEntity e) {
    assert(storage);
    storage->data = realloc(storage->data, (storage->sizeOfData + 1) * sizeof(char) * storage->sizeOfComponent);
    storage->sizeOfData++;
    void *result = &((char*)storage->data)[(storage->sizeOfData - 1) * sizeof(char) * storage->sizeOfComponent];
    SparseEmplace(storage->sparse, e);
    return result;
}

static void StorageRemove(EcsStorage *storage, EcsEntity e) {
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

static void* StorageGet(EcsStorage *storage, EcsEntity e) {
    assert(storage);
    assert(e != EcsNil);
    return StorageAt(storage, SparseAt(storage->sparse, e));
}

struct EcsWorld {
    EcsStorage **storages;
    size_t sizeOfStorages;
    EcsEntity *entityIndex;
    size_t sizeOfEntities;
    EcsEntity nextAvailableId;
    EcsSparse *componentIndex;
};

EcsWorld* NewWorld(void) {
    EcsWorld *result = malloc(sizeof(EcsWorld));
    *result = (EcsWorld){0};
    result->nextAvailableId = EcsNil;
    return result;
}

ECS_DEFINE_DTOR(World, {
    if (_World->storages)
        for (int i = 0; i < _World->sizeOfStorages; i++)
            DeleteStorage(&_World->storages[i]);
    SAFE_FREE(_World->storages);
    SAFE_FREE(_World->entityIndex);
    SAFE_FREE(_World->componentIndex);
});

EcsEntity NewEntity(EcsWorld *world) {
    assert(world);
    if (world->nextAvailableId == EcsNil) {
        assert(world->sizeOfEntities < ENTITY_ID_MASK);
        world->entityIndex = realloc(world->entityIndex, ++world->sizeOfEntities * sizeof(EcsEntity));
        EcsEntity e = ECS_ENTITY(world->sizeOfEntities-1, 0, 0);
        world->entityIndex[world->sizeOfEntities-1] = e;
        return e;
    } else {
        EcsEntity id = ENTITY_ID(world->nextAvailableId);
        world->nextAvailableId = ENTITY_ID(world->entityIndex[id]);
        EcsEntity new = ECS_ENTITY(id, ENTITY_VERSION(world->entityIndex[id]), 0);
        world->entityIndex[id] = new;
        return new;
    }
}

static EcsStorage* EcsAssure(EcsWorld *world, EcsEntity componentId, size_t sizeOfComponent) {
    for (int i = 0; i < world->sizeOfStorages; i++)
        if (world->storages[i]->componentId == componentId)
            return world->storages[i];
    EcsStorage *new = NewStorage(componentId, sizeOfComponent);
    world->storages = realloc(world->storages, (world->sizeOfStorages + 1) * sizeof * world->storages);
    world->storages[world->sizeOfStorages++] = new;
    return new;
}

static EcsStorage* EcsFind(EcsWorld *world, EcsEntity e) {
    for (int i = 0; i < world->sizeOfStorages; i++)
        if (world->storages[i]->componentId == e)
            return world->storages[i];
    return NULL;
}

static bool EcsIsValid(EcsWorld *world, EcsEntity e) {
    assert(world);
    return ENTITY_ID(e) < world->sizeOfEntities && world->entityIndex[ENTITY_ID(e)] == e;
}

bool EcsHas(EcsWorld *world, EcsEntity entity, EcsEntity component) {
    assert(world);
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, component));
    return StorageHas(EcsFind(world, component), entity);
}

EcsEntity NewComponent(EcsWorld *world, size_t sizeOfComponent) {
    EcsEntity result = NewEntity(world);
    return EcsAssure(world, result, sizeOfComponent) ? result : EcsNil;
}

EcsEntity NewSystem(EcsWorld *world) {
    return EcsNil;
}

void DeleteEntity(EcsWorld *world, EcsEntity e) {
    assert(world);
    assert(EcsIsValid(world, e));
    for (size_t i = world->sizeOfStorages; i; --i)
        if (world->storages[i - 1] && SparseHas(world->storages[i - 1]->sparse, e))
            StorageRemove(world->storages[i - 1], e);
    world->entityIndex[ENTITY_ID(e)] = ECS_ENTITY(ENTITY_ID(e), ENTITY_VERSION(e) + 1, 0);
    world->nextAvailableId = ENTITY_ID(e);
}

void EcsAttach(EcsWorld *world, EcsEntity entity, EcsEntity component) {
    EcsStorage *storage = NULL;
    for (int i = 0; i < world->sizeOfStorages; i++)
        if (world->storages[i]->componentId == component) {
            storage = world->storages[i]; ;
            break;
        }
    assert(storage);
    StorageEmplace(storage, entity);
}

void EcsDetach(EcsWorld *world, EcsEntity entity, EcsEntity component) {
    assert(world);
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, component));
    EcsStorage *storage = EcsFind(world, component);
    assert(storage);
    assert(StorageHas(storage, entity));
    StorageRemove(storage, entity);
}

void* EcsGet(EcsWorld *world, EcsEntity entity, EcsEntity component) {
    assert(world);
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, component));
    EcsStorage *storage = EcsFind(world, component);
    assert(storage);
    return StorageHas(storage, entity) ? StorageGet(storage, entity) : NULL;
}

void EcsSet(EcsWorld *world, EcsEntity entity, EcsEntity component, const void *data) {
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

void EcsStep(EcsWorld *world) {
    
}

void* EcsField(EcsView view, EcsEntity component) {
    return NULL;
}
