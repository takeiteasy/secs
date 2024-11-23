//
//  main.c
//  secs
//
//  Created by George Watson on 19/11/2022.
//

#include "test.h"

int main(int argc, const char *argv[]) {
    world_t *world = ecs_world();
    ecs_world_destroy(&world);
    return 0;
}
