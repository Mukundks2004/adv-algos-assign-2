# Video script

Hard cap for a group of three is 15 minutes. A group of two would be 12. Going to 17 would be over the cap. This script is written for about 14 minutes so there is a little room for pauses. If you run long, cut the result sentences, not the "what would break" lines.

Jack's Karatsuba numbers come from his i9 runs with GMP 6.2.1. Bailey's OpenAI table comes from the Ryzen harness with GMP 6.3.0. Say that out loud so the two plots are not treated as one experiment.

Slides: `Deliverables/presentation.pptx` and `Downloads/adv-algo-assign-2-slides.pptx`.

## 0:00 to 1:20. Mukund. Slides 1 to 3

We are comparing ways to multiply big integers. The problem is two n-limb numbers, 64 bits per limb, little endian, and a 2n-limb product. That is the same contract as GMP's mpn_mul_n.

We wrote schoolbook, Karatsuba, and an FFT, and Bailey wrote the OpenAI digit convolution. GMP is the library we check against and the practical baseline. The question is where the simple code wins, and where it does not.

## 1:20 to 2:40. Mukund. Slide 4

Schoolbook is the double loop in `SchoolbookMul.c`. Limb i of a multiplies every limb of b, and the product is added at index i plus j. The high half of that 128-bit product is the carry into the next limb.

The invariant is that after limb i is finished, result[i + n] holds the carry out of that row, and every lower limb already includes the partial products that belong there. If you drop the carry, the low digits can still look plausible and the next limb is short by that carry. There is no extra buffer. It is Theta(n squared), so it wins only while n is small.

## 2:40 to 5:20. Jack. Slides 5 and 6

Karatsuba splits each number in half and replaces four half-size products with three. The middle term is z0 plus z2 minus the product of the two differences. The textbook version adds the halves instead. That sum can grow an extra limb, and for n = 2 or 3 the recursion never shrinks. The difference always fits in h limbs. The cost is a sign. If the two differences have the same sign you subtract, otherwise you add. Getting that case wrong still prints a normal-looking number. Our tests hit both signs, a zero difference, odd n, and squaring in one buffer.

Below 16 limbs it calls the same schoolbook loop as the rest of the group. On my machine Karatsuba first beats schoolbook at 18 limbs, but only by a few percent until about 40. The cutoff from 14 to 28 limbs is within 3 percent of the best. Recursing all the way to one limb is 4 to 6 times slower. I expected a jump each time the recursion gained a level. From 256 limbs up, one extra limb at those boundaries moves the time by about 2 percent. The leaf work drops and the extra additions put it back.

## 5:20 to 7:20. Mukund. Slide 7

The FFT treats the number as digits of a polynomial. We use 16-bit digits, and 8-bit digits once n is over 2048 limbs, because the complex doubles have to round to the exact integer. If the rounding is off by one, the carry builds the wrong product. The length is padded up to a power of two. That padding is why the FFT time can sit flat across a band of sizes, and the 8-bit switch is why it jumps between 2048 and 3072 limbs on Jack's grid.

## 7:20 to 9:50. Bailey. Slides 8 and 9

My file is `OpenAiMul.c`. The preprint proves a multitape bound with kappa equal to 2 to the minus 182. That machine uses schoolbook below a cutoff past log n equals 2 to the 2 to the 75. We do not run that machine.

What I time splits each 64-bit limb into four 16-bit digits, convolves them modulo 2 to the 64 minus 2 to the 32 plus 1, and carries in base 2 to the 16.

The case that breaks is a short length. Digits 3 and 5, times 7 and 11, should land as 21, then 68, then 55. Length 2 adds 55 onto 21 and the low digit becomes 76. That is a different integer. The length has to be at least twice the digit count, and the inverse transform has to divide by that length. I checked the products against GMP before trusting the times.

## 9:50 to 12:40. Bailey, then Jack. Slides 10 to 12

On my harness, GMP is fastest at every size through 16384 limbs. Karatsuba is faster than mine at every size in this table. At 16 limbs GMP is 0.14 microseconds and schoolbook is 0.22. My code is 25. At 2048 limbs schoolbook is still ahead of mine, 3.6 milliseconds against 4.3. At 4096 mine passes schoolbook, 9.6 against 14.9, and the FFT is still faster at 4.9. At 8192 the FFT is still ahead, 11.4 against 20.1. At 16384 mine edges the FFT, 44.1 against 47.5. Karatsuba is 18.4, schoolbook is 233, and GMP is 2.6. We did not keep sizes past 16384. Mine does not catch GMP in this table. The gap there is about 17 times.

Jack, your grid is the other experiment. Karatsuba beats schoolbook from about 18 limbs, and on your machine the FFT does not clearly pass Karatsuba until around 100,000 limbs. Say the two machines out loud.

## 12:40 to 14:10. Jack, then Bailey. Slide 13

Jack: I expected the FFT to pass Karatsuba inside the sizes we cared about. It was still behind at 32,768 limbs, and at 65,536 and 131,072 the repeats overlap. The FFT pays for padding, for the 8-bit digit switch, and for growing faster than n log n in our data.

Bailey: I expected my convolution to pass schoolbook by 1024 limbs. It was still behind at 2048. I also expected the FFT to stay ahead of mine. It does through 8192, and at 16384 mine is slightly ahead. Karatsuba is ahead of both. GMP stays ahead the whole way. The preprint's speed is not what this binary measures.

## 14:10 to 14:40. Whoever is closing. Slide 14

Next for me is to build the twiddle powers in setup, outside the timer. Next for the group is Karatsuba on the same harness rows. We would not catch GMP by running a larger n on this code.
