# Advanced Algorithms Assignment 2

## Overview

In groups of 2, you will pick an algorithmic problem and implement multiple genuinely different algorithms that solve it. You will then design and run a careful empirical comparison, write up what you find, produce a video presenting your work, and watch and respond to other groups' videos.

The point of the assignment is the comparison, not any individual implementation. Two algorithms that solve the same problem in different ways encode different ideas about the problem, and comparing them — both theoretically and empirically — is where the learning happens. A successful project identifies regimes where each algorithm wins, explains why in terms of the algorithms' structure, and ideally surfaces something the group didn't expect before they started.

The use of AI tools is permitted and expected, with the same disclosure requirements as Assignment 1. See the AI Use Policy.

## Choosing a Theme

Pick one of three themes for your comparison, or propose your own with approval.

1. Exact vs Approximate vs Heuristic for an NP-hard problem. Implement an exact algorithm (branch-and-bound, DP over subsets, or an ILP formulation using a solver), an approximation algorithm with a proven ratio, and (for groups of 3) a heuristic without proven guarantees. The intellectual content: where does the exact algorithm hit the wall, what does the approximation give up, where do heuristics shine despite their lack of guarantees, and how do these tradeoffs depend on input structure? Strong candidates: vertex cover, set cover, TSP (metric and general), max-cut, independent set, knapsack, graph coloring, scheduling problems, bin packing, max-SAT.

2. Deterministic vs Randomized. Implement a deterministic algorithm and a randomized algorithm (for groups of 3, two distinct randomized approaches or a Monte Carlo / Las Vegas pair) for the same problem. The intellectual content: randomized algorithms are often shorter, sometimes asymptotically faster, but require probabilistic analysis to trust. Where does the deterministic algorithm pay for its determinism? Where does the randomized algorithm pay for its randomness in tail behavior? Strong candidates: minimum cut (Karger vs Stoer-Wagner), primality testing (AKS vs Miller-Rabin), string matching (Karp-Rabin vs KMP), matrix product verification (Freivalds vs recomputation), MST (Karger-Klein-Tarjan randomized vs Kruskal/Prim), median finding (randomized vs deterministic linear-time selection).

3. Theoretical-Optimal vs Practical. Pick a problem where the asymptotically best known algorithm is wildly impractical and a simpler algorithm dominates in real-world use. Implement both (or several points along the spectrum) and find the crossover. The intellectual content: the gap between theory and practice is itself worth studying, and locating it empirically gives a feel for what theoretical complexity does and doesn't tell you. Strong candidates: integer multiplication (schoolbook vs Karatsuba vs FFT-based, comparing against GMP), matrix multiplication (naive vs Strassen, finding the crossover), maximum matching (naive augmenting paths vs Hopcroft-Karp), max-flow (Ford-Fulkerson vs Edmonds-Karp vs Dinic vs push-relabel), sorting (heavily studied; pick this only if you want to engage with the existing literature seriously), convex hull (Graham vs Jarvis vs Chan).

Or propose your own theme. The constraint is that the project must be a comparison of multiple algorithmic approaches to a single well-defined problem, where the comparison is intellectually substantive. Email me a one-paragraph description before committing.

## Group Structure and Attribution

Groups of 2 implement 2 algorithms; groups of 3 implement 3. Each algorithm has a primary author, named in the writeup and the repository. The primary author is responsible for that implementation and should be able to explain it in detail.

The empirical study, the analysis, the writeup, and the video are collaborative work, with all group members contributing meaningfully. Each primary author is specifically responsible for the segment of the video covering their own algorithm.

This attribution structure exists to prevent the failure mode where one strong student does everything. It is not a license to silo: you should review each other's code, help debug, and understand each other's algorithms well enough to ask good questions. But you are individually accountable for your own algorithm.

## Deliverables

1. A working repository containing all implementations, a unified benchmarking framework, scripts to reproduce the experiments, and a README with build instructions and how to run everything. Each implementation is in its own clearly-labeled module with its primary author named.

2. A written report of approximately 4000–6000 words, structured as described below.

3. A video presenting your work, with hard upper bounds on length (see Video section).

4. An individual reflection of approximately 500 words from each group member, submitted alongside the group artifacts.

5. Peer viewing responses: short written responses to 3 other groups' videos, with structure described below.

## Individual Reflection

Each group member submits a separate document of approximately 500 words covering, specifically about their own algorithm:

What's the worst case for your algorithm, and what kind of input triggers it?

What was the hardest bug or design decision you encountered, and how did you resolve it?

If you did this implementation again from scratch, what would you do differently?

What's one thing about your algorithm that you didn't fully understand before starting, and now do?

This document is yours individually. It is graded on specificity and evident understanding, not on length or polish. Generic answers receive low marks; specific concrete answers receive high marks. This is the place where your individual engagement with your algorithm becomes visible to me.

## AI Use Policy

Same as Assignment 1: AI tools are permitted and expected, with mandatory disclosure. The disclosure requirements are slightly different in groups:

Per-member disclosure of tool use: each group member writes a short paragraph describing how they personally used AI tools during the project.

