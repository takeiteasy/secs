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
const Entity EcsNilEntity = { .id = EcsNil };

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
static void DumpEntitySimple(Entity e) {
    printf("(%llx: %d, %d, %d)\n", e.id, e.parts.id, e.parts.version, e.parts.flag);
}

static void DumpSparse(EcsSparse *sparse) {
    printf("*** DUMP SPARSE ***\n");
    printf("sizeOfSparse: %zu, sizeOfDense: %zu\n", sparse->sizeOfSparse, sparse->sizeOfDense);
    printf("Sparse Contents:\n");
    for (int i = 0; i < sparse->sizeOfSparse; i++)
        DumpEntitySimple(sparse->sparse[i]);
    printf("Dense Contents:\n");
    for (int i = 0; i < sparse->sizeOfDense; i++)
        DumpEntitySimple(sparse->dense[i]);
    printf("*** END SPARSE DUMP ***\n");
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
    assert(!ENTITY_IS_NIL(e));
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
    assert(!ENTITY_IS_NIL(e));
    return StorageAt(storage, SparseAt(storage->sparse, e));
}

struct World {
    EcsStorage **storages;
    size_t sizeOfStorages;
    Entity *entities;
    size_t sizeOfEntities;
    uint32_t *recyclable;
    size_t sizeOfRecyclable;
    uint32_t nextAvailableId;
};

#define X(NAME, _) Entity ECS_ID(NAME) = (Entity) { .id = -1 };
ECS_BOOTSTRAP
#undef X

World* EcsWorld(void) {
    World *result = malloc(sizeof(World));
    *result = (World){0};
    result->nextAvailableId = EcsNil;
    
#define X(NAME, SZ) ECS_ID(NAME) = EcsNewComponent(result, SZ);
    ECS_BOOTSTRAP
#undef X
    
    return result;
}

#define X(NAME, _) ECS_ID(NAME) = EcsNilEntity;
ECS_DEFINE_DTOR(World, {
    if (_p->storages)
        for (int i = 0; i < _p->sizeOfStorages; i++)
            DeleteEcsStorage(&_p->storages[i]);
    SAFE_FREE(_p->storages);
    SAFE_FREE(_p->entities);
    SAFE_FREE(_p->recyclable);
    ECS_BOOTSTRAP
});
#undef X

bool EcsIsValid(World *world, Entity e) {
    assert(world);
    uint32_t id = ENTITY_ID(e);
    return id < world->sizeOfEntities && ENTITY_CMP(world->entities[id], e);
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
        case EcsRelationType: // Use EcsRelation()
        case EcsSystemType: // NOTE: potentially could be used for some sort of event system
            assert(false);
        case EcsPrefabType: {
            Prefab *c = EcsGet(world, component, EcsPrefab);
            for (int i = 0; i < MAX_ECS_COMPONENTS; i++) {
                if (ENTITY_IS_NIL((*c)[i]))
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

void EcsAssociate(World *world, Entity entity, Entity object, Entity relation) {
    assert(world);
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, object));
    assert(ENTITY_ISA(object, Component));
    assert(EcsIsValid(world, relation));
    assert(ENTITY_ISA(relation, Entity));
    EcsAttach(world, entity, EcsRelation);
    Relation *pair = EcsGet(world, entity, EcsRelation);
    pair->object = object;
    pair->relation = relation;
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

void EcsDisassociate(World *world, Entity entity) {
    assert(EcsIsValid(world, entity));
    assert(EcsHas(world, entity, EcsRelation));
    EcsDetach(world, entity, EcsRelation);
}

bool EcsHasRelation(World *world, Entity entity, Entity object) {
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, object));
    EcsStorage *storage = EcsFind(world, EcsRelation);
    if (!storage)
        return false;
    Relation *relation = StorageGet(storage, entity);
    if (!relation)
        return false;
    return ENTITY_CMP(relation->object, object);
}

bool EcsRelated(World *world, Entity entity, Entity relation) {
    assert(EcsIsValid(world, entity));
    assert(EcsIsValid(world, relation));
    EcsStorage *storage = EcsFind(world, EcsRelation);
    if (!storage)
        return false;
    Relation *_relation = StorageGet(storage, entity);
    if (!_relation)
        return false;
    return ENTITY_CMP(_relation->relation, relation);
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

void EcsRelations(World *world, Entity entity, Entity relation, SystemCb cb) {
    EcsStorage *pairs = EcsFind(world, EcsRelation);
    for (size_t i = 0; i < world->sizeOfEntities; i++) {
        Entity e = world->entities[i];
        if (StorageHas(pairs, e)) {
            Relation *pair = StorageGet(pairs, e);
            if (ENTITY_CMP(pair->object, relation) && ENTITY_CMP(pair->relation, entity)) {
                View view = { .entityId = e };
                view.componentIndex[0] = relation;
                view.componentData[0] = (void*)pair;
                cb(&view);
            }
        }
    }
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
    return index >= MAX_ECS_COMPONENTS || ENTITY_IS_NIL(view->componentIndex[index]) ? NULL : view->componentData[index];
}
