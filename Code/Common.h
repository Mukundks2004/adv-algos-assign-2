#ifndef COMMON_H
#define COMMON_H

#include <gmp.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Shared radix: every algorithm operates on little endian arrays of
 * mp_limb_t, i.e. radix 2^GMP_LIMB_BITS (the native GMP limb radix).
 */

#if GMP_LIMB_BITS == 64 && defined(__SIZEOF_INT128__)
typedef unsigned __int128 DoubleLimb;
#elif GMP_LIMB_BITS == 32
typedef uint64_t DoubleLimb;
#else
#error "Unsupported limb size: need a type twice as wide as mp_limb_t"
#endif

typedef void (*SetupFunction)(mp_size_t n);
typedef void (*MulFunction)(mp_limb_t *result, const mp_limb_t *a, const mp_limb_t *b, mp_size_t n);
typedef void (*TeardownFunction)(void);

/*
 * Common scaffold shared by every implementation. Setup allocates all scratch
 * memory for operands of n limbs (called before timing), Mul is the
 * custom_mul_n entry point, Teardown releases the scratch memory.
 */
typedef struct MulAlgorithm
{
    const char *name;
    bool isImplemented;
    SetupFunction setup;
    MulFunction mul;
    TeardownFunction teardown;
} MulAlgorithm;

extern const MulAlgorithm SchoolbookAlgorithm;
extern const MulAlgorithm FftAlgorithm;
extern const MulAlgorithm KaratsubaAlgorithm;
extern const MulAlgorithm OpenAiAlgorithm;
extern const MulAlgorithm GmpAlgorithm;

#endif
