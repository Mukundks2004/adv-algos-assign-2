#include "Common.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * FFT based multiplication using a complex double precision FFT.
 * Each operand is split into small digits (16 bits for small n, 8 bits for
 * large n to keep rounding error under control), the digit sequences are
 * convolved via FFT, and the exact integer convolution is carried back into
 * limbs.
 */

#define SMALL_OPERAND_LIMBS 2048
#define MAX_OPERAND_LIMBS (1 << 22)

static double *aReal = NULL;
static double *aImag = NULL;
static double *bReal = NULL;
static double *bImag = NULL;
static double *twiddleCos = NULL;
static double *twiddleSin = NULL;
static size_t *bitReverse = NULL;
static size_t fftSize = 0;
static int digitBits = 0;
static int digitsPerLimb = 0;

static void *CheckedAlloc(size_t count, size_t size)
{
    void *pointer = calloc(count, size);

    if (pointer == NULL)
    {
        fprintf(stderr, "FFT multiplication: out of memory\n");
        exit(EXIT_FAILURE);
    }

    return pointer;
}

static void FftTeardown(void)
{
    free(aReal);
    free(aImag);
    free(bReal);
    free(bImag);
    free(twiddleCos);
    free(twiddleSin);
    free(bitReverse);
    aReal = aImag = bReal = bImag = twiddleCos = twiddleSin = NULL;
    bitReverse = NULL;
    fftSize = 0;
}

static void FftSetup(mp_size_t n)
{
    const double pi = 3.14159265358979323846264338327950288;

    FftTeardown();

    if (n > MAX_OPERAND_LIMBS)
    {
        fprintf(stderr, "FFT multiplication: operand too large for double precision FFT\n");
        exit(EXIT_FAILURE);
    }

    digitBits = n <= SMALL_OPERAND_LIMBS ? 16 : 8;
    digitsPerLimb = GMP_LIMB_BITS / digitBits;

    size_t productDigits = (size_t)(2 * n) * (size_t)digitsPerLimb;
    size_t logSize = 1;

    fftSize = 2;
    while (fftSize < productDigits)
    {
        fftSize <<= 1;
        logSize++;
    }

    aReal = CheckedAlloc(fftSize, sizeof(double));
    aImag = CheckedAlloc(fftSize, sizeof(double));
    bReal = CheckedAlloc(fftSize, sizeof(double));
    bImag = CheckedAlloc(fftSize, sizeof(double));
    twiddleCos = CheckedAlloc(fftSize / 2, sizeof(double));
    twiddleSin = CheckedAlloc(fftSize / 2, sizeof(double));
    bitReverse = CheckedAlloc(fftSize, sizeof(size_t));

    for (size_t i = 0; i < fftSize / 2; i++)
    {
        double angle = 2.0 * pi * (double)i / (double)fftSize;
        twiddleCos[i] = cos(angle);
        twiddleSin[i] = sin(angle);
    }

    for (size_t i = 0; i < fftSize; i++)
    {
        size_t reversed = 0;

        for (size_t bit = 0; bit < logSize; bit++)
        {
            if (i & ((size_t)1 << bit))
            {
                reversed |= (size_t)1 << (logSize - 1 - bit);
            }
        }

        bitReverse[i] = reversed;
    }
}

/* In place iterative radix-2 FFT. direction is -1 for forward, +1 for inverse (unscaled). */
static void Transform(double *real, double *imag, int direction)
{
    for (size_t i = 0; i < fftSize; i++)
    {
        size_t j = bitReverse[i];

        if (i < j)
        {
            double temp = real[i];
            real[i] = real[j];
            real[j] = temp;
            temp = imag[i];
            imag[i] = imag[j];
            imag[j] = temp;
        }
    }

    for (size_t length = 2; length <= fftSize; length <<= 1)
    {
        size_t half = length / 2;
        size_t step = fftSize / length;

        for (size_t start = 0; start < fftSize; start += length)
        {
            for (size_t k = 0; k < half; k++)
            {
                double wReal = twiddleCos[k * step];
                double wImag = direction * twiddleSin[k * step];
                size_t low = start + k;
                size_t high = low + half;
                double tReal = real[high] * wReal - imag[high] * wImag;
                double tImag = real[high] * wImag + imag[high] * wReal;

                real[high] = real[low] - tReal;
                imag[high] = imag[low] - tImag;
                real[low] += tReal;
                imag[low] += tImag;
            }
        }
    }
}

static void LoadDigits(double *real, double *imag, const mp_limb_t *limbs, mp_size_t n)
{
    mp_limb_t mask = ((mp_limb_t)1 << digitBits) - 1;
    size_t index = 0;

    for (mp_size_t i = 0; i < n; i++)
    {
        for (int k = 0; k < digitsPerLimb; k++)
        {
            real[index++] = (double)((limbs[i] >> (k * digitBits)) & mask);
        }
    }

    for (; index < fftSize; index++)
    {
        real[index] = 0.0;
    }

    for (size_t i = 0; i < fftSize; i++)
    {
        imag[i] = 0.0;
    }
}

static void custom_mul_n(
    mp_limb_t *result,
    const mp_limb_t *a,
    const mp_limb_t *b,
    mp_size_t n)
{
    LoadDigits(aReal, aImag, a, n);
    LoadDigits(bReal, bImag, b, n);

    Transform(aReal, aImag, -1);
    Transform(bReal, bImag, -1);

    for (size_t i = 0; i < fftSize; i++)
    {
        double real = aReal[i] * bReal[i] - aImag[i] * bImag[i];
        double imag = aReal[i] * bImag[i] + aImag[i] * bReal[i];
        aReal[i] = real;
        aImag[i] = imag;
    }

    Transform(aReal, aImag, 1);

    uint64_t mask = ((uint64_t)1 << digitBits) - 1;
    uint64_t carry = 0;
    double scale = 1.0 / (double)fftSize;
    size_t index = 0;

    for (mp_size_t i = 0; i < 2 * n; i++)
    {
        mp_limb_t limb = 0;

        for (int k = 0; k < digitsPerLimb; k++)
        {
            double value = floor(aReal[index++] * scale + 0.5);
            uint64_t coefficient = value > 0.0 ? (uint64_t)value : 0;
            uint64_t sum = coefficient + carry;

            limb |= (mp_limb_t)(sum & mask) << (k * digitBits);
            carry = sum >> digitBits;
        }

        result[i] = limb;
    }
}

const MulAlgorithm FftAlgorithm =
{
    "fft",
    true,
    FftSetup,
    custom_mul_n,
    FftTeardown
};
