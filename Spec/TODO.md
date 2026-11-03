# TODO

Here are the parts of the spec left to do:

## Deliverables

1. A working repository containing all implementations, a unified benchmarking framework, scripts to reproduce the experiments, and a README with build instructions and how to run everything. Each implementation is in its own clearly-labeled module with its primary author named. [DONE]

2. A written report of approximately 4000–6000 words, structured as described below. [IN_PROGRESS]

3. A video presenting your work, with hard upper bounds on length (see Video section). [IN_PROGRESS]

4. An individual reflection of approximately 500 words from each group member, submitted alongside the group artifacts. [IN_PROGRESS]

5. Peer viewing responses: short written responses to 3 other groups' videos, with structure described below. [TODO]

## Report Breakdown

1. The problem and the algorithms. State the problem precisely. Describe each algorithm at a level that someone in the class who hasn't seen it could understand — the key idea, the complexity, and what makes it different from the others.

2. Theoretical comparison and predictions. Before presenting any measurements, compare the algorithms on paper: their asymptotic complexities, their constant-factor and memory characteristics as far as you can reason about them, and the input features each one should be sensitive to. Then commit to predictions: which algorithm should win in which regime, and where should any crossovers fall? Write these down as what you believed before running the experiments, and don't revise them afterwards to look smarter — the gap between this section and your results is where the interesting material in sections 4 and 5 comes from.

3. Experimental methodology. What you measured and how. Describe your input families and how you generated them (sizes, structural parameters, and why these inputs are the right ones for probing the differences between the algorithms), your benchmarking setup (hardware, language/compiler and flags, timing method, how many repetitions, how you handled variance and warm-up), and what you did to make the comparison fair — same input instances for every algorithm, comparable levels of implementation effort, measuring the algorithms rather than the I/O around them. The standard here is that a skeptical reader could rerun your experiments from this section plus the repository and expect to see your plots reproduced.

4. Results and analysis. Plots, tables, and prose. The prose is the important part: what do the results show, and why? Connect what you observe back to the structure of the algorithms. If algorithm A beats algorithm B on dense graphs and loses on sparse graphs, say why in terms of what the algorithms are actually doing, not just that the asymptotic complexities differ.

5. What surprised you. At least one result you didn't expect before running it, and your best explanation. Specific examples carry far more weight than general statements. This section is heavily weighted. If your group genuinely engaged with the work, you will have surprises; if you can't find any, look harder.

6. What we'd do next. What's the natural extension of this project? What questions did the work open up that you didn't have time to answer?

7. Individual contributions. A short paragraph per group member describing what they personally worked on, beyond their primary algorithm. This is for assessment fairness; be honest. A group where one member did 60% of the work is not unusual and not catastrophic, but it should be visible to me.

8. AI use. Described in the next section.
