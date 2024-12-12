//
//  test.h
//  x
//
//  Created by George Watson on 19/11/2022.
//

#ifndef test_h
#define test_h
#include <stdio.h>
#define X_IMPLEMENTATION
#include "x.h"
#if defined(_MSC_VER) || (defined(__STDC__) && __STDC_VERSION__ < 199901L)
typedef enum bool { false = 0, true = !false } bool;
#else
#include <stdbool.h>
#endif

static int test_counter = 0;
static int test_failed  = 0;
static int test_success = 0;

#ifdef _MSC_VER
#define GREEN_ANSI
#define RED_ANSI
#define RESET_ANSI
#else
#define GREEN_ANSI "\033[32m"
#define RED_ANSI "\033[31m"
#define RESET_ANSI "\033[0m"
#endif

#define OK(TEST, EXPECTED) do { \
    int result = (TEST) == (EXPECTED); \
    printf("[TEST%02d] %s == %s? %s%s%s\n", test_counter++, #TEST, #EXPECTED, result ? GREEN_ANSI : RED_ANSI, result ? "YES" : "NO", RESET_ANSI); \
    if (result) \
        test_success++;\
    else \
        test_failed++; \
} while(0)


#endif /* test_h */
