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
        uint16_t unused;
        uint8_t  flags;
    } parts;
    uint64_t id;
} Entity;

typedef union {
    struct {
        uint32_t lo;
        uint32_t hi;
    } parts;
    uint64_t id;
} Pair;

#define ECS_ENTITY(ID, VER, TAG) (Entity) { \
    .parts = { \
        .id = ID, \
        .version = VER, \
        .flags = TAG \
    } \
}
#define ECS_PAIR(A, B) (Pair) { \
    .parts = { \
        .lo = A.parts.id, \
        .hi = B.parts.id \
    } \
}
#define ENTITY_ID(E) ((E).parts.id)
#define ENTITY_VERSION(E) ((E).parts.version)

extern const uint64_t EcsNil;
typedef struct World World;

typedef struct {
    Entity componentId;
    size_t sizeOfComponent;
    const char *name;
} Component;

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

#define ECS_ID(T) Ecs##T
#define ECS_DECLARE(T) Entity ECS_ID(T)
#define ECS_COMPONENT(WORLD, T) EcsComponent(WORLD, sizeof(T))
#define ECS_TAG(WORLD) EcsComponent(WORLD, 0)
#define ECS_QUERY(WORLD, CB, ...) do { \
    Entity components[] = { __VA_ARGS__ }; \
    size_t sizeOfComponents = sizeof(components) / sizeof(Entity); \
    assert(sizeOfComponents < MAX_ECS_COMPONENTS); \
    EcsQuery(WORLD, CB, components, sizeOfComponents); \
} while(0);
#define ECS_FIELD(VIEW, T, IDX) (T*)EcsField(VIEW, IDX)
#define ECS_SYSTEM(WORLD, CB, ...) EcsSystem(WORLD, CB, (Entity[]){ __VA_ARGS__ }, sizeof((Entity[]){ __VA_ARGS__ }) / sizeof(Entity))
#define ECS_PREFAB(WORLD, ...) EcsPrefab(WORLD, (Entity[]){ __VA_ARGS__ }, sizeof((Entity[]){ __VA_ARGS__ }) / sizeof(Entity))

#if defined(__cplusplus)
extern "C" {
#endif

World* EcsWorld(void);
void DeleteWorld(World **world);
Entity EcsEntity(World *world);
Entity EcsComponent(World *world, size_t sizeOfComponent);
Entity EcsSystem(World *world, SystemCb fn, Entity *components, size_t sizeOfComponents);
Entity EcsPrefab(World *world, Entity *components, size_t sizeOfComponents);
void DeleteEntity(World *world, Entity entity);
bool EcsHas(World *world, Entity entity, Entity component);
void EcsAttach(World *world, Entity entity, Entity component);
void EcsDetach(World *world, Entity entity, Entity component);
void* EcsGet(World *world, Entity entity, Entity component);
void EcsSet(World *world, Entity entity, Entity component, const void *data);
void EcsStep(World *world);
void EcsQuery(World *world, SystemCb cb, Entity *components, size_t sizeOfComponents);
void* EcsField(View *view, size_t index);

#if defined(__cplusplus)
}
#endif
#endif // secs_h
