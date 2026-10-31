# Video script

Hard cap for a group of three is 15 minutes. Read straight through, this is about 11 minutes, so there is room for pauses. If you run long, keep the 8-bit sentence and the 2,097,152 row. Do not cut an invariant or a what-breaks line.

The Karatsuba cutoff numbers are the i9 runs with GMP 6.2.1. The other table is the Ryzen harness with GMP 6.3.0. Say the machine before the number.

Slides: `Deliverables/presentation.pptx`. The same words are in the slide notes.

## 0:00. Slide 1

We are comparing ways to multiply big integers. The problem is two n-limb numbers, 64 bits per limb, little endian, and a 2n-limb product. That is the same contract as GMP's mpn_mul_n.

## 0:20. Slide 2

We wrote schoolbook, Karatsuba, and an FFT, and one more file for the OpenAI digit convolution. Schoolbook is Theta of n squared. Karatsuba is about n to the 1.585. The FFT and the OpenAI function are n log n, with different constants. The preprint claims a slightly better exponent, and that machine is not what we time. GMP is the tuned library, and the baseline we check against. We are comparing them because the asymptotically faster code has the larger constant, and the question is where the simple loop still wins.

## 1:05. Slide 3

Schoolbook is the double loop in SchoolbookMul.c. Limb i of a multiplies every limb of b, and the product is added at index i plus j. The high half of that 128-bit product is the carry into the next limb. The invariant is that after limb i is finished, result[i + n] holds the carry out of that row, and every lower limb already includes the partial products that belong there. That carry is the case this loop needs. There is no extra buffer. It is Theta(n squared), so it wins only while n is small.

## 2:15. Slide 4

If you drop the carry, the low digits can still look plausible and the next limb is short by that carry. One limb of all ones, squared, has low limb 1 and high limb 0xFFFFFFFFFFFFFFFE. Throw the carry away and the low limb is still 1. The high limb is 0. That is a different integer.

## 2:45. Slide 5

Karatsuba splits each number in half and replaces four half-size products with three. This part is the invariant, the case that forces the difference form, and what breaks if the sign test is flipped.

## 3:05. Slide 6

The middle term is z0 plus z2 minus the product of the two differences. Each call makes three half-size calls, so the cost is about n to the 1.585. Below 16 limbs it calls the same schoolbook loop as the rest of the group.

## 3:50. Slide 7

The invariant is that every recursive product is square and strictly smaller than n. lowSize is n minus n over 2, and highSize is n over 2. Both are below n once n is at least 2. That is why it terminates, down to the schoolbook base case at 16 limbs. The scratch is about 4n limbs, allocated once in setup, outside the timer.

## 4:30. Slide 8

The textbook version adds the halves instead. That sum can grow an extra limb, and for n = 2 or 3 the recursion never shrinks. Take 999, split as 9 and 99. The sum is 108, which is three digits. The absolute difference is 90, which still fits in h digits. The cost of the difference is a sign.

## 5:05. Slide 9

If the two differences have the same sign you subtract, otherwise you add. Getting that case wrong still prints a normal-looking number. A sign-flip check failed 34,757 of 51,055 products. Sizes at most 16 limbs still all pass, because they are pure schoolbook, so a small test never reaches the bug. On one real 17-limb product, 6 limbs were right, then 19 were wrong, then 9 were right.

## 5:45. Slide 10

On my machine Karatsuba first beats schoolbook near 18 limbs, but only by a few percent until the gap opens up. The table shows schoolbook still ahead at 16 limbs and Karatsuba ahead at 24. A cutoff from 14 to 28 limbs is within about 3 percent of the best. Recursing all the way to one limb is about 4 to 6 times slower. From 256 limbs up, one extra limb at a recursion boundary moves the time by about 2 percent. The leaf work drops and the extra additions put it back.

## 6:30. Slide 11

The FFT treats the number as digits of a polynomial. The case that matters is the digit width. If a complex double rounds to the wrong integer, the carry builds the wrong product.

## 6:50. Slide 12

The invariant is that after the inverse FFT, each bucket rounds to the exact integer coefficient of the digit convolution. We use 16-bit digits while n is at most 2048 limbs, because a double can still land on that integer. Above 2048 the coefficient would leave the exact range of a double, so the case we need is 8-bit digits. If the rounding is off by one, the carry builds a different product. The length is padded up to a power of two, which is also why the time can sit flat and then jump. Setup allocates the twiddles and the digit buffers, outside the timer.

## 7:35. Slide 13

That padding is why the FFT time can sit flat across a band of sizes, and the 8-bit switch is why it jumps between 2048 and 3072 limbs on the i9 grid. At 2048 limbs the FFT took 832 microseconds. At 3072 it took 3,828. The size grew by half and the time grew by about 4.6 times.

