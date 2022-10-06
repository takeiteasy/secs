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
#include <stddef.h>
#include <stdarg.h>

/*!
 * @union Entity
 * @abstract An entity represents a general-purpose object
 * @field parts.id 32-bit unique identifier
 * @field parts.version Entity generation
 * @field parts.unsed 16-bit unused
 * @field parts.flag Entity type flag
 * @field id Full entity identifier
 */
typedef union {
    struct {
        uint32_t id;
        uint16_t version;
        uint8_t unused;
        uint8_t flag;
    } parts;
    uint64_t id;
} Entity;

/*!
 * @defined ECS_COMPOSE_ENTITY
 * @abstract Compose an entity from given parts
 * @param ID 32-bit unique identifier
 * @param VER Entity generation
 * @param TAG Entity type flag
 * @return New Entity object
 */
#define ECS_COMPOSE_ENTITY(ID, VER, TAG) \
    (Entity)                             \
    {                                    \
        .parts = {                       \
            .id = ID,                    \
            .version = VER,              \
            .flag = TAG                  \
        }                                \
    }
/*!
 * @defined ENTITY_ID
 * @abstract Get Entity unique 32-bit id
 * @param E Entity object
 * @return 32-bit unique identifier
 */
#define ENTITY_ID(E) \
    ((E).parts.id)
/*!
 * @defined ENTITY_VERSION
 * @abstract Get Entity object generation
 * @param E Entity object
 * @return Entity generation
 */
#define ENTITY_VERSION(E) \
    ((E).parts.version)
/*!
 * @defined ENTITY_IS_NIL
 * @abstract Check if given entity object is nil
 * @param E Entity object
 * @return boolean
 */
#define ENTITY_IS_NIL(E) \
    ((E).parts.id == EcsNil)
/*!
 * @defined ENTITY_CMP
 * @abstract Compare to entity objects
 * @param A Entity object 1
 * @param B Entity object 2
 * @return boolean
 */
#define ENTITY_CMP(A, B) \
    ((A).id == (B).id)

/*!
 * @const EcsNil
 * @abstract Constatnt 32-bit denoting a nil entity
 */
extern const uint64_t EcsNil;
/*!
 * @const EcsNilEntity
 * @abstract Constatnt Entity object denoting a nil entity
 */
extern const Entity EcsNilEntity;
/*!
 * @typedef World
 * @abstract ECS Manager, holds all ECS data
 */
typedef struct World World;

/*!
 * @struct Component
 * @abstract A component labels an entity as possessing a particular aspect, and holds the data needed to model that aspect
 * @field componentId Unique component entity object
 * @field sizeOfComponent Size of component data
 * @field name Component's name string
 */
typedef struct {
    Entity componentId;
    size_t sizeOfComponent;
    const char *name;
} Component;

#define ECS_ID(T) Ecs##T
#define ECS_DECLARE(T) Entity ECS_ID(T)
/*!
 * @defined ECS_COMPONENT
 * @abstract Convenience wrapper around EcsNewComponent
 * @param WORLD ECS World instance
 * @param T Component type
 * @return Component entity object
 */
#define ECS_COMPONENT(WORLD, T) EcsNewComponent(WORLD, sizeof(T))
/*!
 * @defined ECS_TAG
 * @abstract Convenience wrapper around EcsNewComponent (size param is zero)
 * @param WORLD ECS World instance
 * @return Component entity object
 */
#define ECS_TAG(WORLD) EcsNewComponent(WORLD, 0)
/*!
 * @defined ECS_QUERY
 * @abstract Convenience wrapper around EcsQuery
 * @param WORLD ECS World instance
 * @param CB Query callback
 * @param ... List of components for query
 */
#define ECS_QUERY(WORLD, CB, ...)                                      \
    do                                                                 \
    {                                                                  \
        Entity components[] = {__VA_ARGS__};                           \
        size_t sizeOfComponents = sizeof(components) / sizeof(Entity); \
        assert(sizeOfComponents < MAX_ECS_COMPONENTS);                 \
        EcsQuery(WORLD, CB, components, sizeOfComponents);             \
    } while (0)
/*!
 * @defined ECS_FIELD
 * @abstract Convenience wrapper around EcsViewField (For use in EcsQuery callbacks)
 * @param VIEW View object
 * @param T Component type
 * @param IDX View component index
 * @return Pointer to component data
 */
#define ECS_FIELD(VIEW, T, IDX) (T *)EcsViewField(VIEW, IDX)
/*!
 * @defined ECS_SYSTEM
 * @abstract Convenience wrapper around EcsNewSystem
 * @param WORLD Ecs World object
 * @param CB System callback
 * @param ... List of components
 * @return System entity object
 */
