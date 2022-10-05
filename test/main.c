#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "secs.h"

// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
static uint64_t MurmurHash3_x86_128(const void *key, const int len, uint32_t seed) {
#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))
#define FMIX32(h) h^=h>>16; h*=0x85ebca6b; h^=h>>13; h*=0xc2b2ae35; h^=h>>16;
    const uint8_t * data = (const uint8_t*)key;
    const int nblocks = len / 16;
    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;
    uint32_t c1 = 0x239b961b;
    uint32_t c2 = 0xab0e9789;
    uint32_t c3 = 0x38b34ae5;
    uint32_t c4 = 0xa1e38b93;
    const uint32_t * blocks = (const uint32_t *)(data + nblocks*16);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i*4+0];
        uint32_t k2 = blocks[i*4+1];
        uint32_t k3 = blocks[i*4+2];
        uint32_t k4 = blocks[i*4+3];
        k1 *= c1; k1  = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
        h1 = ROTL32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;
        k2 *= c2; k2  = ROTL32(k2,16); k2 *= c3; h2 ^= k2;
        h2 = ROTL32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;
        k3 *= c3; k3  = ROTL32(k3,17); k3 *= c4; h3 ^= k3;
        h3 = ROTL32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;
        k4 *= c4; k4  = ROTL32(k4,18); k4 *= c1; h4 ^= k4;
        h4 = ROTL32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
    }
    const uint8_t * tail = (const uint8_t*)(data + nblocks*16);
    uint32_t k1 = 0;
    uint32_t k2 = 0;
    uint32_t k3 = 0;
    uint32_t k4 = 0;
    switch(len & 15) {
    case 15: k4 ^= tail[14] << 16;
    case 14: k4 ^= tail[13] << 8;
    case 13: k4 ^= tail[12] << 0;
             k4 *= c4; k4  = ROTL32(k4,18); k4 *= c1; h4 ^= k4;
    case 12: k3 ^= tail[11] << 24;
    case 11: k3 ^= tail[10] << 16;
    case 10: k3 ^= tail[ 9] << 8;
    case  9: k3 ^= tail[ 8] << 0;
             k3 *= c3; k3  = ROTL32(k3,17); k3 *= c4; h3 ^= k3;
    case  8: k2 ^= tail[ 7] << 24;
    case  7: k2 ^= tail[ 6] << 16;
    case  6: k2 ^= tail[ 5] << 8;
    case  5: k2 ^= tail[ 4] << 0;
             k2 *= c2; k2  = ROTL32(k2,16); k2 *= c3; h2 ^= k2;
    case  4: k1 ^= tail[ 3] << 24;
    case  3: k1 ^= tail[ 2] << 16;
    case  2: k1 ^= tail[ 1] << 8;
    case  1: k1 ^= tail[ 0] << 0;
             k1 *= c1; k1  = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };
    h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    FMIX32(h1); FMIX32(h2); FMIX32(h3); FMIX32(h4);
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    char out[16];
    ((uint32_t*)out)[0] = h1;
    ((uint32_t*)out)[1] = h2;
    ((uint32_t*)out)[2] = h3;
    ((uint32_t*)out)[3] = h4;
    return *(uint64_t*)out;
}

typedef struct {
    uint64_t hash:48;
    uint64_t dib:16;
} Bucket;

static void* BucketItem(Bucket *bucket) {
    return ((char*)bucket)+sizeof(Bucket);
}

typedef uint64_t(*MapHashCb)(const void *item, uint32_t seed);
typedef bool(*MapCmpCb)(const void*, const void*);
typedef void(*MapFreeCb)(void*);
typedef bool(*MapIterCb)(void*);

typedef struct {
    uint32_t seed;
    
    MapHashCb hashCb;
    MapCmpCb compareCb;
    MapFreeCb freeCb;
    
    size_t bucketsz,
           nbuckets,
           count,
           mask,
           growat,
           shrinkat,
           elsize,
           cap;
    
    void *buckets,
         *spare,
         *edata;
} Map;

