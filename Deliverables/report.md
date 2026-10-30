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
- It has been implemented by Jack in `Code/KaratsubaMul.c`. His own study, on a different machine, is in `Results/Tables.md`. The table in section 4 is a later harness run on the Ryzen machine, with Karatsuba included.

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

No prediction against GMP. Karatsuba was not in the prediction list because it was not in that harness yet.

## 3. How the times were taken

`Code/Benchmark` in isolated mode, `--algos schoolbook,karatsuba,fft,openai-nlogn,gmp`, sizes 16 through 16384. This run is after Jack's Karatsuba landed on main, so all five are in one table.

The harness builds one random operand pair per size, with the top bit set so the value has n limbs. It checks the product against `mpn_mul_n` before timing. Setup runs first. The timed region is repeated `custom_mul_n` calls only. Iterations are chosen to land near 0.25 seconds. The table is the mean time per call, `per_op_us` in `Deliverables/local-timings.csv`.

Machine: AMD Ryzen 7 6800H, 16 GB RAM, gcc 13.2.0, `-O2 -std=gnu99`, GMP 6.3.0. Every row in that CSV passed the GMP check. If one had disagreed, the harness would have stopped. Jack's earlier Karatsuba study used a different machine and GMP 6.2.1. Those numbers stay in `Results/Tables.md`. They are not mixed into this table.

At 256 limbs, zeros, all-ones, and a random pair were timed separately, five calls, median, also checked with `mpn_mul_n`. That is not the same statistic as the harness mean, so those three sit in the text and not in the main table.

## 4. Results

Mean time per call, in microseconds, from the harness.

| limbs | schoolbook | karatsuba | fft | openai-nlogn | gmp |
|---:|---:|---:|---:|---:|---:|
| 16 | 0.222 | 0.227 | 2.647 | 24.686 | 0.144 |
| 32 | 0.901 | 0.762 | 5.821 | 42.764 | 0.468 |
| 64 | 3.534 | 2.440 | 12.456 | 88.562 | 1.481 |
| 128 | 14.414 | 7.716 | 27.427 | 200.772 | 4.483 |
| 256 | 58.287 | 23.871 | 62.834 | 454.577 | 13.189 |
| 512 | 231.648 | 73.165 | 140.320 | 935.582 | 34.202 |
| 1024 | 903.936 | 224.811 | 375.927 | 2052.912 | 88.575 |
| 2048 | 3631.455 | 681.333 | 854.749 | 4311.667 | 281.493 |
| 4096 | 14877.482 | 2212.154 | 4940.631 | 9600.569 | 535.282 |
| 8192 | 58602.350 | 6145.746 | 11376.371 | 20146.542 | 1238.075 |
| 16384 | 233051.500 | 18392.292 | 47480.480 | 44104.260 | 2593.490 |

GMP is the fastest on every row. At 16 limbs it is 0.144 us. At 16384 limbs it is 2593.490 us, and the OpenAI function is 44104.260 us, about 17 times slower.

Prediction 1 held for schoolbook against the FFT and the NTT. At 16 limbs schoolbook is 0.222 us, the FFT is 2.647 us, and the NTT is 24.686 us. At 64 limbs they are 3.534, 12.456, and 88.562 us. Karatsuba is already slightly ahead of schoolbook at 32 limbs (0.762 vs 0.901) and clearly ahead from 64 limbs (2.440 vs 3.534).

Prediction 2 missed the hopeful half. At 1024 limbs schoolbook is 903.936 us and the NTT is 2052.912 us. At 2048 limbs schoolbook is still ahead, 3631.455 us against 4311.667 us. The NTT is ahead at 4096 limbs, 9600.569 us against 14877.482 us. The crossover on this PC is between 2048 and 4096 limbs.

Prediction 3 held through 8192 limbs. The FFT is still faster there, 11376.371 us against 20146.542 us for the NTT. At 16384 limbs the NTT is slightly ahead, 44104.260 us against 47480.480 us. That is the last size in this table, and the margin is small. Schoolbook is still slightly ahead of the FFT at 256 limbs (58.287 vs 62.834) and the FFT is ahead at 512 limbs (140.320 vs 231.648).

Karatsuba is faster than the NTT at every size in this table, and faster than the FFT as well. At 16384 limbs Karatsuba takes 18392.292 us, the NTT 44104.260 us, and the FFT 47480.480 us.

Prediction 4 held. The small lead over the FFT at 16384 limbs is the 16-bit convolution against an 8-bit double FFT. It is not the paper's extra log-factor saving. GMP stays well ahead.

Prediction 5 held on the ranking. At 256 limbs, median of five calls, checked with `mpn_mul_n`. Zeros: NTT 329.400 us, schoolbook 55.800 us. All ones: NTT 354.900 us, schoolbook 55.500 us. Random: NTT 435.300 us, schoolbook 55.700 us. Schoolbook stayed faster. The NTT time moved by about 100 us. The ranking did not.

## 5. What surprised us

Schoolbook still being faster at 2048 limbs. 3631 us against 4312 us for the NTT. The modular reduction and the twiddles rebuilt inside the call keep the constant high enough that the schoolbook loop wins through 2048 limbs. The NTT pulls ahead at 4096, 9601 us against 14877 us.

On this run the FFT is still faster than the NTT at 8192 limbs, 11376 us against 20147 us. The first size where the NTT is ahead of the FFT is 16384 limbs, 44104 us against 47480 us, and the margin is small. Karatsuba is ahead of both there, at 18392 us. GMP wins every size. At 16384 limbs it takes 2593 us, about 17 times the NTT. We stopped at 16384 limbs. There is no row where the NTT catches GMP.

## 6. What we'd do next

Move the NTT twiddle powers into `OpenAiSetup` and time again. They are recomputed on every call, and the harness already times only `mul` after `setup`. Karatsuba is in this table now. Catching GMP would take a different implementation. At 16384 limbs the gap is about 17 times.

## 7. Who did what

Mukund set up the repo, the harness, schoolbook, the FFT, and the GMP wrapper. Jack implemented Karatsuba and the experiment pipeline in `Results/`. Bailey wrote `OpenAiMul.c`, checked it against `mpn_mul_n` through the harness, and took the combined table in `Deliverables/local-timings.csv` after merging Jack's code. Bailey records the OpenAI segment. Schoolbook and FFT are Mukund's to explain. Karatsuba is Jack's.

## 8. AI use

I used Grok in Cursor for most of OpenAiMul.c. It read the preprint, wrote the first version of the file, and drafted some notes.

The first draft talked as if the timed function was the paper's O(n (lg n)^(1-kappa)) machine. That machine uses schoolbook under a cutoff past log2(n) = 2^(2^75). The function that was timed splits limbs into 16-bit digits and convolves them mod 2^64 - 2^32 + 1. I rewrote this section so the microseconds belong to that code.

I record the video myself. No generated voice. The cue sheet is a draft.

I understand the digit split, why the transform has to be at least twice as long as the digit list, and the carry back into limbs. What I am taking on trust is the proof that the full tape machine meets kappa = 2^(-182). I have not re-derived that proof.
