#include "Common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Primary author: Bailey Tucker (24744334)
 *
 * OpenAI, "Integer multiplication below n log n" (23 September 2026),
 * https://github.com/openai/math/tree/main/preprints/Integer-multiplication-below-n-log-n-September-23-2026
 *
 * The paper's Turing machine multiplies in O(n (lg n)^(1-kappa)) with
 * kappa = 2^(-182). That bound is only claimed once every layout hypothesis
 * holds. The writeup then fixes one cutoff past those hypotheses and uses
 * schoolbook multiplication below it. The hypotheses include a dimension
 * d = floor(b^(2^(-75))) with b = ceil(log2 n), so d >= 2 already requires
 * b >= 2^(2^75). No operand this program can allocate is past that cutoff.
 *
 * This file therefore does not emulate the tape schedule. It implements the
 * exact product recovered by the paper's "multiplication algorithm":
 * split both operands into radix-2^16 digits, compute the linear
 * convolution of those digits, and propagate carries back into limbs.
 * The convolution is evaluated with a length-2^k number-theoretic transform
 * modulo P = 2^64 - 2^32 + 1. Seven generates the 2-power roots of unity
 * modulo P. Zero-padding to at least 2q digits makes the cyclic convolution
 * equal the linear one, and every coefficient of an allocatable product fits
 * strictly below P, so the residue is the integer coefficient. If a future
 * operand would break that fit, the call falls back to schoolbook, which is
 * what the paper specifies below its cutoff and is still exact.
 */

#define OPENAI_DIGIT_BITS 16
#define OPENAI_DIGITS_PER_LIMB (GMP_LIMB_BITS / OPENAI_DIGIT_BITS)
#define OPENAI_GENERATOR 7u

static mp_limb_t *digitsA = NULL;
static mp_limb_t *digitsB = NULL;
static mp_limb_t *scratchA = NULL;
static mp_limb_t *scratchB = NULL;
static size_t *bitReverse = NULL;
static size_t transformSize = 0;
static size_t digitCount = 0;
static mp_size_t preparedLimbs = -1;
static int useSchoolbook = 0;

#if GMP_LIMB_BITS == 64
static const mp_limb_t Modulus = 0xFFFFFFFF00000001ULL;
#endif

static void Fail(const char *message)
{
    fprintf(stderr, "OpenAI multiplication: %s\n", message);
    exit(EXIT_FAILURE);
}

static void *CheckedAlloc(size_t count, size_t size)
{
    void *pointer = calloc(count, size);

    if (pointer == NULL)
    {
        Fail("out of memory");
    }

    return pointer;
}

/* 2*n must fit in mp_size_t, and 8*n (two copies of the 16-bit digits) must fit in size_t. */
static int LengthIsUnsafe(mp_size_t n)
{
    uintmax_t limit = (uintmax_t)1 << (sizeof(mp_size_t) * 8 - 2);
    uintmax_t sizeLimit = (uintmax_t)SIZE_MAX / 8;

    if (limit > 0)
    {
        limit--;
    }

    if (sizeLimit < limit)
    {
        limit = sizeLimit;
    }

    return n < 0 || (n > 0 && (uintmax_t)n > limit);
}

static int RangesOverlap(
    const mp_limb_t *left,
    mp_size_t leftCount,
    const mp_limb_t *right,
    mp_size_t rightCount)
{
    uintptr_t leftStart;
    uintptr_t rightStart;
    size_t leftBytes;
    size_t rightBytes;

    if (left == NULL || right == NULL || leftCount <= 0 || rightCount <= 0)
    {
        return 0;
    }

    leftStart = (uintptr_t)left;
    rightStart = (uintptr_t)right;
    leftBytes = (size_t)leftCount * sizeof(mp_limb_t);
    rightBytes = (size_t)rightCount * sizeof(mp_limb_t);
    return leftStart < rightStart + rightBytes && rightStart < leftStart + leftBytes;
}

#if GMP_LIMB_BITS == 64
static mp_limb_t ModAdd(mp_limb_t left, mp_limb_t right)
{
    unsigned __int128 sum = (unsigned __int128)left + right;

    if (sum >= Modulus)
    {
        sum -= Modulus;
    }

    return (mp_limb_t)sum;
}

