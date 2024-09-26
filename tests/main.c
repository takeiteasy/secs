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

typedef struct {
    float x;
    float y;
    float z;
} Position3D;

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
    assert(ecs_isvalid(world, e1));
    assert(ecs_isvalid(world, e2));
    assert(!ecs_isvalid(world, e3));
    assert(ecs_isvalid(world, e4));
    
    entity_t c1 = ecs_component(world, sizeof(Position));
    assert(ecs_isa(world, c1, ECS_TYPE_COMPONENT));
    assert(!ecs_has(world, e1, c1));
    ecs_give(world, e1, c1);
    assert(ecs_has(world, e1, c1));
    ecs_remove(world, e1, c1);
    assert(!ecs_has(world, e1, c1));
    
    ecs_give(world, e2, c1);
    assert(ecs_has(world, e2, c1));
    __block int n = 0;
    entity_t s1 = ecs_system(world, ^(entity_t e) {
        assert(ecs_cmp(e, e2));
        n++;
        dump_entity(e);
    }, NULL, 1, c1);
    
    assert(!n);
    ecs_step(world);
    assert(n == 1);
    
    ecs_delete(world, s1);
    
    ecs_step(world);
    assert(n == 1);
    
    entity_t c2 = ecs_component(world, sizeof(Position3D));
    assert(ecs_isa(world, c2, ECS_TYPE_COMPONENT));
    ecs_give(world, e2, c2);
    ecs_give(world, e4, c1);
    
    entity_t s2 = ecs_system(world, ^(entity_t e) {
        assert(ecs_cmp(e, e4));
        dump_entity(e);
    }, ^bool(entity_t e) {
        return !ecs_has(world, e, c2);
    }, 1, c1);
    
    ecs_step(world);
    
    ecs_disable(world, s2);
    
    ecs_step(world);
    
    ecs_enable(world, s2);
    
    ecs_step(world);
    
    entity_t e5 = ecs_spawn(world);
    entity_t e6 = ecs_spawn(world);
    entity_t t1 = ecs_tag(world);
    ecs_give(world, e5, t1);
    ecs_give(world, e6, t1);
    assert(ecs_has(world, e5, t1) && ecs_has(world, e6, t1));
    ecs_remove(world, e6, t1);
    assert(ecs_has(world, e5, t1) && ecs_has(world, e6, t1));

    ecs_destroy(world);
    return 0;
}
