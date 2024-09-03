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
- [X] Entities+Components
- [X] Queries
- [X] Systems
- [ ] Tags
- [ ] Prefabs
- [ ] Relationships
- [ ] Pipelines
- [ ] Observers
- [ ] Reflection
- [ ] Rules

## Acknowledgements

- Built using [imap](https://github.com/billziss-gh/imap), an exellent int->int map

## LICENSE
```
The MIT License (MIT)

Copyright (c) 2022 George Watson

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
