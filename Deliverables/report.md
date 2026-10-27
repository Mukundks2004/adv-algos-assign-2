# Comparison and Benchmarking of Popular Integer Multiplication Algorithms

## Spec

1. The problem and the algorithms. State the problem precisely. Describe each algorithm at a level that someone in the class who hasn't seen it could understand — the key idea, the complexity, and what makes it different from the others.

2. Theoretical comparison and predictions. Before presenting any measurements, compare the algorithms on paper: their asymptotic complexities, their constant-factor and memory characteristics as far as you can reason about them, and the input features each one should be sensitive to. Then commit to predictions: which algorithm should win in which regime, and where should any crossovers fall? Write these down as what you believed before running the experiments, and don't revise them afterwards to look smarter — the gap between this section and your results is where the interesting material in sections 4 and 5 comes from.

3. Experimental methodology. What you measured and how. Describe your input families and how you generated them (sizes, structural parameters, and why these inputs are the right ones for probing the differences between the algorithms), your benchmarking setup (hardware, language/compiler and flags, timing method, how many repetitions, how you handled variance and warm-up), and what you did to make the comparison fair — same input instances for every algorithm, comparable levels of implementation effort, measuring the algorithms rather than the I/O around them. The standard here is that a skeptical reader could rerun your experiments from this section plus the repository and expect to see your plots reproduced.

4. Results and analysis. Plots, tables, and prose. The prose is the important part: what do the results show, and why? Connect what you observe back to the structure of the algorithms. If algorithm A beats algorithm B on dense graphs and loses on sparse graphs, say why in terms of what the algorithms are actually doing, not just that the asymptotic complexities differ.

5. What surprised you. At least one result you didn't expect before running it, and your best explanation. Specific examples carry far more weight than general statements. This section is heavily weighted. If your group genuinely engaged with the work, you will have surprises; if you can't find any, look harder.

6. What we'd do next. What's the natural extension of this project? What questions did the work open up that you didn't have time to answer?

7. Individual contributions. A short paragraph per group member describing what they personally worked on, beyond their primary algorithm. This is for assessment fairness; be honest. A group where one member did 60% of the work is not unusual and not catastrophic, but it should be visible to me.

8. AI use. Described in the next section.

## Problem statement

The problem our team is solving is large integer multiplication, which is an essential arithmetic operation for a variety of purposes, such as cryptography (modular exponentiation in RSA) and blockchain (elliptic curve implementations). I don't think the problem can be stated more precisely than that without defining multiplication on the Peano axioms haha.

In the broader context of the assignment, the goal is to implement multiple algorithms that all multiply large integers and do a comparative study on why the performance of the algorithms for various arguments are what they are. In particular, we have chosen algorithms where the theoretical best algorithm (the one which is asymptotically the best) is impractical and a simpler one wins in real use- and we aim to prove this empirically.

The following algorithms have been implemented:

### Schoolbook

The key idea is to multiply every limb of `a` by every limb of `b` and add each partial product into the right position of the output- this mirrors how multiplication of multi-digit numbers is taught to primary school students. The time complexity of this operation is Theta(n^2) since there is a single nested loop driving the algorithm. It uses O(1) extra memory beyond the result. This algorithm is the only one of the implementations that does not use any complex construct such as recursion, transforms, or floating point arithmetic for integer multiplication. The tight, hyper minimal inner loop is the fastest option for small `n` but it scales very poorly.

### Karatsuba

- Key idea is to split each number into a high half anad a low half and compute the three cross terms with only 3 half size multiplications instead of 4. This uses the identity (a0+a1)(b0+b1) - a0a1 - b0b1 = a0b1 + a1b0.
- The recurrence relation can be calculated via masters theorem to give Theta(n^1.5) time complexity with O(n) space for sums and temporary data
- It calculates extra additions and has fewer multiplications. This and the recursion make it similar to Strassen's for matrix multiplication.
- This algo is faster beyond a few dozen limbs, and it does not need the mega FFT machinery
- It has not been implemented yet.

### FFT based

