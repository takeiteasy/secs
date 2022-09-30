#include <stdio.h>
#include <assert.h>
#include "secs.h"

static int TEST_COUNTER = 1;
#if !defined(TEST_DIE_ON_FAILED)
#define TEST_DIE_ON_FAILED true
#endif
#define TEST(OBJECT, RESULT)                                                                              \
    do                                                                                                    \
    {                                                                                                     \
        bool res = (OBJECT) == (RESULT);                                                                  \
        printf("[TEST%02d:%s] %s == %s\n", TEST_COUNTER++, res ? "SUCCESS" : "FAILED", #OBJECT, #RESULT); \
        assert(!TEST_DIE_ON_FAILED || res);                                                               \
    } while (0)

typedef struct {
    float x, y;
} Position;

typedef struct {
    float x, y;
} Velocity;

static int posCbCounter = 0;
static void posCb(EcsView *view) {
    Position *position = ECS_FIELD(view, Position, 0);
    printf("posCb: ENTITY#%llu POS: x:%f,y:%f\n", view->entityId, position->x, position->y);
    posCbCounter++;
}

static int moveCbCounter = 0;
static void moveCb(EcsView *view) {
    Position *position = ECS_FIELD(view, Position, 0);
    Velocity *velocity = ECS_FIELD(view, Velocity, 1);
    printf("moveCb: ENTITY#%llu POS: x:%f,y:%f, VEL: x:%f, y:%f\n", view->entityId, position->x, position->y, velocity->x, velocity->y);
    moveCbCounter++;
}

int main(int argc, char *argv[]) {
    EcsWorld *world = NewWorld();
    
    EcsEntity e1 = NewEntity(world);
    EcsEntity position = ECS_COMPONENT(world, Position);
    EcsAttach(world, e1, position);
    EcsEntity e2 = NewEntity(world);
    EcsAttach(world, e2, position);
    EcsEntity e3 = NewEntity(world);
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

    EcsEntity testTag = ECS_TAG(world);
    EcsAttach(world, e1, testTag);
    TEST(EcsHas(world, e1, testTag), true);
    TEST(EcsHas(world, e2, testTag), false);
    
    EcsEntity velocity = ECS_COMPONENT(world, Velocity);
    EcsAttach(world, e2, position);
    ECS_QUERY(world, posCb, position);
    EcsAttach(world, e1, velocity);
    ECS_QUERY(world, moveCb, position, velocity);
    TEST(posCbCounter, 2);
    TEST(moveCbCounter, 1);
    
    EcsEntity e4 = NewEntity(world);
    EcsAttach(world, e4, position);
    EcsAttach(world, e4, velocity);
    EcsEntity testSystem1 = ECS_SYSTEM(world, posCb, position);
    EcsEntity testSystem2 = ECS_SYSTEM(world, moveCb, position, velocity);
    
    EcsStep(world);
    TEST(posCbCounter, 5);
    TEST(moveCbCounter, 3);
    
    
    DeleteWorld(&world);
    return 0;
}
