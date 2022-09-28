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

#define ECS_ID(ID) ECS__##ID
#define ECS_ENTITY(ID, VER, FLAG) \
    (((EcsEntity)(ID)) | (((EcsEntity)(VER)) << ENTITY_VERSION_SHIFT) | (((EcsEntity)(FLAG)) << ENTITY_FLAG_SHIFT))
#define ENTITY_VERSION(E) \
    (EcsEntityID)(((EcsEntity)(E)) >> ENTITY_VERSION_SHIFT)
#define ENTITY_ID(E) ((EcsEntityID)((E) & ENTITY_ID_MASK))
#define ENTITY_FLAG(E) ((EcsEntityID)((E) >> ENTITY_FLAG_SHIFT))
#define ENTITY_HAS(E, FLAG) ((E) & ENTITY_##FLAG##_MASK)
#define ENTITY_ID_LO(E) ((EcsEntityID)(E))
#define ENTITY_ID_HI(E) ((EcsEntityID)((E) >> ENTITY_VERSION_SHIFT))
#define ECS_PAIR_COMPOSE(A, B) (ENTITY_PAIR_MASK | ((EcsEntity)ENTITY_ID((B)) << 32 | (EcsEntity)ENTITY_ID((A))))
#define ECS_PAIR(world, A, B) 
#define IS_ENTITY_PAIR(E) ((E & ~ENTITY_PAIR_MASK)) 
#define PAIR_FIRST(E) (ENTITY_ID_LO((E)))
#define PAIR_RELATION PAIR_FIRST
#define PAIR_SECOND(E) ((EcsEntityID)ENTITY_ID_HI((E & ~ENTITY_PAIR_MASK)))
#define PAIR_OBJECT PAIR_SECOND
#define PAIR_HAS_RELATION(E, REL) (ENTITY_HAS((E), PAIR) && (PAIR_RELATION((E)) == (REL)))

typedef uint64_t EcsEntity;
typedef uint32_t EcsEntityID;
extern const EcsEntity EcsNil;

typedef struct EcsStorage EcsStorage;
typedef struct {
    EcsStorage **storages;
    size_t sizeOfStorages;
    EcsEntity *entities;
    size_t sizeOfEntities;
    EcsEntity nextAvailableId;
} EcsWorld;

#if defined(__cplusplus)
extern "C" {
#endif

    EcsWorld *NewWorld(void);
    void DeleteWorld(EcsWorld **world);

#if defined(__cplusplus)
}
#endif
#endif // secs_h
