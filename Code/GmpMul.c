#include "Common.h"

static void GmpSetup(mp_size_t n)
{
    (void)n;
}

static void GmpTeardown(void)
{
}

static void custom_mul_n(
    mp_limb_t *result,
    const mp_limb_t *a,
    const mp_limb_t *b,
    mp_size_t n)
{
    mpn_mul_n(result, a, b, n);
}

const MulAlgorithm GmpAlgorithm =
{
    "gmp",
    true,
    GmpSetup,
    custom_mul_n,
    GmpTeardown
};
