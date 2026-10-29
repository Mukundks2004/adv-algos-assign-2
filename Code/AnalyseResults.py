"""
Summarises the CSVs written by RunExperiments.sh into Markdown tables and
figures for the report:

    python3 Code/AnalyseResults.py [resultsDir]

Reads resultsDir/Raw/*.csv (default Results at the repository root) and
writes resultsDir/Tables.md and resultsDir/Figures/*.png. Requires matplotlib.
Every time below is the median over repeats of the time per operation,
computed from total_ms / iterations rather than the rounded per_op_us column.
"""

import csv
import math
import os
import statistics
import sys
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
from matplotlib.ticker import FixedLocator, FuncFormatter, NullLocator

ALGORITHMS = ["schoolbook", "karatsuba", "fft", "gmp"]
KARATSUBA_EXPONENT = math.log2(3)

# Categorical slots 1-4 of the validated reference palette, in fixed order; ordinal blue ramp for ordered sizes.
SERIES_COLOURS = {"schoolbook": "#2a78d6", "karatsuba": "#eb6834", "fft": "#1baf7a", "gmp": "#eda100"}
ORDINAL_BLUES = ["#86b6ef", "#3987e5", "#1c5cab", "#0d366b"]
SURFACE = "#fcfcfb"
INK = "#0b0b0b"
INK_SECONDARY = "#52514e"
INK_MUTED = "#898781"
GRIDLINE = "#e1e0d9"
BASELINE = "#c3c2b7"


def ReadRows(path):
    with open(path, newline="") as handle:
        return list(csv.DictReader(handle))


def ReadEnvironment(path):
    environment = {}

    with open(path) as handle:
        for line in handle:
            key, _, value = line.strip().partition("=")
            environment[key] = value

    return environment


def PerOpMicros(row):
    return float(row["total_ms"]) * 1000.0 / float(row["iterations"])


def GroupTimes(rows, keyFields):
    """Maps a tuple of the key fields to the list of per operation times across repeats."""
    groups = defaultdict(list)

    for row in rows:
        key = tuple(row[field] if field == "algorithm" else int(row[field]) for field in keyFields)
        groups[key].append(PerOpMicros(row))

    return groups


def Medians(groups):
    return {key: statistics.median(values) for key, values in groups.items()}


def Spread(values):
    """(max - min) / median across repeats."""
    return (max(values) - min(values)) / statistics.median(values)


def Correlation(xs, ys):
    meanX = statistics.mean(xs)
    meanY = statistics.mean(ys)
    covariance = sum((x - meanX) * (y - meanY) for x, y in zip(xs, ys))
    return covariance / math.sqrt(sum((x - meanX) ** 2 for x in xs) * sum((y - meanY) ** 2 for y in ys))


def BySize(medians, algorithm):
    return {n: value for (name, n), value in medians.items() if name == algorithm}


def FormatMicros(value):
    if value < 1:
        return f"{value:.3f}"

    if value < 10:
        return f"{value:.2f}"

    if value < 100:
        return f"{value:.1f}"

    return f"{value:,.0f}"


def FormatTimeTick(value, _position=None):
    if value < 1:
        return f"{value * 1000:g} ns"

    if value < 1000:
        return f"{value:g} µs"

    if value < 1e6:
        return f"{value / 1000:g} ms"

    return f"{value / 1e6:g} s"


def FormatSizeTick(value, _position=None):
    if value >= 1024 and value % 1024 == 0:
        return f"{int(value // 1024)}K"

    return f"{int(value)}"


def Crossovers(timesA, timesB):
    """Sizes where timesA and timesB cross, interpolated linearly in log n / log ratio space."""
    sizes = sorted(set(timesA) & set(timesB))
    crossings = []

    for low, high in zip(sizes, sizes[1:]):
        lowGap = math.log(timesA[low] / timesB[low])
        highGap = math.log(timesA[high] / timesB[high])

        if lowGap * highGap < 0:
            fraction = lowGap / (lowGap - highGap)
            crossing = math.exp(math.log(low) + fraction * (math.log(high) - math.log(low)))
            crossings.append((low, high, crossing, highGap < 0))

    return crossings