- The key idea is to treat the operands as digit polynomials. They are evaluated at the roots of unity via the FFT algorithm.
- `FftMul.c` chops each limb into 16-bit digits when `n` is at most 2048 limbs, and 8-bit digits above that, so the complex doubles stay close enough to integers that rounding lands on the exact digit.
- Forward FFT, pointwise product, inverse FFT, then carry the digits back into limbs. Scratch (twiddles, digit buffers) is allocated in setup, outside the timer.
- Time is O(n log n) arithmetic with a smaller constant than the modular transform below, because a double butterfly is cheaper than a multiply mod 2^64 - 2^32 + 1. Extra memory is a handful of length-2^k double arrays.

### OpenAI preprint (Bailey)

Primary author: Bailey Tucker (24744334), `Code/OpenAiMul.c`.

The preprint is OpenAI, "Integer multiplication below n log n", 23 September 2026. The theorem is a multitape bound, O(n (lg n)^(1-kappa)) with kappa = 2^(-182). Harvey and van der Hoeven had got to O(n log n). This paper says it saves a power of the log. The constants are ridiculous. The writeup tells the machine to use schoolbook below a cutoff, and one of the hypotheses already needs log2(n) past 2^(2^75). We cannot allocate that.

So the file does not time that tape machine. Copying schoolbook into it would also have made two copies of the same function. What it does is the product step from the paper, in a form we can check: split each limb into 16-bit digits, convolve them, carry in base 2^16. The convolution is a number-theoretic transform modulo P = 2^64 - 2^32 + 1. The length is at least twice the digit count, so the cyclic wrap does not land on a real coefficient. If the limb size is not 64 bits, or the length is too big for that prime, it falls back to schoolbook.

Quick check. Digits 3, 5 times 7, 11 give places 21, 68, 55. A cyclic length of 2 adds the 55 onto the 21 and the low digit becomes 76, which is a different integer. That is why the length check is there.

Null operands, a negative length, and a length that overflows the size math are rejected. An empty `n = 0` returns. Output can overlap an input because both operands are copied before the product is written.

## 2. Predictions, written before the timings

Leaving these as they were.

1. Schoolbook is the fastest of the three at 16 limbs and at 64 limbs. The inner loop is tiny. The NTT spends the call on modular multiplies.
2. The NTT is ahead of schoolbook by 1024 limbs, or if the constant is worse, by 4096.
3. The complex FFT is ahead of the NTT at every size we try. It is also ahead of schoolbook by 256 limbs.
4. Nothing in the plot looks like the paper's extra log-factor saving. This binary does not contain that saving.
5. At a fixed n, zeros, all-ones, and random limbs give the same ranking. None of these three skips a zero limb.

No prediction for Karatsuba. That file still aborts. No prediction against GMP either. GMP was not linked on the machine that produced the table.

## 3. How the times were taken

Same random limbs for all three algorithms. Graph generation is not in the timer. Setup runs first (FFT twiddles, NTT bit-reversal). Then one correctness call against schoolbook, then a median of five calls, or three calls at 2048 and 4096 limbs.

Machine: AMD Ryzen 7 6800H, 16 GB RAM, gcc 13.2.0, `-O2 -std=gnu99`. This MinGW install has no `libgmp`, so the table is not `./Benchmark` against `mpn_mul_n`. The products did match schoolbook, including all-ones, and schoolbook is the obvious reference. Rerun the harness on a box with GMP before this table is treated as the official one. Raw rows: `Deliverables/local-timings.csv`.

Sizes: 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 limbs. At 256 limbs we also timed all-zero and all-ones, just to see if the bit pattern moved the ranking.

## 4. Results

Median time in microseconds.

| limbs | schoolbook | fft | openai-nlogn |
|---:|---:|---:|---:|
| 16 | 0.400 | 2.500 | 24.200 |
| 32 | 1.000 | 5.500 | 42.200 |
| 64 | 4.200 | 12.000 | 88.200 |
| 128 | 16.600 | 26.300 | 194.100 |
| 256 | 65.100 | 58.900 | 420.200 |
| 512 | 258.900 | 145.200 | 920.400 |
| 1024 | 1035.500 | 404.400 | 1951.400 |
| 2048 | 4306.200 | 1000.000 | 4147.700 |
| 4096 | 17584.800 | 4725.600 | 9236.100 |

