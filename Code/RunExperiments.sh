#!/usr/bin/env bash
#
# Reproduces every benchmark used in the report. Run on Linux with gcc, make
# and libgmp-dev installed:
#
#     bash Code/RunExperiments.sh [outputDir]
#
# Writes one CSV per experiment plus environment.txt into outputDir (default
# Results/Raw at the repository root). Then run
#
#     python3 Code/AnalyseResults.py
#
# to build Results/Tables.md and Results/Figures/.
#
# Methodology:
# - Each (algorithm, size, repeat) runs in its own Benchmark process seeded
#   with the repeat number, so for a given size and repeat every algorithm
#   multiplies the same operands, and one failing or slow algorithm cannot
#   affect the others.
# - The repeat loop is outermost, so slow drift in machine state (thermals,
#   background load) is spread across every algorithm instead of landing on one.
# - The process is pinned to one CPU with taskset when available (CPU=k picks it).
# - REPS (default 5) sets the repeats for the main experiments and
#   SAWTOOTH_REPS (default 3) for the fine grained Karatsuba size sweep.

set -euo pipefail

codeDir="$(cd "$(dirname "$0")" && pwd)"
outputDir="${1:-$codeDir/../Results/Raw}"
reps="${REPS:-5}"
sawtoothReps="${SAWTOOTH_REPS:-3}"
cpu="${CPU:-2}"
baseFlags="-O2 -std=gnu99 -Wall -Wextra"
buildDir="$(mktemp -d)"
trap 'rm -rf "$buildDir"' EXIT

mkdir -p "$outputDir"
: > "$outputDir/errors.log"

pin=()

if command -v taskset > /dev/null 2>&1
then
    pin=(taskset -c "$cpu")
fi

Log()
{
    echo "[$(date +%H:%M:%S)] $*" >&2
}

# Builds the benchmark into $1 with the Makefile, adding any further arguments to CFLAGS.
Build()
{
    local target="$1"
    shift
    make -B -s -C "$codeDir" TARGET="$target" CFLAGS="$baseFlags $*" > /dev/null
}

# Runs one Benchmark process and appends its rows, minus the header, to $2 with the column prefix $3.
Run()
{
    local binary="$1"
    local output="$2"
    local prefix="$3"
    local rows
    shift 3

    if rows="$("${pin[@]}" "$binary" "$@" 2>> "$outputDir/errors.log")"
    then
        tail -n +2 <<< "$rows" | sed "s/^/$prefix/" >> "$output"
    else
        echo "FAILED (prefix $prefix): $binary $*" >> "$outputDir/errors.log"
    fi
}

header="mode,algorithm,n_limbs,iterations,total_ms,per_op_us"
algorithms=(schoolbook karatsuba fft gmp)
threshold="$(sed -n 's/^#define KARATSUBA_THRESHOLD \([0-9]*\).*/\1/p' "$codeDir/KaratsubaMul.c")"

Build "$buildDir/Benchmark"

printf '#include <gmp.h>\n#include <stdio.h>\n\nint main(void)\n{\n    puts(gmp_version);\n    return 0;\n}\n' > "$buildDir/GmpVersion.c"
cc "$buildDir/GmpVersion.c" -o "$buildDir/GmpVersion" -lgmp

{
    echo "date=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "os=$(. /etc/os-release && echo "$PRETTY_NAME")"
    echo "kernel=$(uname -sr)"
    echo "cpu=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ *//')"
    echo "logical_cpus=$(nproc)"
    echo "compiler=$(cc --version | head -1)"
    echo "gmp=$("$buildDir/GmpVersion")"
    echo "cflags=$baseFlags"
    echo "pinned_cpu=${pin[*]:+$cpu}"
    echo "reps=$reps"
    echo "sawtooth_reps=$sawtoothReps"
    echo "karatsuba_threshold=$threshold"
    echo "warmup=3"
    echo "e2e_steps=20"
    echo "e2e_iterations=5"
} > "$outputDir/environment.txt"