def FitExponent(times, minimumSize, maximumSize):
    """Least squares slope of log t against log n over the given size range."""
    points = [(math.log(n), math.log(t)) for n, t in times.items() if minimumSize <= n <= maximumSize]

    if len(points) < 2:
        return None

    meanX = statistics.mean(x for x, _ in points)
    meanY = statistics.mean(y for _, y in points)
    covariance = sum((x - meanX) * (y - meanY) for x, y in points)
    variance = sum((x - meanX) ** 2 for x, _ in points)
    return covariance / variance


def StyleAxes(axes):
    axes.set_facecolor(SURFACE)

    for side in ("top", "right"):
        axes.spines[side].set_visible(False)

    for side in ("left", "bottom"):
        axes.spines[side].set_color(BASELINE)
        axes.spines[side].set_linewidth(0.8)

    axes.tick_params(which="both", colors=BASELINE, labelcolor=INK_SECONDARY, labelsize=8.5)
    axes.grid(True, which="major", color=GRIDLINE, linewidth=0.6, linestyle="-")
    axes.set_axisbelow(True)
    axes.xaxis.label.set_color(INK_SECONDARY)
    axes.yaxis.label.set_color(INK_SECONDARY)


def NewFigure(title, subtitle):
    figure, axes = plt.subplots(figsize=(7.2, 4.6), dpi=200)
    figure.patch.set_facecolor(SURFACE)
    StyleAxes(axes)
    figure.text(0.012, 0.975, title, fontsize=11.5, fontweight="semibold", color=INK, va="top")
    figure.text(0.012, 0.928, subtitle, fontsize=8.5, color=INK_SECONDARY, va="top")
    return figure, axes


def AddLegend(figure, axes):
    """One row of legend entries under the subtitle, clear of the data."""
    handles, labels = axes.get_legend_handles_labels()
    figure.legend(handles, labels, loc="upper left", bbox_to_anchor=(0.006, 0.895), ncol=len(labels), frameon=False,
        fontsize=8.5, labelcolor=INK_SECONDARY, handlelength=1.6, columnspacing=1.6)


def AddEndLabels(figure, axes, ends):
    """Direct labels at line ends. Labels that would collide are dropped rather than stacked; the legend still names every series."""
    figure.tight_layout(rect=(0, 0, 1, 0.84))
    figure.canvas.draw()
    minimumGap = 11 * figure.dpi / 72
    pixelYs = [axes.transData.transform((x, y))[1] for _, x, y in ends]

    for index, (label, x, y) in enumerate(ends):
        if all(abs(pixelYs[index] - other) >= minimumGap for otherIndex, other in enumerate(pixelYs) if otherIndex != index):
            axes.annotate(label, (x, y), xytext=(6, 0), textcoords="offset points", fontsize=8.5, color=INK_SECONDARY, va="center")


def SaveFigure(figure, path, top=0.84):
    figure.tight_layout(rect=(0, 0, 1, top))
    figure.savefig(path, facecolor=SURFACE)
    plt.close(figure)


def PlotLine(axes, sizes, values, colour, label):
    axes.plot(sizes, values, color=colour, linewidth=1.5, solid_joinstyle="round", solid_capstyle="round",
        marker="o", markersize=4.5, markeredgecolor=SURFACE, markeredgewidth=1.0, label=label)


def LogSizeAxis(axes, sizes, every=2):
    axes.set_xscale("log", base=2)
    ticks = [2 ** k for k in range(int(math.log2(min(sizes))), int(math.log2(max(sizes))) + 1, every)]
    axes.xaxis.set_major_locator(FixedLocator(ticks))
    axes.xaxis.set_minor_locator(NullLocator())
    axes.xaxis.set_major_formatter(FuncFormatter(FormatSizeTick))
    axes.set_xlabel("operand size n (64-bit limbs)", fontsize=9)


