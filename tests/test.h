//
//  test.h
//  secs
//
//  Created by George Watson on 19/11/2022.
//

#ifndef test_h
#define test_h
#define ZAWARUDO_IMPLEMENTATION
#include "zawarudo.h"
#include <stdio.h>

static int TEST_COUNTER = 0;
#define TEST(OBJECT, RESULT)                                                                              \
    do                                                                                                    \
    {                                                                                                     \
        bool res = (OBJECT) == (RESULT);                                                                  \
        printf("[TEST%02d:%s] %s == %s\n", ++TEST_COUNTER, res ? "SUCCESS" : "FAILED", #OBJECT, #RESULT); \
        if (!res)                                                                                         \
            exit(1);                                                                           \
    } while (0)

#endif /* test_h */
