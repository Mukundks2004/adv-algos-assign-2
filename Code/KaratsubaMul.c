#include "Common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Karatsuba multiplication.
 * Primary author: Jack McCowan
 *
 * Each operand is split at h = ceil(n / 2) limbs, a = a1 * B^h + a0 and
 * b = b1 * B^h + b0 with B = 2^GMP_LIMB_BITS, so a0 and b0 have h limbs and
 * a1 and b1 have n - h limbs. Then
 *
 *     a * b = z2 * B^2h + z1 * B^h + z0
 *     z0 = a0 * b0
 *     z2 = a1 * b1
 *     z1 = a0 * b1 + a1 * b0 = z0 + z2 - (a0 - a1)(b0 - b1)
 *
 * so three half size products replace four, giving Theta(n^log2(3)).
 *
 * The subtractive form of z1 is used (as in GMP's toom22) rather than
 * (a0 + a1)(b0 + b1) - z0 - z2: |a0 - a1| and |b0 - b1| always fit in h
 * limbs, whereas a0 + a1 can carry into an extra limb. Every recursive
 * product is therefore square and strictly smaller than n, at the cost of
 * tracking the sign of (a0 - a1)(b0 - b1).
 *
 * At or below KARATSUBA_THRESHOLD limbs the schoolbook implementation is used
 * as the base case, so the two algorithms share the same inner kernel.
 * Override the threshold at build time with -DKARATSUBA_THRESHOLD=k. The
 * default of 16 was the fastest in a sweep of 1 to 96 (gcc 11.4 -O2, x86-64
 * Ubuntu 22.04 container); 12 to 28 were within a few percent of it.
 */

#ifndef KARATSUBA_THRESHOLD
#define KARATSUBA_THRESHOLD 16
#endif

#if KARATSUBA_THRESHOLD < 1
#error "KARATSUBA_THRESHOLD must be at least 1 or the recursion never terminates"
#endif

static mp_limb_t *scratch = NULL;
static mp_size_t setupLimbs = 0;

/* result = a + b over n limbs, returns the carry out. result may alias a or b. */
static mp_limb_t AddLimbs(mp_limb_t *result, const mp_limb_t *a, const mp_limb_t *b, mp_size_t n)
{
    mp_limb_t carry = 0;

    for (mp_size_t i = 0; i < n; i++)
    {
        DoubleLimb sum = (DoubleLimb)a[i] + b[i] + carry;
        result[i] = (mp_limb_t)sum;
        carry = (mp_limb_t)(sum >> GMP_LIMB_BITS);
    }

    return carry;
}

/* result = a - b over n limbs, returns the borrow out. result may alias a or b. */
static mp_limb_t SubtractLimbs(mp_limb_t *result, const mp_limb_t *a, const mp_limb_t *b, mp_size_t n)
{
    mp_limb_t borrow = 0;

    for (mp_size_t i = 0; i < n; i++)
    {
        DoubleLimb difference = (DoubleLimb)a[i] - b[i] - borrow;
        result[i] = (mp_limb_t)difference;
        borrow = (mp_limb_t)(difference >> GMP_LIMB_BITS) & 1;
    }

    return borrow;
}

/* result[0..n) += value in place, returns the carry out of the top limb. */
static mp_limb_t AddLimb(mp_limb_t *result, mp_size_t n, mp_limb_t value)
{
    for (mp_size_t i = 0; i < n && value != 0; i++)
    {
        result[i] += value;
        value = result[i] < value;
    }

    return value;
}

/* result[0..n) = a[0..n) - value, returns the borrow out of the top limb. */
static mp_limb_t SubtractLimb(mp_limb_t *result, const mp_limb_t *a, mp_size_t n, mp_limb_t value)
{
    for (mp_size_t i = 0; i < n; i++)
    {
        mp_limb_t limb = a[i];
        result[i] = limb - value;
        value = limb < value;
    }

    return value;
}

/* result[0..n) += a[0..m) in place with m <= n, returns the carry out of the top limb. */
static mp_limb_t AddInto(mp_limb_t *result, mp_size_t n, const mp_limb_t *a, mp_size_t m)
{
    mp_limb_t carry = AddLimbs(result, result, a, m);
    return AddLimb(result + m, n - m, carry);
}

/* Compares a (n limbs) with b (m <= n limbs, zero padded to n). Returns -1, 0 or 1. */
static int CompareLimbs(const mp_limb_t *a, mp_size_t n, const mp_limb_t *b, mp_size_t m)
{
    for (mp_size_t i = n - 1; i >= m; i--)
    {
        if (a[i] != 0)
        {
            return 1;
        }
    }

    for (mp_size_t i = m - 1; i >= 0; i--)
    {
        if (a[i] != b[i])
        {
            return a[i] > b[i] ? 1 : -1;
        }
    }

    return 0;
}

/*
 * result[0..n) = |a - b| where a has n limbs and b has m <= n limbs.
 * Returns true when a < b, i.e. when a - b is negative.
 */
static bool AbsoluteDifference(mp_limb_t *result, const mp_limb_t *a, mp_size_t n, const mp_limb_t *b, mp_size_t m)
{
    if (CompareLimbs(a, n, b, m) < 0)
    {
        /* a < b forces a[m..n) to be zero, so b - a only involves the low m limbs. */
        SubtractLimbs(result, b, a, m);
        memset(result + m, 0, (size_t)(n - m) * sizeof(mp_limb_t));
        return true;
    }

    mp_limb_t borrow = SubtractLimbs(result, a, b, m);
    SubtractLimb(result + m, a + m, n - m, borrow);
    return false;
}

/*
 * Scratch limbs needed to multiply two n limb operands. Each level uses 4h
 * limbs (|a0 - a1|, |b0 - b1| and their 2h limb product) and hands the rest
 * of the buffer to its children, the largest of which has h limbs.
 */
static mp_size_t ScratchLimbs(mp_size_t n)
{
    mp_size_t total = 0;

    while (n > KARATSUBA_THRESHOLD)
    {
        mp_size_t lowSize = n - n / 2;
        total += 4 * lowSize;
        n = lowSize;
    }

    return total;
}

/* result[0..2n) = a * b, using workspace[0..ScratchLimbs(n)). */
static void KaratsubaRecurse(
    mp_limb_t *result,
    const mp_limb_t *a,
    const mp_limb_t *b,
    mp_size_t n,
    mp_limb_t *workspace)
{
    if (n <= KARATSUBA_THRESHOLD)
    {
        SchoolbookAlgorithm.mul(result, a, b, n);
        return;
    }

    mp_size_t lowSize = n - n / 2;
    mp_size_t highSize = n / 2;
    const mp_limb_t *aHigh = a + lowSize;
    const mp_limb_t *bHigh = b + lowSize;
    mp_limb_t *aDifference = workspace;
    mp_limb_t *bDifference = workspace + lowSize;
    mp_limb_t *middle = workspace + 2 * lowSize;
    mp_limb_t *childWorkspace = workspace + 4 * lowSize;
    mp_limb_t *z0 = result;
    mp_limb_t *z2 = result + 2 * lowSize;

    bool aNegative = AbsoluteDifference(aDifference, a, lowSize, aHigh, highSize);
    bool bNegative = AbsoluteDifference(bDifference, b, lowSize, bHigh, highSize);

    /* middle = |a0 - a1| * |b0 - b1|, z0 and z2 are written straight into result. */
    KaratsubaRecurse(middle, aDifference, bDifference, lowSize, childWorkspace);
    KaratsubaRecurse(z0, a, b, lowSize, childWorkspace);
    KaratsubaRecurse(z2, aHigh, bHigh, highSize, childWorkspace);

    /*
     * Overwrite middle with z1 = z0 + z2 -/+ middle. z1 = a0 * b1 + a1 * b0 is
     * non negative and below 2 * B^2h, so it is 2h limbs plus a top carry of
     * 0 or 1. The intermediate borrow and carries always cancel down to that.
     */
    mp_limb_t topCarry;

    if (aNegative == bNegative)
    {
        /* (a0 - a1)(b0 - b1) >= 0, so z1 = z0 + z2 - middle. */
        mp_limb_t borrow = SubtractLimbs(middle, z0, middle, 2 * lowSize);
        topCarry = AddInto(middle, 2 * lowSize, z2, 2 * highSize) - borrow;
    }
    else
    {
        /* (a0 - a1)(b0 - b1) < 0, so z1 = z0 + z2 + middle. */
        topCarry = AddLimbs(middle, middle, z0, 2 * lowSize);
        topCarry += AddInto(middle, 2 * lowSize, z2, 2 * highSize);
    }

    /*
     * result += z1 * B^h. The product a * b fits in 2n limbs, so nothing can
     * carry out of the top. 3h <= 2n holds for every n >= 2.
     */
    mp_limb_t carry = AddInto(result + lowSize, 2 * n - lowSize, middle, 2 * lowSize);
    carry += AddLimb(result + 3 * lowSize, 2 * n - 3 * lowSize, topCarry);
    assert(carry == 0);
    (void)carry;
}

static void KaratsubaTeardown(void)
{
    free(scratch);
    scratch = NULL;
    setupLimbs = 0;
}

static void KaratsubaSetup(mp_size_t n)
{
    KaratsubaTeardown();

    size_t scratchLimbs = (size_t)ScratchLimbs(n);
    scratch = malloc((scratchLimbs > 0 ? scratchLimbs : 1) * sizeof(mp_limb_t));

    if (scratch == NULL)
    {
        fprintf(stderr, "Karatsuba multiplication: out of memory\n");
        exit(EXIT_FAILURE);
    }

    /* Touch every page now so first touch faults are not paid while timing. */
    memset(scratch, 0, scratchLimbs * sizeof(mp_limb_t));
    setupLimbs = n;
}

static void custom_mul_n(
    mp_limb_t *result,
    const mp_limb_t *a,
    const mp_limb_t *b,
    mp_size_t n)
{
    if (n > setupLimbs)
    {
        fprintf(stderr, "Karatsuba multiplication: called with %ld limbs after setup for %ld\n", (long)n, (long)setupLimbs);
        abort();
    }

    KaratsubaRecurse(result, a, b, n, scratch);
}

const MulAlgorithm KaratsubaAlgorithm =
{
    "karatsuba",
    true,
    KaratsubaSetup,
    custom_mul_n,
    KaratsubaTeardown
};