Prediction 1 held. At 16 limbs the times are 0.400, 2.500, 24.200 us. At 64 limbs they are 4.200, 12.000, 88.200 us.

Prediction 2 missed the hopeful half. At 1024 limbs schoolbook is still faster, 1035.500 us against 1951.400 us. The NTT is slightly ahead at 2048 (4147.700 vs 4306.200) and clearly ahead at 4096 (9236.100 vs 17584.800). So the crossover on this PC is between 1024 and 2048, and 2048 is close enough that another compiler could move it. The "by 4096" half of the prediction held.

Prediction 3 held. FFT beat the NTT on every row. It passed schoolbook at 256 limbs (58.900 vs 65.100). At 128 limbs schoolbook was still ahead of FFT (16.600 vs 26.300).

Prediction 4 held. At 4096 limbs FFT took 4725.600 us and the NTT took 9236.100 us. FFT is still about twice as fast. From 16 limbs to 4096 the NTT went from about 60 times schoolbook down to about half of schoolbook. That is the n log n shape showing up late. It is the modular convolution, not the paper's tape theorem.

Prediction 5, only checked at 256 limbs. Random: NTT 420.200 us, schoolbook 65.100 us. All zeros: NTT 320.100 us, schoolbook 65.500 us. All ones: NTT 346.900 us, schoolbook 65.500 us. Schoolbook stayed faster. The NTT time moved by about 100 us and the zero run was later, so some of that is cache. The ranking did not change.

The FFT column also matched schoolbook on every size in the table, including 4096 limbs where it has switched to 8-bit digits.

## 5. What surprised us

Schoolbook still being almost twice as fast at 1024 limbs. 1035 us against 1951 us. 1024 limbs felt "large enough" for an n log n convolution. The modular reduction, the twiddles being rebuilt inside the call, and four 16-bit digits per limb keep the constant high enough that the tight double loop wins through 1024 limbs.

The other one is the right-hand column. The OpenAI preprint is the asymptotically faster paper. In this process the FFT is the fastest of the three from 256 limbs on, and at 4096 limbs it is about twice the NTT and about 3.7 times schoolbook. The title of the preprint does not show up in the binary that was timed.

## 6. What we'd do next

Move the NTT twiddle powers into `OpenAiSetup` and time again. They are recomputed on every call, and the harness was written so that work can sit outside the timer. Then run the same sizes through `./Benchmark` with GMP linked, and add Karatsuba to the same rows once Jack's file runs. One more size, 8192 limbs, would show whether the NTT's lead over schoolbook keeps growing the way n log n versus n^2 should.

## 7. Who did what

Mukund set up the repo, the harness, schoolbook, the FFT, and the GMP wrapper. Jack has Karatsuba. That file still aborts, so there is nothing to time. Bailey wrote `OpenAiMul.c`, checked it against schoolbook (1 limb through 4096, all-ones, a second call on the same setup, and in-place squares), and took the local table. The group report still needs Jack's section and a GMP rerun. Bailey records the OpenAI segment. Schoolbook and FFT are Mukund's to explain.

## 8. AI use

I used Grok in Cursor for most of OpenAiMul.c. It read the preprint, wrote the first version of the file, and drafted some notes.

The first draft talked as if the timed function was the paper's O(n (lg n)^(1-kappa)) machine. That machine uses schoolbook under a cutoff past log2(n) = 2^(2^75). The function that was timed splits limbs into 16-bit digits and convolves them mod 2^64 - 2^32 + 1. I rewrote this section so the microseconds belong to that code.

The same draft treated my own PC's table as if it had come from the group harness against GMP. This PC has no libgmp. The products were checked against schoolbook.

I record the video myself. No generated voice. The cue sheet is a draft.

I understand the digit split, why the transform has to be at least twice as long as the digit list, and the carry back into limbs. What I am taking on trust is the proof that the full tape machine meets kappa = 2^(-182). I have not re-derived that proof.
