//
//  components.c
//  secs
//
//  Created by George Watson on 19/11/2022.
//

#include "test.h"

typedef struct {
    float x, y;
} Position;

typedef struct {
    float x, y;
} Velocity;

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
    
    DeleteWorld(&world);
    return EXIT_SUCCESS;
}
