#include <stdio.h>
#include <assert.h>
#include "secs.h"

static int TEST_COUNTER = 0;
#if !defined(TEST_DIE_ON_FAILED)
#define TEST_DIE_ON_FAILED true
#endif
#define TEST(OBJECT, RESULT)                                                                              \
    do                                                                                                    \
    {                                                                                                     \
        bool res = (OBJECT) == (RESULT);                                                                  \
        printf("[TEST%02d:%s] %s == %s\n", ++TEST_COUNTER, res ? "SUCCESS" : "FAILED", #OBJECT, #RESULT); \
        assert(!TEST_DIE_ON_FAILED || res);                                                               \
    } while (0)

typedef struct {
    float x, y;
} Position;

typedef struct {
    float x, y;
} Velocity;

static int posCbCounter = 0;
static void posCb(View *view) {
    TEST(!!ECS_FIELD(view, Position, 0), true);
    posCbCounter++;
}

static int moveCbCounter = 0;
static void moveCb(View *view) {
    TEST(!!ECS_FIELD(view, Position, 0), true);
    TEST(!!ECS_FIELD(view, Velocity, 1), true);
    moveCbCounter++;
}

static int childCbCounter = 0;
static void childCb(View *view) {
    TEST(!!ECS_FIELD(view, Relation, 0), true);
    childCbCounter++;
}

int main(int argc, char *argv[]) {
    World *world = EcsWorld();
    
    Entity e1 = EcsNewEntity(world);
    Entity position = ECS_COMPONENT(world, Position);
    EcsAttach(world, e1, position);
    Entity e2 = EcsNewEntity(world);
    EcsAttach(world, e2, position);
    Entity e3 = EcsNewEntity(world);
    TEST(EcsHas(world, e1, position), true);
    TEST(EcsHas(world, e2, position), true);
    TEST(EcsHas(world, e3, position), false);
    
    Position *p1 = EcsGet(world, e1, position);
    Position *p2 = EcsGet(world, e2, position);
    Position *p3 = EcsGet(world, e3, position);
    TEST(!!p1, true);
    TEST(!!p2, true);
    TEST(!!p3, false);
    
    EcsSet(world, e1, position, &(Position) { .x = 1.f, .y = 2.f });
    EcsSet(world, e2, position, &(Position) { .x = 3.f, .y = 4.f });
    TEST(p1->x, 1.f);
    TEST(p1->y, 2.f);
    TEST(p2->x, 3.f);
    TEST(p2->y, 4.f);
    
    EcsDetach(world, e2, position);
    Position *p1a = EcsGet(world, e1, position);
    TEST(EcsHas(world, e1, position), true);
    TEST(p1a->x, 1.f);
    TEST(p1a->y, 2.f);
    TEST(EcsHas(world, e2, position), false);
    TEST(EcsGet(world, e2, position), NULL);
    TEST(EcsHas(world, e3, position), false);
    TEST(EcsGet(world, e3, position), NULL);

    Entity testTag = ECS_TAG(world);
    EcsAttach(world, e1, testTag);
    TEST(EcsHas(world, e1, testTag), true);
    TEST(EcsHas(world, e2, testTag), false);
    
    Entity velocity = ECS_COMPONENT(world, Velocity);
    EcsAttach(world, e2, position);
    ECS_QUERY(world, posCb, position);
    EcsAttach(world, e1, velocity);
    ECS_QUERY(world, moveCb, position, velocity);
    TEST(posCbCounter, 2);
    TEST(moveCbCounter, 1);
    
    Entity e4 = EcsNewEntity(world);
    EcsAttach(world, e4, position);
    EcsAttach(world, e4, velocity);
    Entity testSystemA = ECS_SYSTEM(world, posCb, position);
    Entity testSystemB = ECS_SYSTEM(world, moveCb, position, velocity);
    
    EcsStep(world);
    TEST(posCbCounter, 5);
    TEST(moveCbCounter, 3);
    
    DeleteEntity(world, testSystemA);
    EcsStep(world);
    TEST(posCbCounter, moveCbCounter);
    
    Entity e5 = EcsNewEntity(world);
    Entity testPrefab = ECS_PREFAB(world, position, velocity);
    EcsAttach(world, e5, testPrefab);
    TEST(EcsHas(world, e5, position), true);
    TEST(EcsHas(world, e5, velocity), true);
    
    TEST(ENTITY_ISA(testPrefab, Prefab), true);
    TEST(ENTITY_ISA(testSystemB, System), true);
    TEST(ENTITY_ISA(position, Component), true);
    TEST(ENTITY_ISA(e5, Component), false);
    
    Entity parent = EcsNewEntity(world);
    Entity child = EcsNewEntity(world);
    EcsAssociate(world, child, EcsChildof, parent);
    EcsAssociate(world, e1, EcsChildof, parent);
    ECS_CHILDREN(world, parent, childCb);
    TEST(childCbCounter, 2);
    TEST(EcsHasRelation(world, child, EcsChildof), true);
    TEST(EcsRelated(world, child, parent), true);
    EcsDisassociate(world, e1);
    ECS_CHILDREN(world, parent, childCb);
    TEST(childCbCounter, 3);
    
    DeleteWorld(&world);
    return 0;
}
