//
//  main.c
//  secs
//
//  Created by George Watson on 19/11/2022.
//

#define SECS_IMPLEMENTATION
#include "secs.h"
#include <stdio.h>

typedef struct {
    float x;
    float y;
} Position;

static void dump_entity(entity_t e) {
    printf("(%llx: {%d, %d, %d, %d})\n", e.value, e.id, e.version, e.alive, e.type);
}

int main(int argc, const char *argv[]) {
    world_t *world = ecs_create();
    
    entity_t e1 = ecs_spawn(world);
    entity_t e2 = ecs_spawn(world);
    entity_t e3 = ecs_spawn(world);
    ecs_delete(world, e3);
    entity_t e4 = ecs_spawn(world);
    bool v1 = ecs_isvalid(world, e1);
    bool v2 = ecs_isvalid(world, e2);
    bool v3 = ecs_isvalid(world, e3);
    bool v4 = ecs_isvalid(world, e4);
    
    entity_t c1 = ecs_component(world, sizeof(Position));
    assert(ecs_isa(world, c1, ECS_TYPE_COMPONENT));
    assert(!ecs_has(world, e1, c1));
    ecs_attach(world, e1, c1);
    assert(ecs_has(world, e1, c1));
    ecs_detach(world, e1, c1);
    assert(!ecs_has(world, e1, c1));
    ecs_attach(world, e2, c1);
    assert(ecs_has(world, e2, c1));
    
    entity_t s1 = ecs_system(world, ^(entity_t e) {
        dump_entity(e);
    }, 1, c1);
    
    ecs_step(world);
    
    ecs_delete(world, s1);
    
    ecs_step(world);
    
    ecs_destroy(world);
    return 0;
}
