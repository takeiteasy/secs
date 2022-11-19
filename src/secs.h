/* secs.h -- https://github.com/takeiteasy/secs
 
 The MIT License (MIT)

 Copyright (c) 2022 George Watson

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef secs_h
#define secs_h

/*!
 * @header secs Simple Entity Component System
 * @copyright George Watson
 */

#if defined(_MSC_VER) && _MSC_VER < 1800
#include <windef.h>
#define bool BOOL
#define true 1
#define false 0
#else
#if defined(__STDC__) && __STDC_VERSION__ < 199901L
typedef enum bool { false = 0, true = !false } bool;
#else
#include <stdbool.h>
#endif
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Taken from: https://gist.github.com/61131/7a22ac46062ee292c2c8bd6d883d28de
#define N_ARGS(...) _NARG_(__VA_ARGS__, _RSEQ())
#define _NARG_(...) _SEQ(__VA_ARGS__)
#define _SEQ(_1, _2, _3, _4, _5, _6, _7, _8, _9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64,_65,_66,_67,_68,_69,_70,_71,_72,_73,_74,_75,_76,_77,_78,_79,_80,_81,_82,_83,_84,_85,_86,_87,_88,_89,_90,_91,_92,_93,_94,_95,_96,_97,_98,_99,_100,_101,_102,_103,_104,_105,_106,_107,_108,_109,_110,_111,_112,_113,_114,_115,_116,_117,_118,_119,_120,_121,_122,_123,_124,_125,_126,_127,N,...) N
#define _RSEQ() 127,126,125,124,123,122,121,120,119,118,117,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

/*!
 * @union Entity
 * @abstract An entity represents a general-purpose object
 * @field parts Struct containing entity information
 * @field id 32-bit unique identifier
 * @field version Entity generation
 * @field unused 16-bit unused
 * @field flag Entity type flag
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
#define ECS_QUERY(WORLD, CB, ...) EcsQuery(WORLD, CB, (Entity[]){__VA_ARGS__}, sizeof((Entity[]){__VA_ARGS__}) / sizeof(Entity));
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
#define ECS_SYSTEM(WORLD, CB, ...) EcsNewSystem(WORLD, CB, N_ARGS(__VA_ARGS__), __VA_ARGS__)
/*!
 * @defined ECS_PREFAB
 * @abstract Convenience wrapper around EcsNewPrefab
 * @param WORLD ECS World instance
 * @param ... List of components
 * @return Prefab entity object
 */
#define ECS_PREFAB(WORLD, ...) EcsNewPrefab(WORLD, N_ARGS(__VA_ARGS__), __VA_ARGS__)
/*!
 * @defined ECS_CHILDREN
 * @abstract Convenience wrapper for EcsRelation to find all entities with relation of EcsChildOf
 * @param WORLD ECS World instance
 * @param PARENT Parent entity
 * @param CB Query callback
 */
#define ECS_CHILDREN(WORLD, PARENT, CB) (EcsRelations((WORLD), (PARENT), EcsChildof, (CB)))
/*!
 * @defined ENTITY_ISA
 * @abstract Check the type of an entity
 * @param E Entity object to check
 * @param TYPE EntityFlag to compare
 * @return boolean
 */
#define ENTITY_ISA(E, TYPE) ((E).parts.flag == Ecs##TYPE##Type)

/*!
 * @struct View
 * @abstract Structure to hold all components for an entity (used for queries)
 * @field componentData Array of component data
 * @field componentIndex Indexes for componentData
 * @field sizeOfComponentData Size of component data and index arrays
 * @field entityId Entity ID components belong to
 */
typedef struct {
    void **componentData;
    Entity *componentIndex;
    size_t sizeOfComponentData;
    Entity entityId;
} View;

/*!
 * @typedef SystemCb
 * @abstract Typedef for System callbacks
 * @param view Struct containing component data
 */
typedef void(*SystemCb)(View* view);
/*!
 * @struct Prefab
 * @abstract Prefab component type (Array of component IDs)
 * @field components Array of component IDs
 * @field sizeOfComponents Size of component data
 */
typedef struct {
    Entity *components;
    size_t sizeOfComponents;
} Prefab;
/*!
 * @struct System
 * @abstract System component type
 * @field callback Address to system callback
 * @field components Array of component IDs
 * @field sizeOfComponents Size of component data
 */
typedef struct {
    SystemCb callback;
    Entity *components;
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
 * @param sizeOfComponents Length of component array
 * @param ... Component IDs
 * @return System Entity object
 */
Entity EcsNewSystem(World *world, SystemCb fn, size_t sizeOfComponents, ...);
/*!
 * @function EcsNewPrefab
 * @abstract Register a new prefab in the ECS
 * @param world ECS World instance
 * @param sizeOfComponents Length of component array
 * @param ... Component IDs
 * @return Prefab Entity object
 */
Entity EcsNewPrefab(World *world, size_t sizeOfComponents, ...);
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
 * @return Boolean
 */
bool EcsIsValid(World *world, Entity entity);
/*!
 * @function EcsHas
 * @abstract Check if an Entity has component registered to it
 * @param world ECS World instance
 * @param entity Entity to check
 * @param component Component ID to check
 * @return Boolean
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
 * @return Boolean
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
