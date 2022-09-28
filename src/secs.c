#include "secs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define SAFE_FREE(X) \
    do { \
        if ((X)) { \
            free((X)); \
            (X) = NULL; \
        } \
    } while(0)

#define ECS_DEFINE_DTOR(T, ...) \
    void Delete##T(Ecs##T **T) { \
        Ecs##T *_##T = *T; \
        if (!T || !_##T) \
            return; \
        SAFE_FREE(_##T); \
        *T = NULL;\
    }

const EcsEntity EcsNil = (EcsEntity)ENTITY_ID_MASK;

typedef struct {
    EcsEntity *sparse;
    EcsEntity *dense;
    size_t sizeOfSparse;
    size_t sizeOfDense;
} EcsSparse;

struct EcsStorage {
    EcsEntity componentId;
    void *data;
    size_t sizeOfData;
    size_t sizeOfComponent;
    EcsSparse *sparse;
};

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
        size_t newSparseSize = sparse->sizeOfSparse + 1;
        sparse->sparse = realloc(sparse->sparse, newSparseSize * sizeof * sparse->sparse);
        memset(sparse->sparse + sparse->sizeOfSparse, EcsNil, (newSparseSize - sparse->sizeOfSparse) * sizeof * sparse->sparse);
        sparse->sizeOfSparse = newSparseSize;
    }
    
    sparse->sparse[id] = (EcsEntity)sparse->sizeOfDense;
    sparse->dense = realloc(sparse->dense, ++sparse->sizeOfDense * sizeof * sparse->dense);
    sparse->dense[sparse->sizeOfDense] = e;
}

static size_t SparseRemove(EcsSparse *sparse, EcsEntity e) {
    assert(sparse);
    assert(SparseHas(sparse, e));
    
    const EcsEntity id = ENTITY_ID(e);
    size_t pos = sparse->sparse[id];
    EcsEntity other = sparse->dense[sparse->sizeOfDense-1];
    
    sparse->sparse[id] = (EcsEntity)pos;
    sparse->dense[pos] = other;
    sparse->sparse[pos] = EcsNil;
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
    DeleteSparse(&_storage->sparse);
});

static bool StorageHas(EcsStorage *storage, EcsEntity e) {
    assert(storage);
    assert(e != EcsNil);
    return SparseHas(storage->sparse, e);
}

static void* StorageEmplace(EcsStorage *storage, EcsEntity e) {
    assert(storage);
    storage->data = realloc(storage->data, ++storage->sizeOfData * sizeof(char) * storage->sizeOfComponent);
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
    SAFE_FREE(_World->entities);
});
