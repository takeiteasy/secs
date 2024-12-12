//
//  main.c
//  x
//
//  Created by George Watson on 19/11/2022.
//

#include "test.h"

typedef struct {
    float x, y;
} Position;

int main(int argc, const char *argv[]) {
    world_t *world = ecs_world();
    OK(world != NULL, true);
    entity_t e1 = ecs_spawn(world);
    OK(entity_isa(world, e1, ECS_ENTITY), true);
    entity_t c1 = ecs_component(world, sizeof(Position));
    OK(entity_isa(world, c1, ECS_COMPONENT), true);
    Position *p1 = entity_give(world, e1, c1);
    OK(p1 != NULL, true);
    p1->x = 10;
    p1->y = 20;
    Position *p2 = entity_get(world, e1, c1);
    OK(p2->x == 10, true);
    OK(p2->y == 20, true);
    entity_remove(world, e1, c1);
    Position *p3 = entity_get(world, e1, c1);
    OK(p3, false);
    ecs_delete(world, e1);
    OK(entity_isvalid(world, e1), false);
    ecs_world_destroy(&world);
    return 0;
}