# 1. Isolated multiplication, all algorithms, sizes spaced by factors of 1.5 and 2 up to 2^17 limbs.
#    Schoolbook stops at 32768 limbs, where one multiplication already takes about a second.
mainSizes=(8 12 16 24 32 48 64 96 128 192 256 384 512 768 1024 1536 2048 3072 4096 6144 8192 12288 16384 24576 32768 49152 65536 98304 131072)
declare -A maxIsolated=([schoolbook]=32768 [karatsuba]=131072 [fft]=131072 [gmp]=131072)
echo "rep,$header" > "$outputDir/isolated.csv"

for rep in $(seq 1 "$reps")
do
    Log "isolated: repeat $rep of $reps"

    for n in "${mainSizes[@]}"
    do
        for algorithm in "${algorithms[@]}"
        do
            if (( n <= maxIsolated[$algorithm] ))
            then
                Run "$buildDir/Benchmark" "$outputDir/isolated.csv" "$rep," --mode isolated --algos "$algorithm" --sizes "$n" --seed "$rep"
            fi
        done
    done
done

# 2. End to end modular squaring chain, all algorithms.
e2eSizes=(16 64 256 1024 4096 16384)
declare -A maxEndToEnd=([schoolbook]=4096 [karatsuba]=16384 [fft]=16384 [gmp]=16384)
echo "rep,$header" > "$outputDir/e2e.csv"

for rep in $(seq 1 "$reps")
do
    Log "e2e: repeat $rep of $reps"

    for n in "${e2eSizes[@]}"
    do
        for algorithm in "${algorithms[@]}"
        do
            if (( n <= maxEndToEnd[$algorithm] ))
            then
                Run "$buildDir/Benchmark" "$outputDir/e2e.csv" "$rep," --mode e2e --algos "$algorithm" --sizes "$n" --seed "$rep"
            fi
        done
    done
done

# 3. Fine schoolbook vs Karatsuba sweep around their crossover.
echo "rep,$header" > "$outputDir/crossover.csv"

for rep in $(seq 1 "$reps")
do
    Log "crossover: repeat $rep of $reps"

    for n in $(seq 16 2 96)
    do
        for algorithm in schoolbook karatsuba
        do
            Run "$buildDir/Benchmark" "$outputDir/crossover.csv" "$rep," --mode isolated --algos "$algorithm" --sizes "$n" --seed "$rep"
        done
    done
done

# 4. Karatsuba base case threshold sweep, powers of two and two sizes that are not.
thresholds=(1 2 4 6 8 10 12 14 16 20 24 28 32 40 48 64 96 128)
thresholdSizes=(64 256 1000 1024 3000 4096)

for t in "${thresholds[@]}"
do
    Build "$buildDir/Benchmark_T$t" "-DKARATSUBA_THRESHOLD=$t"
done

echo "threshold,rep,$header" > "$outputDir/threshold.csv"

for rep in $(seq 1 "$reps")
do
    Log "threshold: repeat $rep of $reps"

    for n in "${thresholdSizes[@]}"
    do
        for t in "${thresholds[@]}"
        do
            Run "$buildDir/Benchmark_T$t" "$outputDir/threshold.csv" "$t,$rep," --mode isolated --algos karatsuba --sizes "$n" --seed "$rep"
        done
    done
done

# 5. Fine grained Karatsuba size sweep at the default threshold, including both
#    sides of every point where the recursion gains a level (threshold * 2^k).
sawtoothSizes=($( { seq 16 8 2112; for k in 0 1 2 3 4 5 6 7; do echo $((threshold << k)); echo $(((threshold << k) + 1)); done; } | sort -n | uniq ))
echo "rep,$header" > "$outputDir/sawtooth.csv"

for rep in $(seq 1 "$sawtoothReps")
do
    Log "sawtooth: repeat $rep of $sawtoothReps"

    for n in "${sawtoothSizes[@]}"
    do
        Run "$buildDir/Benchmark" "$outputDir/sawtooth.csv" "$rep," --mode isolated --algos karatsuba --sizes "$n" --seed "$rep"
    done
done

Log "done; failures (if any) are in $outputDir/errors.log"