#define ECS_SYSTEM(WORLD, CB, ...) EcsNewSystem(WORLD, CB, (Entity[]){__VA_ARGS__}, sizeof((Entity[]){__VA_ARGS__}) / sizeof(Entity))
/*!
 * @defined ECS_PREFAB
 * @abstract Convenience wrapper around EcsNewPrefab
 * @param WORLD ECS World instance
 * @param ... List of components
 * @return Prefab entity object
 */
#define ECS_PREFAB(WORLD, ...) EcsNewPrefab(WORLD, (Entity[]){__VA_ARGS__}, sizeof((Entity[]){__VA_ARGS__}) / sizeof(Entity))
/*!
 * @defined ENTITY_ISA
 * @abstract Check the type of an entity
 * @param E Entity object to check
 * @param TYPE EntityFlag to compare
 * @return boolean
 */
#define ENTITY_ISA(E, TYPE) ((E).parts.flag == Ecs##TYPE##Type)

/*!
 * @defined MAX_ECS_COMPONENTS
 * @internal
 */
#define MAX_ECS_COMPONENTS 16

/*!
 * @struct View
 * @abstract Structure to hold all components for an entity (used for queries)
 * @field componentData Array of component data
 * @field componentIndex Indexes for componentData
 * @field entityId Entity ID components belong to
 */
typedef struct {
    void *componentData[MAX_ECS_COMPONENTS];
    Entity componentIndex[MAX_ECS_COMPONENTS];
    Entity entityId;
} View;

/*!
 * @typedef SystemCb
 * @abstract Typedef for System callbacks
 */
typedef void(*SystemCb)(View*);
/*!
 * @typedef Prefab
 * @abstract Prefab component type (Array of component IDs)
 */
typedef Entity Prefab[MAX_ECS_COMPONENTS];
/*!
 * @struct System
 * @abstract System component type
 * @field callback Address to system callback
 * @field components Array of component IDs
 * @field sizeOfComponents Size of component data
 */
typedef struct {
    SystemCb callback;
    Prefab components;
    size_t sizeOfComponents;
} System;
/*!
 * @struct Relation
 * @abstract Entity pair component type
 * @field object Type of relationship
 * @field relation Entity relation
 */
typedef struct {
    Entity object,
           relation;
} Relation;

#define ECS_BOOTSTRAP             \
    X(System, sizeof(System))     \
    X(Prefab, sizeof(Prefab))     \
    X(Relation, sizeof(Relation)) \
    X(Childof, 0)

#define X(NAME, _) extern Entity ECS_ID(NAME);
ECS_BOOTSTRAP
#undef X

/*!
 * @enum EntityFlag
 * @abstract Entity type ID flags
 * @constant EcsEntityType Default type
 * @constant EcsComponentType Component type
 * @constant EcsSystemType System type
 * @constant EcsPrefabType Prefab type
 * @constant EcsRelationType Relation type
 */
typedef enum {
    EcsEntityType    = 0,
    EcsComponentType = (1 << 0),
    EcsSystemType    = (1 << 1),
    EcsPrefabType    = (1 << 2),
    EcsRelationType  = (1 << 3)
} EntityFlag;

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @function EcsWorld
 * @abstract Create a new ECS instance
 * @return ECS World instance
 */
World* EcsWorld(void);
/*!
 * @function DeleteWorld
 * @param world ECS World instance
 * @abstract Destroy ECS instance
 */
void DeleteWorld(World **world);

/*!
 * @function EcsNewEntity
 * @abstract Register a new Entity in the ECS
 * @param world ECS World instance
 * @return Entity object
 */
Entity EcsNewEntity(World *world);
/*!
 * @function EcsNewComponent
 * @abstract Register a new component in the ECS
 * @param world ECS World instance
 * @param sizeOfComponent Size of component data
 * @return Component Entity object
 */
Entity EcsNewComponent(World *world, size_t sizeOfComponent);
/*!
 * @function EcsNewSystem
 * @abstract Register a new system in the ECS
 * @param world ECS World instance
 * @param fn System callback function
 * @param components Array of component IDs
 * @param sizeOfComponents Length of component array
 * @return System Entity object
 */
Entity EcsNewSystem(World *world, SystemCb fn, Entity *components, size_t sizeOfComponents);
/*!
 * @function EcsNewPrefab
 * @abstract Register a new prefab in the ECS
 * @param world ECS World instance
 * @param components Array of component IDs
 * @param sizeOfComponents Length of component array
 * @return Prefab Entity object
 */
