//
//  systems.c
//  x
//
//  Created by George Watson on 19/11/2022.
//

#include "test.h"

typedef struct {
    int x, y;
} Position;

static world_t* world;
static entity_t position_component;

void PositionSystem(entity_t e) {
    Position *p = entity_get(world, e, position_component);
    p->x += 10;
    p->y += 20;
}

int main(int argc, const char *argv[]) {
    world = ecs_world();
    entity_t e = ecs_spawn(world);
    position_component = ecs_component(world, sizeof(Position));
    Position *p1 = entity_give(world, e, position_component);
    p1->x = 0;
    p1->y = 0;
    entity_t s1 = ecs_system(world, PositionSystem, 1, position_component);
    OK(entity_isa(world, s1, ECS_SYSTEM), true);
    for (int i = 0; i < 10; i++)
        ecs_step(world);
    Position *p2 = entity_get(world, e, position_component);
    OK(p2->x == 100, true);
    OK(p2->y == 200, true);
    ecs_world_destroy(&world);
    return 0;
}