## 8:10. Slide 14

This file is OpenAiMul.c. The invariant, the case that forces it, and what breaks if it is dropped. The preprint is a multitape bound with kappa equal to 2 to the minus 182. That machine uses schoolbook below a cutoff past log n equals 2 to the 2 to the 75. We do not run that machine.

## 8:30. Slide 15

Each 64-bit limb splits into four digits in base 2 to the 16, low digit first. The integer product is the linear convolution of those digits, then a carry back into limbs. I evaluate that convolution with a number-theoretic transform modulo 2 to the 64, minus 2 to the 32, plus 1. The length is the next power of two that is at least twice the digit count.

## 9:05. Slide 16

The invariant is one sentence. After the inverse transform, bucket k holds the exact sum of the digit products for shift k. A higher shift has not wrapped into it, and the sum is below the modulus. The while loop forces that length. The inverse then multiplies by the modular inverse of the length. The length puts the product in the right bucket. The multiply by the inverse brings the coefficient back to the integer sum.

## 9:40. Slide 17

Digits 3 and 5, times 7 and 11. The places are 21, then 68, then 55. If the length is only 2, the 55 wraps onto the 21 and the low digit is 76. That is a different integer. Length 4 leaves 21, 68, and 55 in their own places. This example needs no carry, so the carry is not what repairs a wrap.

## 10:05. Slide 18

Without the pad, the low digit is 76. Without the divide by the length, every coefficient is multiplied by the length. On this example that product is exactly four times the true value, and it is still a normal integer. Both failures are exact.

## 10:25. Slide 19

There are two experiments, and a viewer has to keep them apart. One is a Ryzen 7 6800H with GMP 6.3.0. All five algorithms go through 16,384 limbs, and Karatsuba, the FFT, the OpenAI function, and GMP continue through 2,097,152. The other is an i9-13900H with GMP 6.2.1, out to 131,072 limbs. Every product was checked against mpn_mul_n before the clock started. Setup runs first. The timed region is only the multiply. Say the machine before the number.

## 10:45. Slide 20

On my harness, GMP is fastest at every size through 2,097,152 limbs. At 16,384 I edge the FFT, 44.1 milliseconds against 47.5, and Karatsuba is still ahead at 18.4. At 131,072 I pass Karatsuba, 420 milliseconds against 505, and the FFT is still behind Karatsuba at 878. The FFT passes Karatsuba at 524,288 limbs, 4.08 seconds against 4.62. At 2,097,152 GMP takes 0.78 seconds, I take 8.24, the FFT takes 20.2, and Karatsuba takes 41.4. Above 2,048 limbs the FFT uses 8-bit digits, so its transform length is 16n. Mine stays 8n. That is about twice the arithmetic, and by 131,072 limbs the FFT takes about twice as long as mine. It does not catch me again. Each doubling, Karatsuba grows by about 3 and mine by about 2.1, which is why I pass Karatsuba and the longer FFT passes it later. GMP grows by about 2.1 as well, so the gap to GMP stays about 11 times. If you are short on time, keep the 8-bit sentence and the 2,097,152 row.

## 12:00. Slide 21

On my machine the FFT does not clearly pass Karatsuba until around 100,000 limbs. At 32,768 limbs Karatsuba took 54.8 milliseconds and the FFT took 63.7. At 65,536 they are close, 163 against 172, and the repeats overlap. At 131,072 the FFT is slightly ahead, 481 against 501. GMP stays ahead the whole way. The FFT pays for padding, for the 8-bit digit switch, and for growing faster than n log n in our data.

## 12:40. Slide 22

The surprise is schoolbook. I expected my convolution to pass it by 1024 limbs, and it was still behind at 2048. The modular multiplies and the twiddles rebuilt on every call keep the constant that high. I also expected the FFT to stay ahead. It does through 8192, and at 16384 mine is slightly ahead. That lead holds, because above 2048 limbs the FFT's transform is twice as long. Passing Karatsuba at 131072 was the one I did not trust from the small rows, and the doubling factors put it there. The FFT passes Karatsuba later, at 524288, which is later than on the i9 grid. GMP looked like the same crossing on the early graph. Past 32768 limbs it grows about as fast as mine, so the 11 times gap at 2097152 stays a gap. The preprint's speed is not what this binary measures.

On the i9 grid, the FFT was expected to pass Karatsuba inside the sizes we cared about. It was still behind at 32,768 limbs, and at 65,536 and 131,072 the repeats overlap.

## 13:30. Slide 23

Next for me is to build the twiddle powers in setup, outside the timer. That lowers the constant. It does not change the exponent. At 2,097,152 limbs GMP is still about 11 times faster, and the FFT, with twice the transform length, is about twice as slow as mine.
