# Video cue sheet

Group of three, hard cap 15 minutes for the whole thing. This is a cue sheet, not a script to read word for word. Bailey records the OpenAI part himself.

## Open (about 1 minute, whoever is starting)

We multiply big integers and compare a few ways of doing it. Schoolbook, an FFT, and the OpenAI preprint from 23 September 2026, timed with GMP in the same harness. Karatsuba is still a stub, so it is not in the times. Theme is the usual one: the paper with the best big-O is not the thing you would actually call.

## Schoolbook (Mukund, about 2 minutes)

Show `Code/SchoolbookMul.c`, the double loop.

Say: every limb of `a` multiplies every limb of `b`, and the product lands at index i+j. The carry is the high half of a double-width product. If you drop the carry, the low limb looks fine and the next limb is short. That is the whole algorithm. It uses no extra buffers. It wins while n is small because there is nothing else to do. It is Theta(n^2), so it falls off once the transforms have paid for their setup.

## FFT (Mukund, about 2 minutes)

Show `Code/FftMul.c`, `digitBits` and `LoadDigits`.

Say: the number is a polynomial in 2^16, or 2^8 once n is over 2048 limbs. The FFT turns multiplication into a pointwise product. The inverse FFT plus a carry puts the limbs back. The 8-bit switch is there because double precision stops being exact if the digits are wide and n is large. If that rounding is off by one, the carry builds the wrong integer. We checked it against schoolbook through 4096 limbs, including the 8-bit sizes.

## OpenAI (Bailey, about 2 to 3 minutes)

Slides are optional. The code is enough: `Code/OpenAiMul.c`.

The preprint's theorem is a multitape bound, with kappa = 2^(-182). The paper uses schoolbook below a cutoff past log n = 2^(2^75). We do not run that.

The function that is timed splits each 64-bit limb into four 16-bit digits, convolves them mod 2^64 - 2^32 + 1, and carries in base 2^16.

The break case. Digits 3 and 5, times 7 and 11. Places are 21, then 68, then 55. Length 2 adds 55 onto 21 and gives 76. That is a different integer. The length has to be at least twice the digit count, and the inverse transform has to divide by that length. I checked the function against schoolbook from 1 limb through 4096, including all-ones and an in-place square.

## Times (about 2 minutes)

Table is in `Deliverables/report.md` and `Deliverables/local-timings.csv`. Harness, isolated mode, checked against GMP before the clock started.

Schoolbook won among our three at 16 and 64 limbs. GMP won those too, 0.169 us against 0.240 us for schoolbook at 16 limbs. I expected my code to pass schoolbook by 1024 limbs. At 1024, schoolbook took 969 microseconds and mine took 2192. Schoolbook was still ahead at 2048, 3970 against 4592. Mine was ahead at 4096, 10230 against 15456. The FFT passed schoolbook at 512 limbs, not at 256. At 4096 the FFT took 5252 microseconds and GMP took 637.

On the sizes we ran, GMP was the fastest. The FFT was the fastest of the three we wrote.

## Close (20 seconds)

Next is to move the twiddle powers out of the timed call, and put Karatsuba on the same plot once it exists.
