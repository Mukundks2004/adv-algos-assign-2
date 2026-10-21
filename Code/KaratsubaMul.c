#include "Common.h"

#include <stdio.h>
#include <stdlib.h>

/* Scaffold only: Karatsuba multiplication is not implemented yet. */

static void KaratsubaSetup(mp_size_t n)
{
    (void)n;
}

static void KaratsubaTeardown(void)
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
    fprintf(stderr, "Karatsuba multiplication is not implemented\n");
    abort();
}

const MulAlgorithm KaratsubaAlgorithm =
{
    "karatsuba",
    false,
    KaratsubaSetup,
    custom_mul_n,
    KaratsubaTeardown
};
