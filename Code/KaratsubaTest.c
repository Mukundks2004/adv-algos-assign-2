#include "Common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Correctness test for KaratsubaMul.c against mpn_mul_n on structured inputs
 * chosen to exercise every carry, borrow and sign path (see Fill below).
 * Primary author: Jack McCowan
 *
 * Not part of the benchmark build. From the repository root:
 *
 *     gcc -O1 -g -std=gnu99 -fsanitize=address,undefined -I Code Code/KaratsubaTest.c \
 *         Code/KaratsubaMul.c Code/SchoolbookMul.c -o KaratsubaTest -lgmp && ./KaratsubaTest
 *
 * Add -DKARATSUBA_THRESHOLD=k to test other base case cutoffs.
 */

#define PATTERN_COUNT 9
#define CANARY ((mp_limb_t)0xDEADBEEFCAFEF00DULL)

static void Fill(mp_limb_t *x, mp_size_t n, int pattern)
{
    mp_size_t half = n - n / 2;

    switch (pattern)
    {
    case 0: /* all ones: maximum carries everywhere */
        memset(x, 0xFF, (size_t)n * sizeof(mp_limb_t));
        break;
    case 1: /* uniform random */
        mpn_random(x, n);
        break;
    case 2: /* long runs of zeros and ones, GMP's corner case generator */
        mpn_random2(x, n);
        break;
    case 3: /* zero */
        memset(x, 0, (size_t)n * sizeof(mp_limb_t));
        break;
    case 4: /* low half all ones, high half zero: a0 > a1 */
        memset(x, 0, (size_t)n * sizeof(mp_limb_t));
        memset(x, 0xFF, (size_t)half * sizeof(mp_limb_t));
        break;
    case 5: /* low half zero, high half all ones: a0 < a1 */
        memset(x, 0xFF, (size_t)n * sizeof(mp_limb_t));
        memset(x, 0, (size_t)half * sizeof(mp_limb_t));
        break;
    case 6: /* one */
        memset(x, 0, (size_t)n * sizeof(mp_limb_t));
        x[0] = 1;
        break;
    case 7: /* only the top limb set */
        memset(x, 0, (size_t)n * sizeof(mp_limb_t));
        x[n - 1] = ~(mp_limb_t)0;
        break;
    default: /* a0 == a1 (as far as the shorter high half reaches): zero difference */
        mpn_random2(x, n);
        for (mp_size_t i = 0; i < n / 2; i++)
        {
            x[half + i] = x[i];
        }
        if (half > n / 2)
        {
            x[half - 1] = 0;
        }
        break;
    }
}

static int CheckOne(mp_limb_t *a, mp_limb_t *b, mp_size_t n, mp_limb_t *result, mp_limb_t *expected, int pa, int pb)
{
    result[2 * n] = CANARY;
    result[2 * n + 1] = CANARY;
    KaratsubaAlgorithm.mul(result, a, b, n);
    mpn_mul_n(expected, a, b, n);

    if (memcmp(result, expected, (size_t)(2 * n) * sizeof(mp_limb_t)) != 0 || result[2 * n] != CANARY || result[2 * n + 1] != CANARY)
    {
        fprintf(stderr, "FAIL n=%ld patterns=(%d,%d)\n", (long)n, pa, pb);
        return 1;
    }

    return 0;
}

int main(void)
{
    const mp_size_t extraSizes[] = { 255, 256, 257, 511, 512, 513, 1000, 1023, 1024, 1025, 2047, 2049, 4095, 4097, 10007 };
    const int extraCount = (int)(sizeof(extraSizes) / sizeof(extraSizes[0]));
    mp_size_t maxN = 10007;
    mp_limb_t *a = malloc((size_t)maxN * sizeof(mp_limb_t));
    mp_limb_t *b = malloc((size_t)maxN * sizeof(mp_limb_t));
    mp_limb_t *result = malloc((size_t)(2 * maxN + 2) * sizeof(mp_limb_t));
    mp_limb_t *expected = malloc((size_t)(2 * maxN) * sizeof(mp_limb_t));
    long checks = 0;
    int failures = 0;

    /* Exact sized setup for every n: ASan catches any scratch overrun. */
    for (int s = 0; s < 200 + extraCount; s++)
    {
        mp_size_t n = s < 200 ? s + 1 : extraSizes[s - 200];
        int repeats = n <= 200 ? 3 : 1;

        KaratsubaAlgorithm.setup(n);

        for (int r = 0; r < repeats; r++)
        {
            for (int pa = 0; pa < PATTERN_COUNT; pa++)
            {
                for (int pb = 0; pb < PATTERN_COUNT; pb++)
                {
                    Fill(a, n, pa);
                    Fill(b, n, pb);
                    failures += CheckOne(a, b, n, result, expected, pa, pb);
                    checks++;
                }
            }
        }

        /* Squaring with aliased operands, as a real caller might do. */
        Fill(a, n, 2);
        failures += CheckOne(a, a, n, result, expected, 2, 2);
        checks++;

        KaratsubaAlgorithm.teardown();
    }

    /* One large setup reused for every smaller n: checks the scratch size is monotone in n. */
    KaratsubaAlgorithm.setup(1025);

    for (mp_size_t n = 1; n <= 1025; n++)
    {
        Fill(a, n, 2);
        Fill(b, n, 1);
        failures += CheckOne(a, b, n, result, expected, 2, 1);
        checks++;
    }

    KaratsubaAlgorithm.teardown();

    printf("%s: %ld checks, %d failures\n", failures == 0 ? "PASS" : "FAIL", checks, failures);
    free(a);
    free(b);
    free(result);
    free(expected);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
