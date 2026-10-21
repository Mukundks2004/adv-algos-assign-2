#include "Common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define MAX_SIZES 64
#define TARGET_SECONDS 0.25
#define MAX_ITERATIONS 1000000

typedef struct Options
{
    bool runIsolated;
    bool runEndToEnd;
    mp_size_t sizes[MAX_SIZES];
    int sizeCount;
    long iterations;
    int warmup;
    int steps;
    unsigned long seed;
} Options;

static const MulAlgorithm *const algorithms[] =
{
    &SchoolbookAlgorithm,
    &FftAlgorithm,
    &KaratsubaAlgorithm,
    &OpenAiAlgorithm,
    &GmpAlgorithm
};

static const int algorithmCount = (int)(sizeof(algorithms) / sizeof(algorithms[0]));

static double NowSeconds(void)
{
#ifdef _WIN32
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + (double)now.tv_nsec * 1e-9;
#endif
}

static void *CheckedAlloc(size_t count)
{
    void *pointer = malloc(count * sizeof(mp_limb_t));

    if (pointer == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    return pointer;
}

/* Random n limb operand with the top bit set so it has exactly n limbs, exported little endian. */
static void GenerateOperand(mp_limb_t *limbs, mp_size_t n, gmp_randstate_t state)
{
    mpz_t value;
    size_t written = 0;

    mpz_init(value);
    mpz_urandomb(value, state, (mp_bitcnt_t)n * GMP_LIMB_BITS);
    mpz_setbit(value, (mp_bitcnt_t)n * GMP_LIMB_BITS - 1);
    memset(limbs, 0, (size_t)n * sizeof(mp_limb_t));
    mpz_export(limbs, &written, -1, sizeof(mp_limb_t), 0, 0, value);
    mpz_clear(value);

    if (written != (size_t)n)
    {
        fprintf(stderr, "Operand export produced %zu limbs, expected %ld\n", written, (long)n);
        exit(EXIT_FAILURE);
    }
}

/* Writes and reads every page so first touch costs are paid before timing. */
static volatile mp_limb_t touchSink;

static void TouchPages(mp_limb_t *limbs, mp_size_t count)
{
    mp_limb_t sum = 0;

    memset(limbs, 0, (size_t)count * sizeof(mp_limb_t));

    for (mp_size_t i = 0; i < count; i += 256)
    {
        sum += limbs[i];
    }

    touchSink = sum;
}

static void PrintHeader(void)
{
    printf("mode,algorithm,n_limbs,iterations,total_ms,per_op_us\n");
}

static void PrintResult(const char *mode, const char *name, mp_size_t n, long iterations, double seconds)
{
    printf("%s,%s,%ld,%ld,%.6f,%.3f\n", mode, name, (long)n, iterations, seconds * 1e3, seconds * 1e6 / (double)iterations);
    fflush(stdout);
}

static void RunIsolated(const MulAlgorithm *algorithm, const Options *options, mp_size_t n, gmp_randstate_t state)
{
    mp_limb_t *a = CheckedAlloc((size_t)n);
    mp_limb_t *b = CheckedAlloc((size_t)n);
    mp_limb_t *result = CheckedAlloc((size_t)(2 * n));
    mp_limb_t *expected = CheckedAlloc((size_t)(2 * n));

    GenerateOperand(a, n, state);
    GenerateOperand(b, n, state);
    TouchPages(result, 2 * n);
    TouchPages(expected, 2 * n);

    algorithm->setup(n);
    mpn_mul_n(expected, a, b, n);

    for (int i = 0; i < options->warmup; i++)
    {
        algorithm->mul(result, a, b, n);
    }

    algorithm->mul(result, a, b, n);

    if (memcmp(result, expected, (size_t)(2 * n) * sizeof(mp_limb_t)) != 0)
    {
        fprintf(stderr, "MISMATCH: %s disagrees with GMP at n = %ld\n", algorithm->name, (long)n);
        exit(EXIT_FAILURE);
    }

    long iterations = options->iterations;

    if (iterations <= 0)
    {
        double calibrationStart = NowSeconds();
        algorithm->mul(result, a, b, n);
        double single = NowSeconds() - calibrationStart;
        double estimate = single > 0.0 ? TARGET_SECONDS / single : (double)MAX_ITERATIONS;
        iterations = estimate < 1.0 ? 1 : (estimate > MAX_ITERATIONS ? MAX_ITERATIONS : (long)estimate);
    }

    double start = NowSeconds();

    for (long i = 0; i < iterations; i++)
    {
        algorithm->mul(result, a, b, n);
    }

    double finish = NowSeconds();

    PrintResult("isolated", algorithm->name, n, iterations, finish - start);

    algorithm->teardown();
    free(a);
    free(b);
    free(result);
    free(expected);
}

/*
 * End to end application: a chain of modular squarings x <- x^2 + 1 modulo
 * 2^(64n) - 1 (the Fermat / Lucas-Lehmer style workload). Every squaring goes
 * through the multiplication under test; the fold of the 2n limb product back
 * to n limbs and the increment are done with GMP's low level add routines.
 */
static void SquareChain(const MulAlgorithm *algorithm, mp_limb_t *x, mp_limb_t *copy, mp_limb_t *product, mp_size_t n, int steps)
{
    for (int step = 0; step < steps; step++)
    {
        memcpy(copy, x, (size_t)n * sizeof(mp_limb_t));
        algorithm->mul(product, x, copy, n);

        mp_limb_t carry = mpn_add_n(x, product, product + n, n);

        while (carry != 0)
        {
            carry = mpn_add_1(x, x, n, carry);
        }

        carry = mpn_add_1(x, x, n, 1);

        while (carry != 0)
        {
            carry = mpn_add_1(x, x, n, carry);
        }
    }
}

static void RunEndToEnd(const MulAlgorithm *algorithm, const Options *options, mp_size_t n, gmp_randstate_t state)
{
    mp_limb_t *initial = CheckedAlloc((size_t)n);
    mp_limb_t *x = CheckedAlloc((size_t)n);
    mp_limb_t *copy = CheckedAlloc((size_t)n);
    mp_limb_t *product = CheckedAlloc((size_t)(2 * n));
    mp_limb_t *expected = CheckedAlloc((size_t)n);

    GenerateOperand(initial, n, state);
    TouchPages(product, 2 * n);
    TouchPages(copy, n);

    mp_limb_t *scratch = CheckedAlloc((size_t)(2 * n));
    TouchPages(scratch, 2 * n);

    memcpy(expected, initial, (size_t)n * sizeof(mp_limb_t));
    SquareChain(&GmpAlgorithm, expected, copy, scratch, n, options->steps);
    free(scratch);

    algorithm->setup(n);

    for (int i = 0; i < options->warmup; i++)
    {
        memcpy(x, initial, (size_t)n * sizeof(mp_limb_t));
        SquareChain(algorithm, x, copy, product, n, options->steps);
    }

    memcpy(x, initial, (size_t)n * sizeof(mp_limb_t));
    SquareChain(algorithm, x, copy, product, n, options->steps);

    if (memcmp(x, expected, (size_t)n * sizeof(mp_limb_t)) != 0)
    {
        fprintf(stderr, "MISMATCH: %s end to end result disagrees with GMP at n = %ld\n", algorithm->name, (long)n);
        exit(EXIT_FAILURE);
    }

    long iterations = options->iterations > 0 ? options->iterations : 5;
    double start = NowSeconds();

    for (long i = 0; i < iterations; i++)
    {
        memcpy(x, initial, (size_t)n * sizeof(mp_limb_t));
        SquareChain(algorithm, x, copy, product, n, options->steps);
    }

    double finish = NowSeconds();

    PrintResult("e2e", algorithm->name, n, iterations, finish - start);

    algorithm->teardown();
    free(initial);
    free(x);
    free(copy);
    free(product);
    free(expected);
}

static void PrintUsage(const char *program)
{
    fprintf(stderr,
        "Usage: %s [--mode isolated|e2e|both] [--sizes n1,n2,...] [--iters N] [--warmup W]\n"
        "          [--steps S] [--seed X] [--algos name1,name2,...]\n"
        "  --mode    isolated multiplication, end to end modular squaring chain, or both (default both)\n"
        "  --sizes   operand sizes in limbs (default 16,64,256,1024,4096)\n"
        "  --iters   timed iterations; 0 = auto for isolated (default), 5 for e2e\n"
        "  --warmup  warmup runs before timing (default 3)\n"
        "  --steps   squarings per end to end iteration (default 20)\n"
        "  --algos   subset of: schoolbook,fft,karatsuba,openai-nlogn,gmp (default all implemented)\n",
        program);
}

static void ParseSizes(Options *options, char *text)
{
    options->sizeCount = 0;

    for (char *token = strtok(text, ","); token != NULL && options->sizeCount < MAX_SIZES; token = strtok(NULL, ","))
    {
        long value = strtol(token, NULL, 10);

        if (value <= 0)
        {
            fprintf(stderr, "Invalid size: %s\n", token);
            exit(EXIT_FAILURE);
        }

        options->sizes[options->sizeCount++] = (mp_size_t)value;
    }
}

int main(int argc, char **argv)
{
    Options options;
    char algoFilter[256] = "";
    const mp_size_t defaultSizes[] = { 16, 64, 256, 1024, 4096 };

    options.runIsolated = true;
    options.runEndToEnd = true;
    options.sizeCount = 5;
    memcpy(options.sizes, defaultSizes, sizeof(defaultSizes));
    options.iterations = 0;
    options.warmup = 3;
    options.steps = 20;
    options.seed = 12345;

    for (int i = 1; i < argc; i++)
    {
        const char *flag = argv[i];

        if (i + 1 >= argc)
        {
            PrintUsage(argv[0]);
            return EXIT_FAILURE;
        }

        char *value = argv[++i];

        if (strcmp(flag, "--mode") == 0)
        {
            options.runIsolated = strcmp(value, "e2e") != 0;
            options.runEndToEnd = strcmp(value, "isolated") != 0;
        }
        else if (strcmp(flag, "--sizes") == 0)
        {
            ParseSizes(&options, value);
        }
        else if (strcmp(flag, "--iters") == 0)
        {
            options.iterations = strtol(value, NULL, 10);
        }
        else if (strcmp(flag, "--warmup") == 0)
        {
            options.warmup = (int)strtol(value, NULL, 10);
        }
        else if (strcmp(flag, "--steps") == 0)
        {
            options.steps = (int)strtol(value, NULL, 10);
        }
        else if (strcmp(flag, "--seed") == 0)
        {
            options.seed = strtoul(value, NULL, 10);
        }
        else if (strcmp(flag, "--algos") == 0)
        {
            snprintf(algoFilter, sizeof(algoFilter), ",%s,", value);
        }
        else
        {
            PrintUsage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    gmp_randstate_t state;
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, options.seed);

    PrintHeader();

    for (int s = 0; s < options.sizeCount; s++)
    {
        mp_size_t n = options.sizes[s];

        for (int a = 0; a < algorithmCount; a++)
        {
            const MulAlgorithm *algorithm = algorithms[a];
            char needle[64];

            snprintf(needle, sizeof(needle), ",%s,", algorithm->name);

            if (algoFilter[0] != '\0' && strstr(algoFilter, needle) == NULL)
            {
                continue;
            }

            if (!algorithm->isImplemented)
            {
                if (s == 0)
                {
                    fprintf(stderr, "Skipping %s: not implemented\n", algorithm->name);
                }

                continue;
            }

            if (options.runIsolated)
            {
                RunIsolated(algorithm, &options, n, state);
            }

            if (options.runEndToEnd)
            {
                RunEndToEnd(algorithm, &options, n, state);
            }
        }
    }

    gmp_randclear(state);
    return EXIT_SUCCESS;
}
