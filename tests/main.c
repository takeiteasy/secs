//
//  main.c
//  secs
//
//  Created by George Watson on 19/11/2022.
//

#include "test.h"

int main(int argc, const char *argv[]) {
    World *world = EcsWorld();
    
    DeleteWorld(&world);
    return EXIT_SUCCESS;
}