static mp_limb_t ModSub(mp_limb_t left, mp_limb_t right)
{
    if (left >= right)
    {
        return left - right;
    }

    return (mp_limb_t)(Modulus - (right - left));
}

/* z mod P for z < 2^128, using 2^64 ≡ 2^32 - 1 (mod P). */
static mp_limb_t ModReduce(unsigned __int128 value)
{
    const mp_limb_t two32Minus1 = ((mp_limb_t)1 << 32) - 1;

    for (int step = 0; step < 3; step++)
    {
        mp_limb_t low = (mp_limb_t)value;
        mp_limb_t high = (mp_limb_t)(value >> 64);
        value = (unsigned __int128)low + (unsigned __int128)high * two32Minus1;
    }

    while (value >= Modulus)
    {
        value -= Modulus;
    }

    return (mp_limb_t)value;
}

static mp_limb_t ModMul(mp_limb_t left, mp_limb_t right)
{
    return ModReduce((unsigned __int128)left * right);
}

static mp_limb_t ModPow(mp_limb_t base, mp_limb_t exponent)
{
    mp_limb_t result = 1;

    while (exponent > 0)
    {
        if ((exponent & 1u) != 0)
        {
            result = ModMul(result, base);
        }

        base = ModMul(base, base);
        exponent >>= 1;
    }

    return result;
}
#endif

