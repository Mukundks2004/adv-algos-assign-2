#include "Common.h"

#include <string.h>

static void SchoolbookSetup(mp_size_t n)
{
    (void)n;
}

static void SchoolbookTeardown(void)
{
}

static void custom_mul_n(
    mp_limb_t *result,
    const mp_limb_t *a,
    const mp_limb_t *b,
    mp_size_t n)
{
    memset(result, 0, (size_t)(2 * n) * sizeof(mp_limb_t));

    for (mp_size_t i = 0; i < n; i++)
    {
        mp_limb_t carry = 0;
        mp_limb_t multiplier = a[i];

        for (mp_size_t j = 0; j < n; j++)
        {
            DoubleLimb product = (DoubleLimb)multiplier * b[j] + result[i + j] + carry;
            result[i + j] = (mp_limb_t)product;
            carry = (mp_limb_t)(product >> GMP_LIMB_BITS);
        }

        result[i + n] = carry;
    }
}

const MulAlgorithm SchoolbookAlgorithm =
{
    "schoolbook",
    true,
    SchoolbookSetup,
    custom_mul_n,
    SchoolbookTeardown
};
