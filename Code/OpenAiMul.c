#include "Common.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Scaffold only: the O(n log n) integer multiplication algorithm from
 * https://github.com/openai/math/tree/main/preprints/Integer-multiplication-below-n-log-n-September-23-2026
 * is not implemented yet.
 */

static void OpenAiSetup(mp_size_t n)
{
    (void)n;
}

static void OpenAiTeardown(void)
{
}

static void custom_mul_n(
    mp_limb_t *result,
    const mp_limb_t *a,
    const mp_limb_t *b,
    mp_size_t n)
{
    (void)result;
    (void)a;
    (void)b;
    (void)n;
    fprintf(stderr, "OpenAI nlogn multiplication is not implemented\n");
    abort();
}

const MulAlgorithm OpenAiAlgorithm =
{
    "openai-nlogn",
    false,
    OpenAiSetup,
    custom_mul_n,
    OpenAiTeardown
};
