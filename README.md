# secs 

> [!WARNING]
> **Work in progress!** -- Currently implemented features are listed below

`secs` stands for ***s***imple ***e***ntity ***c***omponent ***s***ystem.

```c
#define SECS_IMPLEMENTATION
// #define SECS_NO_BLOCKS // Disables blocks, use callbacks instead
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
    
    entity_t regular = ecs_spawn(world);
    
    entity_t position_component = ecs_component(world, sizeof(Position));
    
    entity_t system = ecs_system(world, ^(entity_t e) {
        Position *pos = ecs_get(world, e, position_component);
        pos->x = 1;
        pos->y = 2;
    }, NULL, 1, position_component);
    
    ecs_step(world);
    
    Position *pos = ecs_get(world, entity, position_component);
    printf("%.2f, %.2f\n", pos->x, pos->y);
    // 1.00, 2.000
    
    ecs_destroy(world);
    return 1;
}
```

## TODO

- [ ] Documentation + tests
- [ ] Add Thread-saftey
- [X] Entities+Components
- [X] Queries
- [X] Systems
- [X] Tags
- [ ] Prefabs
- [ ] Relationships
- [ ] Pipelines
- [ ] Observers
- [ ] Reflection

## Acknowledgements

- Built using [imap](https://github.com/billziss-gh/imap), an exellent int->int map

## LICENSE

```
secs -- Simple Entity Component System

Copyright (C) 2024  George Watson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
