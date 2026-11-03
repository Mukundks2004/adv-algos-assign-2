# Integer Multiplication Benchmark

This project contains the code to compare integer multiplication algorithms against each other and the GMP.
All code can be found in `Code/`. The shared harness (`Harness.c`, `Common.h`, `GmpMul.c`, `Makefile`) is at the top level, and each algorithm is in its author's folder: `Code/mukund/` (schoolbook, FFT, FFT error probe), `Code/jack/` (Karatsuba and its test) and `Code/bailey/` (OpenAI preprint).

The following implementations have been included in our research:

- Schoolbook multiplication
- FFT multiplication
- Karatsuba
- Open AI's cutting edge research algorithm
- GMP multiplication

Also in this repo are shared contracts and types, and the benchmarking harness.

## Interface

Every implementation exposes the interface `custom_mul_n(result, a, b, n)` on little endian `mp_limb_t` arrays (radix is 2^`GMP_LIMB_BITS`- each limb represents a value in this base), plus setup/teardown functions so scratch memory (the high speed, temporary storage area that the process uses) is allocated and deallocated outside the timed region.

## Prerequisites

- C99 compiler (gcc/clang)
- `make`
- and GMP with headers (you can install this on ubuntu with `sudo apt install build-essential libgmp-dev`).

This repo has been implemented and tested on ubuntu 22.04 but you can also use WSL or MSYS2: `pacman -S mingw-w64-x86_64-gmp`

## Build

```sh
cd Code
make
```

## Run

The benchmarking function can be configured in many different ways, as shown below:

```sh
./Benchmark [--mode isolated|e2e|both] [--sizes 16,64,256,...] [--iters N] [--warmup W] [--steps S] [--seed X] [--algos schoolbook,karatsuba,fft,openai-nlogn,gmp]
```

- There are 3 benchmarking modes.
  - isolated mode generates random operands and measures raw mult speed. It calls `mul(result, a, b, n)` repeatedly on the same inputs.
  - e2e simulates a realistic workload; a chain of modular squarings. The time of this step includes `memcpy` and adds overhead, so it reduces the differences between algos.

- `sizes` is a csv for how long the operands are in limbs. The default is `16,64,256,1024,409`. Sweeping through these values show changes as the size of the limbs changes.

- `iters N` is how many timed repetitions there are to run.

- `warmup W` is how many untimed multiplications to do before starting.

- `steps S` is only for `e2e` and is how many chained modular multiplications should be done.

- `seed X` is the seed for random operand generation, default seed is `12345`.

- `algos` is a csv of which algos to run. The default is all of them. If they are not implemented they are skipped.

- The harness writes the results to stdout

Example: `./Benchmark --mode isolated --sizes 64,1024,16384 > results.csv`

## Some more info

The FFT uses 16-bit digits for n ≤ 2048 limbs and 8-bit digits above that to keep double precision rounding exact.

Karatsuba falls back to schoolbook at or below `KARATSUBA_THRESHOLD` limbs (default 16). To sweep it, rebuild with an override, e.g. `make -B CFLAGS="-O2 -std=gnu99 -Wall -Wextra -DKARATSUBA_THRESHOLD=8"` (`-B` is needed because the Makefile does not track CFLAGS).

## Reproduce the report's experiments

Every measurement in the report comes from one script, and every table and figure from another (the analysis needs `python3-matplotlib`):

```sh
bash Code/RunExperiments.sh      # about 14 minutes; writes Results/Raw/*.csv
python3 Code/AnalyseResults.py   # writes Results/Tables.md and Results/Figures/
```

`Results/README.md` describes the methodology, the machine the recorded results came from, and the findings.