def LogRatioAxis(axes, values, candidates):
    """Log y axis labelled at the candidate ratios that fall inside the data range."""
    axes.set_yscale("log")
    low = min(values)
    high = max(values)
    axes.yaxis.set_major_locator(FixedLocator([tick for tick in candidates if low / 1.3 <= tick <= high * 1.3]))
    axes.yaxis.set_minor_locator(NullLocator())
    axes.yaxis.set_major_formatter(FuncFormatter(lambda value, _position: f"{value:g}×"))


def PlotRuntime(isolatedMedians, path):
    figure, axes = NewFigure("Time per multiplication", "Isolated benchmark, median of repeats, log-log axes. Lower is faster.")
    allSizes = []
    ends = []

    for algorithm in ALGORITHMS:
        times = BySize(isolatedMedians, algorithm)
        sizes = sorted(times)
        allSizes += sizes
        PlotLine(axes, sizes, [times[n] for n in sizes], SERIES_COLOURS[algorithm], algorithm)
        ends.append((algorithm, sizes[-1], times[sizes[-1]]))

    LogSizeAxis(axes, allSizes)
    axes.set_xlim(right=max(allSizes) * 2.6)
    axes.set_yscale("log")
    axes.yaxis.set_minor_locator(NullLocator())
    axes.yaxis.set_major_formatter(FuncFormatter(FormatTimeTick))
    axes.set_ylabel("time per multiplication", fontsize=9)
    AddLegend(figure, axes)
    AddEndLabels(figure, axes, ends)
    SaveFigure(figure, path)


def PlotRelativeToGmp(isolatedMedians, path):
    figure, axes = NewFigure("Slowdown relative to GMP", "Time divided by GMP's time at the same size. Where two lines cross, the faster algorithm changes.")
    gmpTimes = BySize(isolatedMedians, "gmp")
    allSizes = []
    allRatios = []
    ends = []

    for algorithm in ["schoolbook", "karatsuba", "fft"]:
        times = BySize(isolatedMedians, algorithm)
        sizes = sorted(n for n in times if n in gmpTimes)
        allSizes += sizes
        ratios = [times[n] / gmpTimes[n] for n in sizes]
        allRatios += ratios
        PlotLine(axes, sizes, ratios, SERIES_COLOURS[algorithm], algorithm)
        ends.append((algorithm, sizes[-1], ratios[-1]))

    LogSizeAxis(axes, allSizes)
    axes.set_xlim(right=max(allSizes) * 2.6)
    LogRatioAxis(axes, allRatios, [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000])
    axes.set_ylabel("times slower than GMP", fontsize=9)
    AddLegend(figure, axes)
    AddEndLabels(figure, axes, ends)
    SaveFigure(figure, path)


def PlotThresholdSweep(thresholdMedians, path):
    figure, axes = NewFigure("Karatsuba base case threshold", "Time relative to the best threshold for each size (1× = best), log scale. Pure Karatsuba is threshold 1.")
    plottedSizes = [64, 256, 1024, 4096]
    thresholds = sorted({t for t, _ in thresholdMedians})
    allRatios = []

    for colour, n in zip(ORDINAL_BLUES, plottedSizes):
        values = {t: thresholdMedians[(t, n)] for t in thresholds if (t, n) in thresholdMedians}

        if not values:
            continue

        best = min(values.values())
        xs = sorted(values)
        ratios = [values[t] / best for t in xs]
        allRatios += ratios
        PlotLine(axes, xs, ratios, colour, f"n = {n}")

    axes.set_xscale("log", base=2)
    axes.xaxis.set_major_locator(FixedLocator([1, 2, 4, 8, 16, 32, 64, 128]))
    axes.xaxis.set_minor_locator(NullLocator())
    axes.xaxis.set_major_formatter(FuncFormatter(lambda value, _position: f"{int(value)}"))
    axes.set_xlabel("KARATSUBA_THRESHOLD (limbs)", fontsize=9)
    LogRatioAxis(axes, allRatios, [1, 1.1, 1.25, 1.5, 2, 3, 4, 6, 8])
    axes.set_ylabel("time relative to best threshold", fontsize=9)
    AddLegend(figure, axes)
    SaveFigure(figure, path)


