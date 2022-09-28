#include <stdio.h>
#include <assert.h>
#include "secs.h"

int main(int argc, char *argv[]) {
    EcsWorld *world = NewWorld();
    
    DeleteWorld(&world);
    return 0;
}