void InitMap(Map *map, MapHashCb hashCb, MapCmpCb compareCb, MapFreeCb freeCb, size_t sizeOfElements) {
    size_t bucketsz = sizeof(Bucket) + sizeOfElements;
    while (bucketsz & (sizeof(uintptr_t)-1))
        bucketsz++;
    memset(map, 0, sizeof(Map));
    
    map->elsize = sizeOfElements;
    map->bucketsz = bucketsz;
    map->spare = ((char*)map)+sizeof(Map);
    map->edata = (char*)map->spare+bucketsz;
    map->cap = map->nbuckets = 16;
    map->mask = map->nbuckets-1;
    
    map->buckets = malloc(map->bucketsz*map->nbuckets);
    memset(map->buckets, 0, map->bucketsz*map->nbuckets);
    map->growat = map->nbuckets*0.75;
    map->shrinkat = map->nbuckets*0.10;
    
    map->hashCb = hashCb;
    map->compareCb = compareCb;
    map->freeCb = freeCb;
    
    uint32_t buffer[17];
    uint32_t seed = (uint32_t)clock();
    for (uint32_t i = 0; i < 17; i++)
        buffer[i] = seed *= 0xac564b05 + 1;
    map->seed = buffer[0] = ROTL32(buffer[10], 13) + ROTL32(buffer[0], 9);
}

static Bucket* BucketAt(Map *map, size_t index) {
    return (Bucket*)(((char*)map->buckets)+(map->bucketsz*index));
}

static void ResizeMap(Map *map, size_t new_cap) {
    Map map2;
    InitMap(&map2, NULL, NULL, NULL, map->elsize);
        
    for (size_t i = 0; i < map->nbuckets; i++) {
        Bucket *entry = BucketAt(map, i);
        if (!entry->dib)
            continue;
        entry->dib = 1;
        size_t j = entry->hash & map2.mask;
        for (;;) {
            Bucket *bucket = BucketAt(&map2, j);
            if (bucket->dib == 0) {
                memcpy(bucket, entry, map->bucketsz);
                break;
            }
            if (bucket->dib < entry->dib) {
                memcpy(map2.spare, bucket, map->bucketsz);
                memcpy(bucket, entry, map->bucketsz);
                memcpy(entry, map2.spare, map->bucketsz);
            }
            j = (j + 1) & map2.mask;
            entry->dib += 1;
        }
    }
    
    free(map->buckets);
    map->buckets = map2.buckets;
    map->nbuckets = map2.nbuckets;
    map->mask = map2.mask;
    map->growat = map2.growat;
    map->shrinkat = map2.shrinkat;
}

static uint64_t MapHash(Map *map, const void *key) {
    return map->hashCb(key, map->seed) << 16 >> 16;
}

void* MapSet(Map *map, void *item) {
    if (map->count == map->growat)
        ResizeMap(map, map->nbuckets*2);
    
    Bucket *entry = map->edata;
    entry->hash = MapHash(map, item);
    entry->dib = 1;
    memcpy(BucketItem(entry), item, map->elsize);
    
    size_t i = entry->hash & map->mask;
    for (;;) {
        Bucket *bucket = BucketAt(map, i);
        if (!bucket->dib) {
            memcpy(bucket, entry, map->bucketsz);
            map->count++;
            return NULL;
        }
        if (entry->hash == bucket->hash && !map->compareCb(BucketItem(entry), BucketItem(bucket))) {
            memcpy(map->spare, BucketItem(bucket), map->elsize);
            memcpy(BucketItem(bucket), BucketItem(entry), map->elsize);
            return map->spare;
        }
        if (bucket->dib < entry->dib) {
            memcpy(map->spare, bucket, map->bucketsz);
            memcpy(bucket, entry, map->bucketsz);
            memcpy(entry, map->spare, map->bucketsz);
        }
        i = (i + 1) & map->mask;
        entry->dib += 1;
    }
}