static void SchoolbookProduct(
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

static void OpenAiTeardown(void)
{
    free(digitsA);
    free(digitsB);
    free(scratchA);
    free(scratchB);
    free(bitReverse);
    digitsA = NULL;
    digitsB = NULL;
    scratchA = NULL;
    scratchB = NULL;
    bitReverse = NULL;
    transformSize = 0;
    digitCount = 0;
    preparedLimbs = -1;
    useSchoolbook = 0;
}

static void PrepareSchoolbook(mp_size_t n)
{
    useSchoolbook = 1;
    preparedLimbs = n;

    if (n > 0)
    {
        scratchA = CheckedAlloc((size_t)n, sizeof(mp_limb_t));
        scratchB = CheckedAlloc((size_t)n, sizeof(mp_limb_t));
    }
}

static void OpenAiSetup(mp_size_t n)
{
    OpenAiTeardown();

    if (n < 0)
    {
        Fail("negative length");
    }

    if (LengthIsUnsafe(n))
    {
        Fail("operand length is too large");
    }

    if (n == 0)
    {
        PrepareSchoolbook(0);
        return;
    }

#if GMP_LIMB_BITS != 64
    PrepareSchoolbook(n);
    return;
#else
    digitCount = (size_t)n * (size_t)OPENAI_DIGITS_PER_LIMB;

    unsigned __int128 coefficientBound =
        (unsigned __int128)digitCount * 65535u * 65535u;
    size_t size = 1;
    int tooLarge = coefficientBound >= Modulus;

    while (!tooLarge && size < 2 * digitCount)
    {
        if (size > ((size_t)1 << 28))
        {
            tooLarge = 1;
            break;
        }

        size <<= 1;
    }

    if (tooLarge)
    {
        PrepareSchoolbook(n);
        return;
    }

    useSchoolbook = 0;
    transformSize = size;
    preparedLimbs = n;
    digitsA = CheckedAlloc(transformSize, sizeof(mp_limb_t));
    digitsB = CheckedAlloc(transformSize, sizeof(mp_limb_t));
    bitReverse = CheckedAlloc(transformSize, sizeof(size_t));

    size_t logSize = 0;

    for (size_t cursor = transformSize; cursor > 1; cursor >>= 1)
    {
        logSize++;
    }

    for (size_t i = 0; i < transformSize; i++)
    {
        size_t reversed = 0;

        for (size_t bit = 0; bit < logSize; bit++)
        {
            if ((i & ((size_t)1 << bit)) != 0)
            {
                reversed |= (size_t)1 << (logSize - 1 - bit);
            }
        }

        bitReverse[i] = reversed;
    }
#endif
}

#if GMP_LIMB_BITS == 64
static void Transform(mp_limb_t *values, int invert)
{
    for (size_t i = 0; i < transformSize; i++)
    {
        size_t j = bitReverse[i];

        if (i < j)
        {
            mp_limb_t temp = values[i];
            values[i] = values[j];
            values[j] = temp;
        }
    }

    for (size_t length = 2; length <= transformSize; length <<= 1)
    {
        mp_limb_t root = ModPow(OPENAI_GENERATOR, (Modulus - 1) / (mp_limb_t)length);

        if (invert)
        {
            root = ModPow(root, Modulus - 2);
        }

        size_t half = length / 2;

        for (size_t start = 0; start < transformSize; start += length)
        {
            mp_limb_t twiddle = 1;

            for (size_t k = 0; k < half; k++)
            {
                size_t low = start + k;
                size_t high = low + half;
                mp_limb_t u = values[low];
                mp_limb_t v = ModMul(values[high], twiddle);

                values[low] = ModAdd(u, v);
                values[high] = ModSub(u, v);
                twiddle = ModMul(twiddle, root);
            }
        }
    }

    if (invert)
    {
        mp_limb_t inverseLength = ModPow((mp_limb_t)transformSize, Modulus - 2);

        for (size_t i = 0; i < transformSize; i++)
        {
            values[i] = ModMul(values[i], inverseLength);
        }
    }
}

static void LoadDigits(mp_limb_t *digits, const mp_limb_t *limbs, mp_size_t n)
{
    const mp_limb_t mask = ((mp_limb_t)1 << OPENAI_DIGIT_BITS) - 1;
    size_t index = 0;

    memset(digits, 0, transformSize * sizeof(mp_limb_t));

    for (mp_size_t limb = 0; limb < n; limb++)
    {
        for (int part = 0; part < OPENAI_DIGITS_PER_LIMB; part++)
        {
            digits[index++] = (limbs[limb] >> (part * OPENAI_DIGIT_BITS)) & mask;
        }
    }
}

static void StoreLimbs(mp_limb_t *result, const mp_limb_t *digits, mp_size_t n)
{
    const unsigned __int128 digitMask = ((mp_limb_t)1 << OPENAI_DIGIT_BITS) - 1;
    unsigned __int128 carry = 0;
    size_t index = 0;

    for (mp_size_t limb = 0; limb < 2 * n; limb++)
    {
        mp_limb_t word = 0;

        for (int part = 0; part < OPENAI_DIGITS_PER_LIMB; part++)
        {
            unsigned __int128 sum = (unsigned __int128)digits[index++] + carry;
            word |= (mp_limb_t)(sum & digitMask) << (part * OPENAI_DIGIT_BITS);
            carry = sum >> OPENAI_DIGIT_BITS;
        }

        result[limb] = word;
    }

    if (carry != 0)
    {
        fprintf(stderr, "OpenAI multiplication: carry escaped the 2n-limb product\n");
        exit(EXIT_FAILURE);
    }
}
#endif

static void custom_mul_n(
    mp_limb_t *result,
    const mp_limb_t *a,
    const mp_limb_t *b,
    mp_size_t n)
{
    if (n != preparedLimbs || (n > 0 && !useSchoolbook && digitsA == NULL))
    {
        Fail("setup does not match this call");
    }

    if (n <= 0)
    {
        return;
    }

    if (a == NULL || b == NULL || result == NULL)
    {
        Fail("null operand or result");
    }

    if (useSchoolbook)
    {
        const mp_limb_t *left = a;
        const mp_limb_t *right = b;

        if (RangesOverlap(result, 2 * n, a, n) || RangesOverlap(result, 2 * n, b, n))
        {
            if (scratchA == NULL || scratchB == NULL)
            {
                Fail("missing scratch for an overlapping product");
            }

            memcpy(scratchA, a, (size_t)n * sizeof(mp_limb_t));
            memcpy(scratchB, b, (size_t)n * sizeof(mp_limb_t));
            left = scratchA;
            right = scratchB;
        }

        SchoolbookProduct(result, left, right, n);
        return;
    }

#if GMP_LIMB_BITS == 64
    LoadDigits(digitsA, a, n);
    LoadDigits(digitsB, b, n);
    Transform(digitsA, 0);
    Transform(digitsB, 0);

    for (size_t i = 0; i < transformSize; i++)
    {
        digitsA[i] = ModMul(digitsA[i], digitsB[i]);
    }

    Transform(digitsA, 1);
    StoreLimbs(result, digitsA, n);
#else
    Fail("schoolbook path was not selected");
#endif
}

const MulAlgorithm OpenAiAlgorithm =
{
    "openai-nlogn",
    true,
    OpenAiSetup,
    custom_mul_n,
    OpenAiTeardown
};
