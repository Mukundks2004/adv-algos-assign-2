# Mukund Reflection

My algorithm is the complex double precision FFT multiplier in `Code/mukund/FftMul.c`. It splits each operand into digits, convolves them with a radix-2 FFT, rounds, and carries.

## Worst case

There are two worst cases: one for speed and one for correctness.

**Speed.** `FftSetup` rounds the number of product digits (`2n * digitsPerLimb`) up to a power of two, so the cost is a staircase in `n`. It is not a smooth curve. The worst inputs are sizes just past a power-of-two boundary, where almost half the transform is zero padding. In `Results/Tables.md`, n = 2048 takes 832 us. Going to n = 3072 takes 3,828 us, which is 4.6x slower for 1.5x the data. The jump comes from two things at once. The digit width drops from 16 to 8 bits above 2048 limbs, which doubles the digit count. Then 3072 limbs rounds up to a 65,536-point transform. n = 4096 uses the same 65,536 points and takes 3,722 us, so 3072 pays the full price for 75% of the data. The same happens at 12288 vs 16384 (31.6 ms vs 29.5 ms).

**Correctness.** Rounding error grows with the size of the convolution coefficients. A coefficient can be as large as `digits * (2^digitBits)^2`. The worst input is one with every digit maximal, such as `2^(64n) - 1` (all limbs `0xFFFF...`). The harness only generates random operands with the top bit set, so it never tests that case. I have not tested the all-ones input, and the 16 vs 8 bit cutoff at 2048 limbs rests on a rough error bound. It is not a measured safety margin.

## Hardest design decision

The hardest decision was the digit width. A double has a 53-bit mantissa, and the final `floor(x * scale + 0.5)` is only correct if the accumulated FFT error stays under 0.5. 16-bit digits make the transform half as long and so are faster, but at 2048 limbs a coefficient can reach about 2^45 and the error gets close to the limit. 8-bit digits give a lot of headroom (about 2^33 at 16384 limbs) but double the transform length. I chose 16 bits up to `SMALL_OPERAND_LIMBS = 2048` and 8 bits above, and I accepted the performance cliff that comes with it. I relied on the harness check, which compares every result with `mpn_mul_n` before timing and exits on `MISMATCH`, to catch an unsafe choice. This is a safety net only for the inputs the harness actually generates.

A smaller design point was where to allocate memory. Twiddles, bit-reversal table and four buffers are built in `FftSetup`, outside the timer, so the benchmark measures the multiplication and not `cos`/`sin` calls and `calloc`.

## What I'd do differently

1. **Pack both operands into one complex FFT.** Currently I put `a` and `b` in separate arrays with a zero imaginary part, which wastes half of every transform. Putting `a` in the real part and `b` in the imaginary part gives both spectra from one forward FFT, which is about a third less work.
2. **Use signed (balanced) digits.** Digits in [-2^(b-1), 2^(b-1)) cut coefficient size and error, which would probably let the 16-bit digits go further than 2048 limbs.
3. **Handle the padding waste.** For example, use a mixed-radix size such as 3·2^k, so 3072 limbs don't pay for 4096.
4. **Fix the twiddle access.** `twiddleCos[k * step]` strides through memory in the early stages, which hurts cache behaviour at large n.
5. **Test the worst-case input** (all ones) explicitly.

## What I didn't understand before

I didn't understand why the FFT must be at least as long as the whole product. The FFT computes a *cyclic* convolution, so if the transform is shorter than the `2n * digitsPerLimb` product digits, the top coefficients wrap around and add into the low ones, silently corrupting the answer. That is why `fftSize` is the next power of two at or above the product digit count, and not the operand digit count. I now also understand why this rounding step works at all. The exact convolution is an integer vector, so the inverse FFT only has to land within 0.5 of the right integer. The algorithm is a numerical one that happens to give exact answers, and the digit-width limit exists because that guarantee has a precision budget.
