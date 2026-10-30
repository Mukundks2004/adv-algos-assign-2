# Bailey Tucker (24744334)

About my file, `Code/OpenAiMul.c`.

## Worst case

The slow inputs are the long ones. Every n-limb multiply builds a transform of length at least 8n, four 16-bit digits per limb, then the next power of two. Zero limbs do not skip that. At 256 limbs an all-zero pair took 329.400 us, a random pair 435.300 us, and schoolbook on the zeros 55.800 us. Those three were checked with `mpn_mul_n`. Schoolbook stayed faster.

If a coefficient could reach the modulus 2^64 - 2^32 + 1, the function switches to the schoolbook loop. The preprint's fast tape machine only starts after log2(n) is past 2^(2^75). I cannot feed it an input that big.

## Hardest decision

What to put in the file. The preprint's fast path is a tape schedule with kappa = 2^(-182), and below its cutoff it says schoolbook. Pasting schoolbook into `OpenAiMul.c` would have matched that sentence and given us two identical algorithms.

I kept the product step, in a form I can check. Split into 16-bit digits, convolve, carry. The convolution is a number-theoretic transform, so the coefficients are exact residues rather than rounded doubles. The failure I kept in mind is a short cyclic length. Digits 3, 5 convolved with 7, 11 should be 21, 68, 55. Length 2 adds 55 into the low digit and yields 76. Setup refuses a length under twice the digit count.

I then compared the output with schoolbook for every n from 1 to 48, and at the sizes around powers of two through 4096, including all-ones, a second call on the same setup, and an in-place square. Those matched. A null pointer, a negative length, and a length that wraps the size math now exit with a message instead of crashing or returning success.

## What I would do differently

I would compute the twiddle powers in `OpenAiSetup`. They are rebuilt on every call, and the harness already times only `mul` after `setup`. The table in the report is that harness, checked against `mpn_mul_n`.

## What I didn't get at the start

I thought the preprint title meant this C function would pull away from the FFT once n was large. The O(n (lg n)^(1-kappa)) line is a tape-cost theorem. The writeup sends every length we can store to schoolbook, and the saving over O(n log n) is a power kappa = 2^(-182) on the log. The routine I can walk through is the digit convolution: the length stops the wrap, the inverse transform divides by the length, and the carry rebuilds the limbs. The 4096-limb harness row, 10230.032 us against 5251.702 us for the FFT, is that routine while the FFT is still ahead. From 8192 limbs up the same routine is faster than the FFT and schoolbook. At 16384 limbs it took 43446.160 us, the FFT took 65589.575 us, and GMP took 2902.526 us. That is about 15 times GMP. The tape theorem is a separate claim.
