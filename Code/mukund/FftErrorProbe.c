#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * Rounding error probe for the FFT multiplier. Run with: gcc -O2 Code/mukund/FftErrorProbe.c -lm -o FftErrorProbe && ./FftErrorProbe
 * Uses the same radix-2 transform and twiddle scheme as FftMul.c. For random and all-ones digits it prints the largest
 * distance of any convolution coefficient from an integer before rounding. Rounding is only correct while this stays below 0.5.
 */
static size_t N;
static double *twc, *tws;
static size_t *rev;

static void Setup(size_t n)
{
    N = n;
    size_t lg = 0;
    while (((size_t)1 << lg) < N) lg++;
    twc = malloc(N / 2 * sizeof(double));
    tws = malloc(N / 2 * sizeof(double));
    rev = malloc(N * sizeof(size_t));
    for (size_t i = 0; i < N / 2; i++)
    {
        double a = 2.0 * 3.14159265358979323846264338327950288 * (double)i / (double)N;
        twc[i] = cos(a);
        tws[i] = sin(a);
    }
    for (size_t i = 0; i < N; i++)
    {
        size_t r = 0;
        for (size_t b = 0; b < lg; b++) if (i & ((size_t)1 << b)) r |= (size_t)1 << (lg - 1 - b);
        rev[i] = r;
    }
}

static void Transform(double *re, double *im, int dir)
{
    for (size_t i = 0; i < N; i++)
    {
        size_t j = rev[i];
        if (i < j) { double t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
    }
    for (size_t len = 2; len <= N; len <<= 1)
    {
        size_t half = len / 2, step = N / len;
        for (size_t s = 0; s < N; s += len)
            for (size_t k = 0; k < half; k++)
            {
                double wr = twc[k * step], wi = dir * tws[k * step];
                size_t lo = s + k, hi = lo + half;
                double tr = re[hi] * wr - im[hi] * wi, ti = re[hi] * wi + im[hi] * wr;
                re[hi] = re[lo] - tr; im[hi] = im[lo] - ti;
                re[lo] += tr; im[lo] += ti;
            }
    }
}

static double Run(size_t limbs, int bits, int allOnes)
{
    size_t digits = limbs * (64 / bits), size = 2;
    while (size < 2 * digits) size <<= 1;
    Setup(size);
    double *ar = calloc(size, sizeof(double)), *ai = calloc(size, sizeof(double));
    double *br = calloc(size, sizeof(double)), *bi = calloc(size, sizeof(double));
    uint64_t mask = ((uint64_t)1 << bits) - 1;
    srand(12345);
    for (size_t i = 0; i < digits; i++)
    {
        uint64_t x = allOnes ? mask : ((((uint64_t)rand() << 30) ^ ((uint64_t)rand() << 15) ^ (uint64_t)rand()) & mask);
        uint64_t y = allOnes ? mask : ((((uint64_t)rand() << 30) ^ ((uint64_t)rand() << 15) ^ (uint64_t)rand()) & mask);
        ar[i] = (double)x; br[i] = (double)y;
    }
    Transform(ar, ai, -1);
    Transform(br, bi, -1);
    for (size_t i = 0; i < size; i++)
    {
        double r = ar[i] * br[i] - ai[i] * bi[i], m = ar[i] * bi[i] + ai[i] * br[i];
        ar[i] = r; ai[i] = m;
    }
    Transform(ar, ai, 1);
    double worst = 0.0, scale = 1.0 / (double)size;
    for (size_t i = 0; i < size; i++)
    {
        double v = ar[i] * scale, e = fabs(v - floor(v + 0.5));
        if (e > worst) worst = e;
    }
    free(ar); free(ai); free(br); free(bi); free(twc); free(tws); free(rev);
    return worst;
}

int main(void)
{
    size_t sizes[] = { 256, 1024, 2048, 4096, 8192, 16384, 32768 };
    printf("limbs,bits,random_maxerr,allones_maxerr\n");
    for (int b = 0; b < 2; b++)
        for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++)
        {
            int bits = b ? 8 : 16;
            printf("%zu,%d,%.3e,%.3e\n", sizes[s], bits, Run(sizes[s], bits, 0), Run(sizes[s], bits, 1));
            fflush(stdout);
        }
    return 0;
}
