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

#define ENTITY_ID_MASK          0xFFFFFFFFull /* Mask to use to get the entity number out of an identifier.*/
#define ENTITY_VERSION_SHIFT    32 /* Extent of the entity number within an identifier. */
#define ENTITY_VERSION_MASK     (0xFFFFull << ENTITY_VERSION_SHIFT) /* Mask to use to get the version out of an identifier. */
#define ENTITY_FLAG_SHIFT       56
#define ENTITY_FLAG_MASK        (0xFFull << ENTITY_FLAG_SHIFT)
#define ENTITY_PAIR_SHIFT       63
#define ENTITY_PAIR_MASK        (1ull << ENTITY_PAIR_SHIFT)

#define ECS_ENTITY(ID, VER, FLAG) \
    (((EcsEntity)(ID)) | (((EcsEntity)(VER)) << ENTITY_VERSION_SHIFT) | (((EcsEntity)(FLAG)) << ENTITY_FLAG_SHIFT))
#define ENTITY_VERSION(E) \
    (EcsEntityID)(((EcsEntity)(E)) >> ENTITY_VERSION_SHIFT)
#define ENTITY_ID(E) ((EcsEntityID)((E)&ENTITY_ID_MASK))
#define ENTITY_FLAG(E) ((EcsEntityID)((E) >> ENTITY_FLAG_SHIFT))
#define ENTITY_HAS(E, FLAG) ((E)&ENTITY_##FLAG##_MASK)
#define ENTITY_ID_LO(E) ((EcsEntityID)(E))
#define ENTITY_ID_HI(E) ((EcsEntityID)((E) >> ENTITY_VERSION_SHIFT))
#define ECS_PAIR(A, B) (ENTITY_PAIR_MASK | ((EcsEntity)ENTITY_ID((B)) << 32 | (EcsEntity)ENTITY_ID((A))))
#define IS_ENTITY_PAIR(E) ((E & ~ENTITY_PAIR_MASK))
#define PAIR_FIRST(E) (ENTITY_ID_LO((E)))
#define PAIR_RELATION PAIR_FIRST
#define PAIR_SECOND(E) ((EcsEntityID)ENTITY_ID_HI((E & ~ENTITY_PAIR_MASK)))
#define PAIR_OBJECT PAIR_SECOND
#define PAIR_HAS_RELATION(E, REL) (ENTITY_HAS((E), PAIR) && (PAIR_RELATION((E)) == (REL)))

typedef uint64_t EcsEntity;
typedef uint32_t EcsEntityID;
extern const EcsEntity EcsNil;
typedef struct EcsWorld EcsWorld;

typedef struct {
    EcsEntity componentId;
    size_t sizeOfComponent;
    const char *name;
} EcsComponent;

#define MAX_ECS_VIEW_COMPONENTS 16

typedef struct {
    void *componentData[MAX_ECS_VIEW_COMPONENTS];
    size_t componentIndex[MAX_ECS_VIEW_COMPONENTS];
    EcsEntity entityId;
} EcsView;
typedef void(*EcsSystemFn)(EcsView*);

#define ECS_ID(T) Ecs__##T
#define ECS_DECLARE(T) EcsEntity ECS_ID(T)
#define ECS_COMPONENT(WORLD, T) NewComponent(WORLD, sizeof(T))
#define ECS_TAG(WORLD) NewComponent(WORLD, 0)
#define ECS_QUERY(WORLD, CB, ...) do { \
    EcsEntity components[] = { __VA_ARGS__ }; \
    size_t sizeOfComponents = sizeof(components) / sizeof(EcsEntity); \
    assert(sizeOfComponents < MAX_ECS_VIEW_COMPONENTS); \
    EcsQuery(WORLD, CB, components, sizeOfComponents); \
} while(0);
#define ECS_FIELD(VIEW, T, IDX) (T*)EcsField(VIEW, IDX)
#define ECS_SYSTEM(WORLD, CB, ...) NewSystem(WORLD, CB, sizeof((uint64_t[]) {__VA_ARGS__}) / sizeof(uint64_t), __VA_ARGS__)

#if defined(__cplusplus)
extern "C" {
#endif

EcsWorld* NewWorld(void);
void DeleteWorld(EcsWorld **world);
EcsEntity NewEntity(EcsWorld *world);
EcsEntity NewComponent(EcsWorld *world, size_t sizeOfComponent);
EcsEntity NewSystem(EcsWorld *world, EcsSystemFn fn, size_t n, ...);
void DeleteEntity(EcsWorld *world, EcsEntity entity);
bool EcsHas(EcsWorld *world, EcsEntity entity, EcsEntity component);
void EcsAttach(EcsWorld *world, EcsEntity entity, EcsEntity component);
void EcsDetach(EcsWorld *world, EcsEntity entity, EcsEntity component);
void* EcsGet(EcsWorld *world, EcsEntity entity, EcsEntity component);
void EcsSet(EcsWorld *world, EcsEntity entity, EcsEntity component, const void *data);
void EcsStep(EcsWorld *world);
void EcsQuery(EcsWorld *world, EcsSystemFn cb, EcsEntity *components, size_t sizeOfComponents);
void* EcsField(EcsView *view, size_t index);

#if defined(__cplusplus)
}
#endif
#endif // secs_h