Entity EcsNewPrefab(World *world, Entity *components, size_t sizeOfComponents);
/*!
 * @function DeleteEntity
 * @abstract Remove an Entity from the ECS (Increase the Entity generation in entity index)
 * @param world ECS World instance
 * @param entity Entity to remove
 */
void DeleteEntity(World *world, Entity entity);

/*!
 * @function EcsIsValid
 * @abstract Check if Entity ID is in range and generation corrosponds with entity generation in the index
 * @param world ECS World instance
 * @param entity Entity to check
 * @param boolean
 */
bool EcsIsValid(World *world, Entity entity);
/*!
 * @function EcsHas
 * @abstract Check if an Entity has component registered to it
 * @param world ECS World instance
 * @param entity Entity to check
 * @param component Component ID to check
 * @param return boolean
 */
bool EcsHas(World *world, Entity entity, Entity component);
/*!
 * @function EcsAttach
 * @abstract Attach a component to an entity
 * @param world ECS World instance
 * @param entity Entity to attach to
 * @param component Component ID to attach
 */
void EcsAttach(World *world, Entity entity, Entity component);
/*!
 * @function EcsAssociate
 * @abstract Attach a relation between two entities
 * @param world ECS World instance
 * @param entity Entity to attach relation to
 * @param object Type of relation (Must be a registered component/tag)
 * @param relation Entity ID to relate to
 */
void EcsAssociate(World *world, Entity entity, Entity object, Entity relation);
/*!
 * @function EcsDetach
 * @abstract Remove a component from an entity
 * @param world ECS World instance
 * @param entity Entity to modify
 * @param component Component to remove
 */
void EcsDetach(World *world, Entity entity, Entity component);
/*!
 * @function EcsDisassociate
 * @abstract Remove a relation from an entity
 * @param world ECS World instance
 * @param entity Entity to modify
 */
void EcsDisassociate(World *world, Entity entity);
/*!
 * @function EcsHasRelation
 * @abstract Check if an entity has a relation
 * @param world ECS World instance
 * @param entity Entity to check
 * @param object Type of relation to check (Must be a registered component/tag)
 * @return boolean
 */
bool EcsHasRelation(World *world, Entity entity, Entity object);
/*!
 * @function EcsRelated
 * @abstract Check if two entities are related (in any way)
 * @param world ECS World instance
 * @param entity Entity with suspected relation
 * @param relation Potentially related entity
 * @return boolean
 */
bool EcsRelated(World *world, Entity entity, Entity relation);
/*!
 * @function EcsGet
 * @abstract Get component data from an entity
 * @param world ECS World instance
 * @param entity Entity object
 * @param component Component ID for desired data
 * @return Pointer to component data
 */
void* EcsGet(World *world, Entity entity, Entity component);
/*!
 * @function EcsSet
 * @abstract Set the component data for an entity
 * @param world ECS World instance
 * @param entity Entity to modify
 * @param component Component ID for desired data
 * @param data Reference to data to set
 */
void EcsSet(World *world, Entity entity, Entity component, const void *data);
/*!
 * @function EcsRelations
 * @abstract Query all entities with specified relation
 * @param world ECS World instance
 * @param entity Parent entity
 * @param relation Type of relation (Must be a registered component/tag)
 * @param cb Query callback
 */
void EcsRelations(World *world, Entity entity, Entity relation, SystemCb cb);
/*!
 * @defined ECS_CHILDREN
 * @abstract Convenience wrapper for EcsRelation to find all entities with relation of EcsChildOf
 * @param WORLD ECS World instance
 * @param PARENT Parent entity
 * @param CB Query callback
 */
#define ECS_CHILDREN(WORLD, PARENT, CB) (EcsRelations((WORLD), (PARENT), EcsChildof, (CB)))

/*!
 * @function EcsStep
 * @abstract Progress ECS (run registered systems)
 * @param world ECS World intance
 */
void EcsStep(World *world);
/*!
 * @function EcsQuery
 * @abstract Query all entities with given components
 * @param world ECS World instance
 * @param cb Query callback
 * @param components Array of component IDs
 * @param sizeOfComponents Length of component array
 */
void EcsQuery(World *world, SystemCb cb, Entity *components, size_t sizeOfComponents);
/*!
 * @function EcsViewField
 * @abstract Retrieve a component from View object at specified index (For use in EcsQuery callbacks)
 * @param view View object
 * @param index Component index
 * @return Pointer to component data
 */
void* EcsViewField(View *view, size_t index);

#if defined(__cplusplus)
}
#endif
#endif // secs_h
