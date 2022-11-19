//
//  relations.c
//  secs
//
//  Created by George Watson on 19/11/2022.
//

#include "test.h"

static int childCbCounter = 0;
static void childCb(View *view) {
    TEST(!!ECS_FIELD(view, Relation, 0), true);
    childCbCounter++;
}

int main(int argc, char *argv[]) {
    World *world = EcsWorld();
    
    Entity e1 = EcsNewEntity(world);
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
    return EXIT_SUCCESS;
}
