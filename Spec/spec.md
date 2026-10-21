# Spec

## Task overview

The task is to write multiple implementations of integer multiplication algorithms in C and a benchmarking harness to be able to compare them. This is our submission for the assignment given in `overview.md`.

Please implement the following algorithms:

1. Schoolbook multiplication
2. FFT based integer multiplication

And write the scaffold but do not actually implement the following:

1. Karatsuba
2. `nlogn` integer multiplication authored by OpenAI, available here: `https://github.com/openai/math/tree/main/preprints/Integer-multiplication-below-n-log-n-September-23-2026`

The plan is to benchmark all of these algorithms against GMP. Ideally all 4 implementations should have the same scaffold and same interface.

## Interfaces

Please use the following common interface.

The signature of each multiplication function should be the following:

```c
void custom_mul_n(
    mp_limb_t *result,
    const mp_limb_t *a,
    const mp_limb_t *b,
    mp_size_t n
);
```

- The representation of an integer to multiply should be the type `mp_limb_t*`.
- `a` and `b` should each contain `n` limbs.
- `result` has capacity for `2 * n` limbs.
- Operands and output do not overlap.
- All buffers should be allocated prior to timing.
- Algorithms should use the same radix.

## Harness

A benchmarking harness should also be created. This should allow for two types of tests: algorithm benchmark for isolated multiplication performance and a true end to end benchmark in a real application.

The harness should do the following:

- Initialize inputs and export into little endian limb arrays.
- Force exactly n limbs (benchmark should be fixed size).
- Touch all pages (if necessary)
- Warm up both paths by running the algo a few times prior to benchmarking.
- Allocate memory if required.
- Verify that the results are equal prior to timing.
- Record start time, run algorithm many iterations, record finish time, calculate difference, and print it.

## Conventions

Use these naming conventions:

- Pascal case functions (aside from already implemented snake case functions), types, structs (if used) and files
- Camel case local variables
- Use multiline braces

## Folder structure

Please allocate one file per implementation. Keep the common headers/types in their own file. Keep the harness in its own file. The code should go inside the folder called `Code`.

Please write instructions to build and run benchmarking in the `README.md` in the root.