At least three specific examples across the group of where AI was wrong, unhelpful, or misleading, and what the group did about it. Group projects involve more complex coordination work — designing benchmarking infrastructure, integrating implementations, producing publication-quality plots — and AI tools tend to fail in distinctive ways on this kind of work. If your group can't find three examples across all members, you are either not using the tools much or not checking their output.

A reflection on AI in group work specifically: where did AI help the group function (e.g. explaining one member's algorithm to another, helping resolve technical disagreements, generating boilerplate)? Where did it hurt or threaten to (e.g. tempting members to skip understanding each other's work, producing convincing-looking analyses that turned out to be wrong)? This is a research question the field hasn't answered well yet, and your honest experience is useful data.

## Video

You will produce a video presenting your work, posted as an unlisted YouTube link (or submitted as a video file if you prefer not to use YouTube). The video is the main public artifact of your work; it should make a viewer who hasn't read your report understand what you did and what you found.

Hard upper bounds on length:

Groups of 2: 12 minutes maximum.

Groups of 3: 15 minutes maximum.

These are hard caps. Video submissions exceeding the limit by even a small amount will be penalized — the cap is part of the assignment, and respecting it is part of doing the assignment well. Editing down is itself a useful skill: shorter videos are almost always better than longer ones because they force you to identify what actually matters. Don't pad to fill the time; if you can do it well in less, do it well in less.

Required structure:

Problem and algorithms overview (~2 minutes): what is the problem, what are the algorithms, why are you comparing them?

Per-algorithm deep dive segments: each primary author presents the trickiest or most interesting aspect of their own algorithm. Not just "here's what it does" but "here's the invariant, here's why this case is needed, here's what would break without it." This is the segment where individual understanding becomes most visible. Roughly 2–3 minutes per algorithm.

Results (~3–4 minutes): your most important empirical findings, with at least one plot or visualization. Focus on what's interesting — don't walk through every plot you produced, walk through the ones that tell the story.

The most surprising thing you found (~1–2 minutes): your headline surprise from the report. This is often the best moment of a presentation; give it the space it deserves.

Brief closing (~30 seconds): one sentence on what you'd do next.

Production guidance:

Audio quality matters; production polish does not. A clear voiceover with slides is excellent. A talking-head webcam recording with screen-share is excellent. Animated explainers are great if you enjoy making them, and absolutely not required.

The grading is on the content — clarity of exposition, depth of the technical content, how well the segments work together — not on visual production value beyond a minimum threshold ("audio is audible, visuals are legible, structure is clear"). Don't spend your time on After Effects when you should be running experiments.

The per-author deep dive is the part of the video most directly assessing individual understanding. Each primary author should record their own segment. If a primary author cannot competently explain the trickiest part of their own algorithm on video, that will be visible.

## Peer Viewing Responses

After all groups have submitted their videos, you will be assigned 3 other groups' videos to watch. For each, submit a short written response (~200 words each, so about 600 words total) addressing:

What problem did this group work on, and what algorithms did they compare?

What did they find that you thought was interesting or surprising?

What is one thing you would have done differently, or one question their work raised that they didn't answer?

The point is to ensure students actually engage with each other's work, to see the diversity of projects across the class, and to encourage groups to produce videos worth watching (since you know peers will watch them).

Watching the videos and writing the responses should take roughly 2 hours total.

## Grading

Implementation correctness and quality (20%): do the algorithms work, are they reasonably clean? Assessed per algorithm and attributed to the primary author, with group-level adjustments for the overall code quality (build system, repository organization, etc.).

Experimental methodology (15%): is the benchmarking infrastructure sound, are the inputs well-chosen, is the methodology defensible?

Analysis quality (20%): does the report connect observations back to the structure of the algorithms? Does it identify and explain non-obvious results?

"What surprised you" (10%): the depth of the surprises and the quality of the explanation.

Video (10%): clarity, structure, and especially the quality of the per-author deep-dive segments.

Individual reflection (5%, per member): specificity and evidence of individual engagement.

AI use disclosure (10%): honesty and specificity, per-member.

Peer viewing responses (5%): substantive engagement with the videos you watched.

Report quality (5%): structure, clarity, writing.

Individual grades within a group can differ. The default is that all group members get the same grade for the group-level components, but I will adjust based on individual contribution paragraphs, primary-author work, video deep-dive segments, and individual reflections. The individual reflection and the per-author video segment are the two main places where individual understanding is assessed; underperforming on these will lower an individual's grade regardless of the group's overall quality.

## Timeline

End of Week 1: Group composition and theme/problem chosen. Send a short email describing the problem, the algorithms you plan to implement, and the primary author for each. I'll respond with whether the scope is appropriate.

End of Week 2: At least one implementation per group member should be substantially working. Internal group check-in: are you on track, or do you need to descope?

End of Week 3: All implementations working. Benchmarking infrastructure built. Begin running experiments and drafting the report.

End of Week 4: Final experiments complete, report written, video produced, everything submitted. Plan to have experiments and a full report draft done a few days before the deadline, and reserve the final days for the video: recording, and especially editing, reliably takes longer than students expect — particularly when members film their segments separately and someone has to stitch them together. A group still running experiments two days before the deadline will submit a bad video.

Week 5: Video assignments for peer viewing go out shortly after submission; your three peer viewing responses are due one week later.
