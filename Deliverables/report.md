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
