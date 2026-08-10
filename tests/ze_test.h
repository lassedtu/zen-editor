/**
 * @file ze_test.h
 * @brief minimal unit testing framework for ze.
 *
 * single-header test framework. define tests with TEST(name), use ASSERT
 * macros for checks, and call RUN_TESTS() from main to execute them all.
 */

#ifndef ZE_TEST_H
#define ZE_TEST_H

#include <stdio.h>
#include <string.h>

#define ZE_TEST_MAX_TESTS 256

typedef struct
{
    const char *name;
    void (*func)(int *failed);
} ZeTest;

static ZeTest ze_tests[ZE_TEST_MAX_TESTS];
static int ze_test_count = 0;

/**
 * @brief register a test function. called by the TEST macro.
 * @param name display name for the test.
 * @param func pointer to the test function.
 */
static void ze_test_register(const char *name, void (*func)(int *failed))
{
    if (ze_test_count < ZE_TEST_MAX_TESTS)
    {
        ze_tests[ze_test_count].name = name;
        ze_tests[ze_test_count].func = func;
        ze_test_count++;
    }
}

/**
 * @brief define a test case.
 *
 * expands to a test function and a constructor that registers it.
 * usage:
 *   TEST(my_test)
 *   {
 *       ASSERT(1 + 1 == 2);
 *   }
 */
#define TEST(name)                                                      \
    static void ze_test_##name(int *ze_test_failed);                    \
    __attribute__((constructor)) static void ze_register_##name(void)   \
    {                                                                    \
        ze_test_register(#name, ze_test_##name);                        \
    }                                                                    \
    static void ze_test_##name(int *ze_test_failed)

/**
 * @brief fail the current test if expr is false.
 */
#define ASSERT(expr)                                                    \
    do {                                                                 \
        if (!(expr))                                                     \
        {                                                                \
            printf("  FAIL: %s:%d: %s\n", __FILE__, __LINE__, #expr);   \
            *ze_test_failed = 1;                                         \
            return;                                                      \
        }                                                                \
    } while (0)

/**
 * @brief fail the current test if a != b (integer comparison).
 */
#define ASSERT_EQ(a, b)                                                 \
    do {                                                                 \
        long ze_a = (long)(a);                                           \
        long ze_b = (long)(b);                                           \
        if (ze_a != ze_b)                                                \
        {                                                                \
            printf("  FAIL: %s:%d: %s == %s (%ld != %ld)\n",           \
                   __FILE__, __LINE__, #a, #b, ze_a, ze_b);             \
            *ze_test_failed = 1;                                         \
            return;                                                      \
        }                                                                \
    } while (0)

/**
 * @brief fail the current test if strings a and b are not equal.
 */
#define ASSERT_STR_EQ(a, b)                                             \
    do {                                                                 \
        const char *ze_a = (a);                                          \
        const char *ze_b = (b);                                          \
        if (ze_a == NULL || ze_b == NULL || strcmp(ze_a, ze_b) != 0)     \
        {                                                                \
            printf("  FAIL: %s:%d: %s == %s (\"%s\" != \"%s\")\n",     \
                   __FILE__, __LINE__, #a, #b,                          \
                   ze_a ? ze_a : "(null)", ze_b ? ze_b : "(null)");     \
            *ze_test_failed = 1;                                         \
            return;                                                      \
        }                                                                \
    } while (0)

/**
 * @brief run all registered tests and print a summary.
 * @return 0 if all tests passed, 1 if any failed.
 */
#define RUN_TESTS()                                                     \
    do {                                                                 \
        int ze_passed = 0;                                               \
        int ze_total_failed = 0;                                         \
        printf("running %d test(s)...\n\n", ze_test_count);             \
        for (int ze_i = 0; ze_i < ze_test_count; ze_i++)                \
        {                                                                \
            int ze_test_failed = 0;                                      \
            ze_tests[ze_i].func(&ze_test_failed);                       \
            if (ze_test_failed)                                          \
            {                                                            \
                printf("  FAILED: %s\n\n", ze_tests[ze_i].name);       \
                ze_total_failed++;                                        \
            }                                                            \
            else                                                         \
            {                                                            \
                printf("  passed: %s\n", ze_tests[ze_i].name);         \
                ze_passed++;                                             \
            }                                                            \
        }                                                                \
        printf("\n%d passed, %d failed, %d total\n",                    \
               ze_passed, ze_total_failed, ze_test_count);              \
        return ze_total_failed > 0 ? 1 : 0;                             \
    } while (0)

#endif /* ZE_TEST_H */
