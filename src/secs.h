#ifndef secs_h
#define secs_h
#if defined(_MSC_VER)
#if !defined(bool)
#define bool int
#endif
#if !defined(true)
#define true 1
#endif
#if !defined(false)
#define false 0
#endif
#else
#include <stdbool.h>
#endif
#include <stdint.h>
#include <stdarg.h>

typedef union {
    struct {
        uint32_t id;
        uint16_t version;
        uint8_t unused;
        uint8_t flag;
    } parts;
    uint64_t id;
} Entity;

#define ECS_COMPOSE_ENTITY(ID, VER, TAG) \
    (Entity)                             \
    {                                    \
        .parts = {                       \
            .id = ID,                    \
            .version = VER,              \
            .flag = TAG                  \
        }                                \
    }
#define ENTITY_ID(E) ((E).parts.id)
#define ENTITY_VERSION(E) ((E).parts.version)
#define ECS_ENTITY_IS_NIL(E) ((E).parts.id == EcsNil)
#define ECS_CMP(A, B) ((A).id == (B).id)

extern const uint64_t EcsNil;
extern const Entity EcsNilEntity;
typedef struct World World;

typedef struct {
    Entity componentId;
    size_t sizeOfComponent;
    const char *name;
} Component;

#define ECS_ID(T) Ecs##T
#define ECS_DECLARE(T) Entity ECS_ID(T)
#define ECS_COMPONENT(WORLD, T) EcsNewComponent(WORLD, sizeof(T))
#define ECS_TAG(WORLD) EcsNewComponent(WORLD, 0)
#define ECS_QUERY(WORLD, CB, ...)                                      \
    do                                                                 \
    {                                                                  \
        Entity components[] = {__VA_ARGS__};                           \
        size_t sizeOfComponents = sizeof(components) / sizeof(Entity); \
        assert(sizeOfComponents < MAX_ECS_COMPONENTS);                 \
        EcsQuery(WORLD, CB, components, sizeOfComponents);             \
    } while (0)
#define ECS_FIELD(VIEW, T, IDX) (T *)EcsViewField(VIEW, IDX)
#define ECS_SYSTEM(WORLD, CB, ...) EcsNewSystem(WORLD, CB, (Entity[]){__VA_ARGS__}, sizeof((Entity[]){__VA_ARGS__}) / sizeof(Entity))
#define ECS_PREFAB(WORLD, ...) EcsNewPrefab(WORLD, (Entity[]){__VA_ARGS__}, sizeof((Entity[]){__VA_ARGS__}) / sizeof(Entity))
#define ECS_ENTITY_ISA(E, TYPE) ((E).parts.flag == Ecs##TYPE##Type)

#define MAX_ECS_COMPONENTS 16

typedef struct {
    void *componentData[MAX_ECS_COMPONENTS];
    Entity componentIndex[MAX_ECS_COMPONENTS];
    Entity entityId;
} View;

typedef void(*SystemCb)(View*);
typedef Entity Prefab[MAX_ECS_COMPONENTS];
typedef struct {
    SystemCb callback;
    Prefab components;
    size_t sizeOfComponents;
} System;
typedef struct {
    Entity object, relation;
} Relation;

#define ECS_BOOTSTRAP             \
    X(System, sizeof(System))     \
    X(Prefab, sizeof(Prefab))     \
    X(Relation, sizeof(Relation)) \
    X(Childof, 0)

#define X(NAME, _) extern Entity ECS_ID(NAME);
ECS_BOOTSTRAP
#undef X

typedef enum {
    EcsEntityType    = 0,
    EcsComponentType = (1 << 0),
    EcsSystemType    = (1 << 1),
    EcsPrefabType    = (1 << 2),
    EcsPairType      = (1 << 3)
} EntityFlags;

#if defined(__cplusplus)
extern "C" {
#endif

World* EcsWorld(void);
void DeleteWorld(World **world);

Entity EcsNewEntity(World *world);
Entity EcsNewComponent(World *world, size_t sizeOfComponent);
Entity EcsNewSystem(World *world, SystemCb fn, Entity *components, size_t sizeOfComponents);
Entity EcsNewPrefab(World *world, Entity *components, size_t sizeOfComponents);
void DeleteEntity(World *world, Entity entity);

bool EcsIsValid(World *world, Entity e);
bool EcsHas(World *world, Entity entity, Entity component);
void EcsAttach(World *world, Entity entity, Entity component);
void EcsAssociate(World *world, Entity entity, Entity object, Entity relation);
void EcsDetach(World *world, Entity entity, Entity component);
void EcsDisassociate(World *world, Entity entity);
bool EcsHasRelation(World *world, Entity entity, Entity object);
bool EcsRelated(World *world, Entity entity, Entity relation);
void* EcsGet(World *world, Entity entity, Entity component);
void EcsSet(World *world, Entity entity, Entity component, const void *data);
void EcsRelations(World *world, Entity entity, Entity relation, SystemCb cb);
#define ECS_CHILDREN(WORLD, PARENT, CB) (EcsRelations((WORLD), (PARENT), EcsChildof, (CB)))

void EcsStep(World *world);
void EcsQuery(World *world, SystemCb cb, Entity *components, size_t sizeOfComponents);
void* EcsViewField(View *view, size_t index);

#if defined(__cplusplus)
}
#endif
#endif // secs_h