void* MapGet(Map *map, const void *key) {
    uint64_t hash = MapHash(map, key);
    size_t i = hash & map->mask;
    for (;;) {
        Bucket *bucket = BucketAt(map, i);
        if (!bucket->dib)
            return NULL;
        if (bucket->hash == hash && !map->compareCb(key, BucketItem(bucket)))
            return BucketItem(bucket);
        i = (i + 1) & map->mask;
    }
}

void* MapDel(Map *map, const void *key) {
    uint64_t hash = MapHash(map, key);
    size_t i = hash & map->mask;
    for (;;) {
        Bucket *bucket = BucketAt(map, i);
        if (!bucket->dib)
            return NULL;
        
        if (bucket->hash == hash && !map->compareCb(key, BucketItem(bucket))) {
            memcpy(map->spare, BucketItem(bucket), map->elsize);
            bucket->dib = 0;
            for (;;) {
                Bucket *prev = bucket;
                i = (i + 1) & map->mask;
                bucket = BucketAt(map, i);
                if (bucket->dib <= 1) {
                    prev->dib = 0;
                    break;
                }
                memcpy(prev, bucket, map->bucketsz);
                prev->dib--;
            }
            map->count--;
            if (map->nbuckets > map->cap && map->count <= map->shrinkat)
                ResizeMap(map, map->nbuckets/2);
            return map->spare;
        }
        i = (i + 1) & map->mask;
    }
}

static void FreeMapItems(Map *map) {
    if (!map->freeCb)
        return;
    for (size_t i = 0; i < map->nbuckets; i++) {
        Bucket *bucket = BucketAt(map, i);
        if (bucket->dib)
            map->freeCb(BucketItem(bucket));
    }
}

void EmptyMap(Map *map) {
    map->count = 0;
    FreeMapItems(map);
    if (map->nbuckets != map->cap) {
        void *new_buckets = malloc(map->bucketsz*map->cap);
        if (new_buckets) {
            free(map->buckets);
            map->buckets = new_buckets;
        }
        map->nbuckets = map->cap;
    }
    memset(map->buckets, 0, map->bucketsz*map->nbuckets);
    map->mask = map->nbuckets-1;
    map->growat = map->nbuckets*0.75;
    map->shrinkat = map->nbuckets*0.10;
}

void DestroyMap(Map *map) {
    FreeMapItems(map);
    free(map->buckets);
}

void MapEach(Map *map, MapIterCb cb) {
    for (size_t i = 0; i < map->nbuckets; i++) {
        Bucket *bucket = BucketAt(map, i);
        if (!bucket->dib)
            continue;
        if (!cb(BucketItem(bucket)))
            return;
    }
}

typedef struct {
    const char *name;
    uint32_t age;
} Person;

uint64_t PersonHash(const void *item, uint32_t seed) {
    const char *key = ((Person*)item)->name;
    return MurmurHash3_x86_128(key, (uint32_t)strlen(key), seed);
}

bool PersonCmp(const void *a, const void *b) {
    return strcmp(((Person*)a)->name, ((Person*)b)->name);
}

bool PersonIter(void *item) {
    Person *person = (Person*)item;
    printf("%s: %d\n", person->name, person->age);
    return true;
}

int main(int argc, char *argv[]) {
    Map map;
    InitMap(&map, PersonHash, PersonCmp, NULL, sizeof(Person));
    
    MapSet(&map, &(Person) { .name = "Test1", .age = 1 });
    MapSet(&map, &(Person) { .name = "Test2", .age = 2 });
    MapSet(&map, &(Person) { .name = "Test3", .age = 3 });
    
    MapEach(&map, PersonIter);
    
    Person *test1 = MapGet(&map, &(Person) { .name = "Test1" });
    Person *test2 = MapGet(&map, &(Person) { .name = "Test2" });
    Person *test3 = MapGet(&map, &(Person) { .name = "Test3" });
    
    MapDel(&map, &(Person) { .name = "Test1" });
    
    MapEach(&map, PersonIter);
    
    EmptyMap(&map);

    return 0;
}