def PlotSawtooth(sawtoothMedians, threshold, path):
    figure, axes = NewFigure("Karatsuba time divided by n^log2(3)",
        f"Median of repeats, scaled so the median is 1. Hairlines at n = {threshold}·2^k: one more limb adds a recursion level.")
    times = BySize(sawtoothMedians, "karatsuba")
    sizes = sorted(times)
    normalised = [times[n] / n ** KARATSUBA_EXPONENT for n in sizes]
    scale = statistics.median(normalised)
    values = [value / scale for value in normalised]

    for k in range(0, 8):
        boundary = threshold << k

        if sizes[0] <= boundary <= sizes[-1]:
            axes.axvline(boundary, color=BASELINE, linewidth=0.8, zorder=0)

    axes.plot(sizes, values, color=SERIES_COLOURS["karatsuba"], linewidth=1.5, solid_joinstyle="round")
    LogSizeAxis(axes, sizes, every=1)
    axes.grid(False, axis="x")
    axes.set_ylabel("normalised time", fontsize=9)
    axes.yaxis.set_major_formatter(FuncFormatter(lambda value, _position: f"{value:.2f}"))
    SaveFigure(figure, path, top=0.89)


def Main():
    codeDir = os.path.dirname(os.path.abspath(__file__))
    resultsDir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(codeDir, "..", "Results")
    rawDir = os.path.join(resultsDir, "Raw")
    figureDir = os.path.join(resultsDir, "Figures")
    os.makedirs(figureDir, exist_ok=True)

    environment = ReadEnvironment(os.path.join(rawDir, "environment.txt"))
    threshold = int(environment["karatsuba_threshold"])
    steps = int(environment["e2e_steps"])

    isolatedGroups = GroupTimes(ReadRows(os.path.join(rawDir, "isolated.csv")), ["algorithm", "n_limbs"])
    endToEndGroups = GroupTimes(ReadRows(os.path.join(rawDir, "e2e.csv")), ["algorithm", "n_limbs"])
    crossoverGroups = GroupTimes(ReadRows(os.path.join(rawDir, "crossover.csv")), ["algorithm", "n_limbs"])
    thresholdGroups = GroupTimes(ReadRows(os.path.join(rawDir, "threshold.csv")), ["threshold", "n_limbs"])
    sawtoothGroups = GroupTimes(ReadRows(os.path.join(rawDir, "sawtooth.csv")), ["algorithm", "n_limbs"])

    isolated = Medians(isolatedGroups)
    endToEnd = Medians(endToEndGroups)
    crossover = Medians(crossoverGroups)
    thresholdMedians = Medians(thresholdGroups)
    sawtooth = Medians(sawtoothGroups)
    timesByAlgorithm = {algorithm: BySize(isolated, algorithm) for algorithm in ALGORITHMS}
    lines = []

    lines.append("# Benchmark tables")
    lines.append("")
    lines.append("Generated by `Code/AnalyseResults.py` from `Results/Raw/`. Do not edit by hand; rerun the script.")
    lines.append("All times are medians over repeats, in microseconds per operation.")
    lines.append("")
    lines.append("## Environment")
    lines.append("")

    for key, value in environment.items():
        lines.append(f"- `{key}`: {value}")

    lines.append("")
    lines.append("## 1. Isolated multiplication time (µs)")
    lines.append("")
    lines.append("| n (limbs) | bits | " + " | ".join(ALGORITHMS) + " |")
    lines.append("|---:|---:|" + "---:|" * len(ALGORITHMS))

    for n in sorted({n for _, n in isolated}):
        cells = [FormatMicros(timesByAlgorithm[a][n]) if n in timesByAlgorithm[a] else "–" for a in ALGORITHMS]
        lines.append(f"| {n} | {n * 64:,} | " + " | ".join(cells) + " |")

    spreads = sorted(((Spread(values), key) for key, values in isolatedGroups.items() if len(values) > 1), reverse=True)
    medianSpread = statistics.median(spread for spread, _ in spreads)
    lines.append("")
    lines.append(f"Run-to-run spread, (max − min) / median over repeats: median {medianSpread:.1%} across all cells; worst five:")
    lines.append("")

    for spread, (algorithm, n) in spreads[:5]:
        lines.append(f"- {algorithm} at n = {n}: {spread:.1%}")

    lines.append("")
    lines.append("## 2. Ratios at selected sizes")
    lines.append("")
    lines.append("| n (limbs) | schoolbook / karatsuba | fft / karatsuba | karatsuba / gmp | schoolbook / fft |")
    lines.append("|---:|---:|---:|---:|---:|")
    karatsuba = timesByAlgorithm["karatsuba"]

    for n in sorted(karatsuba):
        schoolbookRatio = f"{timesByAlgorithm['schoolbook'][n] / karatsuba[n]:.2f}" if n in timesByAlgorithm["schoolbook"] else "–"
        fftRatio = f"{timesByAlgorithm['fft'][n] / karatsuba[n]:.2f}" if n in timesByAlgorithm["fft"] else "–"
        gmpRatio = f"{karatsuba[n] / timesByAlgorithm['gmp'][n]:.2f}" if n in timesByAlgorithm["gmp"] else "–"
        schoolbookFftRatio = (f"{timesByAlgorithm['schoolbook'][n] / timesByAlgorithm['fft'][n]:.2f}"
            if n in timesByAlgorithm["schoolbook"] and n in timesByAlgorithm["fft"] else "–")
        lines.append(f"| {n} | {schoolbookRatio} | {fftRatio} | {gmpRatio} | {schoolbookFftRatio} |")

    lines.append("")
    lines.append("## 3. Crossovers")
    lines.append("")
    lines.append("Interpolated in log space between the two measured sizes either side of each sign change.")
    lines.append("")
    lines.append("| pair | source | bracket (limbs) | interpolated n | faster above the crossing |")
    lines.append("|---|---|---|---:|---|")
    pairs = [
        ("schoolbook", "karatsuba", BySize(crossover, "schoolbook"), BySize(crossover, "karatsuba"), "fine sweep (step 2)"),
        ("schoolbook", "karatsuba", timesByAlgorithm["schoolbook"], karatsuba, "main grid"),
        ("schoolbook", "fft", timesByAlgorithm["schoolbook"], timesByAlgorithm["fft"], "main grid"),
        ("karatsuba", "fft", karatsuba, timesByAlgorithm["fft"], "main grid"),
        ("fft", "gmp", timesByAlgorithm["fft"], timesByAlgorithm["gmp"], "main grid"),
        ("karatsuba", "gmp", karatsuba, timesByAlgorithm["gmp"], "main grid"),
    ]

    for nameA, nameB, timesA, timesB, source in pairs:
        crossings = Crossovers(timesA, timesB)

        if not crossings:
            sizes = sorted(set(timesA) & set(timesB))
            faster = nameA if timesA[sizes[-1]] < timesB[sizes[-1]] else nameB
            lines.append(f"| {nameA} vs {nameB} | {source} | none in {sizes[0]}–{sizes[-1]} | – | {faster} throughout |")

        for low, high, crossing, aFasterAbove in crossings:
            lines.append(f"| {nameA} vs {nameB} | {source} | {low}–{high} | {crossing:.0f} | {nameA if aFasterAbove else nameB} |")

    lines.append("")
    lines.append("## 4. Empirical growth exponents")
    lines.append("")
    lines.append("Slope of log t against log n. Theory: schoolbook 2, Karatsuba log2(3) ≈ 1.585, FFT n log n (slope just above 1).")
    lines.append("")
    powersOfTwo = [2 ** k for k in range(3, 18)]
    lines.append("| algorithm | fit n ≥ 1024 | " + " | ".join(f"{n}→{2 * n}" for n in powersOfTwo[:-1]) + " |")
    lines.append("|---|---:|" + "---:|" * (len(powersOfTwo) - 1))

    for algorithm in ALGORITHMS:
        times = timesByAlgorithm[algorithm]
        fit = FitExponent(times, 1024, 1 << 30)
        local = [f"{math.log2(times[2 * n] / times[n]):.2f}" if n in times and 2 * n in times else "–" for n in powersOfTwo[:-1]]
        lines.append(f"| {algorithm} | {fit:.3f} | " + " | ".join(local) + " |")

    lines.append("")
    lines.append("## 5. Karatsuba threshold sweep")
    lines.append("")
    lines.append("Time relative to the best threshold for that size (1.00 = best); raw medians in µs in the second table.")
    lines.append("")
    thresholds = sorted({t for t, _ in thresholdMedians})
    thresholdSizes = sorted({n for _, n in thresholdMedians})
    best = {n: min(thresholdMedians[(t, n)] for t in thresholds) for n in thresholdSizes}
    lines.append("| threshold | " + " | ".join(f"n = {n}" for n in thresholdSizes) + " | geometric mean |")
    lines.append("|---:|" + "---:|" * (len(thresholdSizes) + 1))

    for t in thresholds:
        relative = [thresholdMedians[(t, n)] / best[n] for n in thresholdSizes]
        geometricMean = math.exp(statistics.mean(math.log(value) for value in relative))
        lines.append(f"| {t} | " + " | ".join(f"{value:.2f}" for value in relative) + f" | {geometricMean:.3f} |")

    lines.append("")
    lines.append("| threshold | " + " | ".join(f"n = {n}" for n in thresholdSizes) + " |")
    lines.append("|---:|" + "---:|" * len(thresholdSizes))

    for t in thresholds:
        lines.append(f"| {t} | " + " | ".join(FormatMicros(thresholdMedians[(t, n)]) for n in thresholdSizes) + " |")

    lines.append("")
    lines.append(f"## 6. Karatsuba either side of each recursion level change (threshold {threshold})")
    lines.append("")
    lines.append(f"At n = {threshold}·2^k the leaves are exactly {threshold} limbs; one more limb adds a level and roughly halves the leaves.")
    lines.append("")
    lines.append("| n | time (µs) | n + 1 | time (µs) | ratio (n + 1) / n | expected from n^1.585 |")
    lines.append("|---:|---:|---:|---:|---:|---:|")
    sawtoothTimes = BySize(sawtooth, "karatsuba")

    for k in range(1, 8):
        n = threshold << k

        if n in sawtoothTimes and n + 1 in sawtoothTimes:
            ratio = sawtoothTimes[n + 1] / sawtoothTimes[n]
            expected = ((n + 1) / n) ** KARATSUBA_EXPONENT
            lines.append(f"| {n} | {FormatMicros(sawtoothTimes[n])} | {n + 1} | {FormatMicros(sawtoothTimes[n + 1])} | {ratio:.3f} | {expected:.3f} |")

    lines.append("")
    lines.append("Mean of time / n^1.585 (ns) over each band between level changes, per repeat. "
        "Consistent repeats mean a band level is real; the band to band drift is the lower order (linear) terms and memory effects.")
    lines.append("")
    sawtoothRows = ReadRows(os.path.join(rawDir, "sawtooth.csv"))
    repeats = sorted({int(row["rep"]) for row in sawtoothRows})
    byRepeat = {rep: {int(row["n_limbs"]): PerOpMicros(row) for row in sawtoothRows if int(row["rep"]) == rep} for rep in repeats}
    lines.append("| band (limbs) | " + " | ".join(f"repeat {rep}" for rep in repeats) + " |")
    lines.append("|---|" + "---:|" * len(repeats))

    for k in range(0, 8):
        low = threshold << k
        high = threshold << (k + 1)
        band = [n for n in sorted(sawtoothTimes) if low < n <= high]

        if len(band) >= 2:
            means = [statistics.mean(byRepeat[rep][n] / n ** KARATSUBA_EXPONENT for n in band) * 1000 for rep in repeats]
            lines.append(f"| {band[0]}–{band[-1]} | " + " | ".join(f"{value:.3f}" for value in means) + " |")

    largeSizes = [n for n in sorted(sawtoothTimes) if n >= 256]

    if len(repeats) >= 2 and len(largeSizes) >= 3:
        curves = {rep: [byRepeat[rep][n] / n ** KARATSUBA_EXPONENT for n in largeSizes] for rep in repeats}
        correlations = [Correlation(curves[first], curves[second]) for index, first in enumerate(repeats) for second in repeats[index + 1:]]
        spreads = [Spread([byRepeat[rep][n] for rep in repeats]) for n in largeSizes]
        lines.append("")
        lines.append(f"Fine structure check for n ≥ 256: correlation of the normalised curve between pairs of repeats = "
            + ", ".join(f"{value:.2f}" for value in correlations)
            + f"; median run-to-run spread {statistics.median(spreads):.1%}. "
            "Correlations near zero mean size to size wiggles at that scale are noise, not structure.")
    lines.append("")
    lines.append(f"## 7. End to end modular squaring chain ({steps} squarings per chain)")
    lines.append("")
    lines.append("Per squaring = chain time / steps. Overhead = per squaring time / isolated multiplication time − 1, i.e. the cost of the fold, increment and copy plus any cache effects.")
    lines.append("")
    lines.append("| algorithm | n (limbs) | chain (µs) | per squaring (µs) | isolated mul (µs) | overhead |")
    lines.append("|---|---:|---:|---:|---:|---:|")

    for algorithm in ALGORITHMS:
        for n in sorted(BySize(endToEnd, algorithm)):
            chain = endToEnd[(algorithm, n)]
            perSquaring = chain / steps
            isolatedTime = timesByAlgorithm[algorithm].get(n)
            overhead = f"{perSquaring / isolatedTime - 1:+.1%}" if isolatedTime else "–"
            isolatedText = FormatMicros(isolatedTime) if isolatedTime else "–"
            lines.append(f"| {algorithm} | {n} | {FormatMicros(chain)} | {FormatMicros(perSquaring)} | {isolatedText} | {overhead} |")

    lines.append("")
    lines.append("## Figures")
    lines.append("")

    figures = [
        ("runtime.png", "Time per multiplication", lambda path: PlotRuntime(isolated, path)),
        ("relative_to_gmp.png", "Slowdown relative to GMP", lambda path: PlotRelativeToGmp(isolated, path)),
        ("threshold_sweep.png", "Karatsuba threshold sweep", lambda path: PlotThresholdSweep(thresholdMedians, path)),
        ("karatsuba_sawtooth.png", "Karatsuba sawtooth", lambda path: PlotSawtooth(sawtooth, threshold, path)),
    ]

    for fileName, caption, plot in figures:
        plot(os.path.join(figureDir, fileName))
        lines.append(f"![{caption}](Figures/{fileName})")
        lines.append("")

    with open(os.path.join(resultsDir, "Tables.md"), "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))

    print(f"Wrote {os.path.join(resultsDir, 'Tables.md')} and {len(figures)} figures")


if __name__ == "__main__":
    Main()
