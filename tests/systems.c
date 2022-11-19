//
//  systems.c
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

static int posCbCounter = 0;
static void posCb(View *view) {
    TEST(!!ECS_FIELD(view, Position, 0), true);
    posCbCounter++;
}

static int moveCbCounter = 0;
static void moveCb(View *view) {
    TEST(!!ECS_FIELD(view, Position, 0), true);
    TEST(!!ECS_FIELD(view, Velocity, 1), true);
    moveCbCounter++;
}

int main(int argc, const char *argv[]) {
    World *world = EcsWorld();
    
    Entity e1 = EcsNewEntity(world);
    Entity position = ECS_COMPONENT(world, Position);
    EcsAttach(world, e1, position);
    Entity e2 = EcsNewEntity(world);
    EcsAttach(world, e2, position);
    
    Entity velocity = ECS_COMPONENT(world, Velocity);
    EcsAttach(world, e2, position);
    ECS_QUERY(world, posCb, position);
    EcsAttach(world, e1, velocity);
    ECS_QUERY(world, moveCb, position, velocity);
    TEST(posCbCounter, 2);
    TEST(moveCbCounter, 1);
    
    Entity e4 = EcsNewEntity(world);
    EcsAttach(world, e4, position);
    EcsAttach(world, e4, velocity);
    Entity testSystemA = ECS_SYSTEM(world, posCb, position);
    Entity testSystemB = ECS_SYSTEM(world, moveCb, position, velocity);
    
    EcsStep(world);
    TEST(posCbCounter, 5);
    TEST(moveCbCounter, 3);
    
    DeleteEntity(world, testSystemA);
    EcsStep(world);
    TEST(posCbCounter, moveCbCounter);
    
    Entity e5 = EcsNewEntity(world);
    Entity testPrefab = ECS_PREFAB(world, position, velocity);
    EcsAttach(world, e5, testPrefab);
    TEST(EcsHas(world, e5, position), true);
    TEST(EcsHas(world, e5, velocity), true);
    
    TEST(ENTITY_ISA(testPrefab, Prefab), true);
    TEST(ENTITY_ISA(testSystemB, System), true);
    TEST(ENTITY_ISA(position, Component), true);
    TEST(ENTITY_ISA(e5, Component), false);

    DeleteWorld(&world);
    return EXIT_SUCCESS;
}
