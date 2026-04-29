import csv
import os
import statistics
import sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

BIN_LABELS = [f"{1.0 - i*0.05:.2f}-{1.0 - (i+1)*0.05:.2f}" for i in range(20)]

def analyze(csv_path, out_path):
    with open(csv_path) as f:
        reader = csv.reader(f)
        row1 = [int(x) for x in next(reader)]
        dist_rows = [[int(x) for x in row] for row in reader]

    attempted = [v for v in row1 if v != -1]

    hist_dir = os.path.join(os.path.dirname(out_path) or '.', 'histograms')
    os.makedirs(hist_dir, exist_ok=True)

    total_removed = 0
    total_merged = 0
    pad = len(str(len(dist_rows)))

    with open(out_path, 'w') as f:
        f.write("=== RANSAC Stats ===\n")
        f.write(f"Attempts: {len(attempted)}\n")
        f.write(f"Mean:     {statistics.mean(attempted):.4f}\n")
        f.write(f"Median:   {statistics.median(attempted)}\n")
        f.write(f"Mode:     {statistics.mode(attempted)}\n")
        f.write("\n=== Per-Update Stats ===\n")

        for i, row in enumerate(dist_rows):
            bins    = row[:20]
            removed = row[20]
            merged  = row[21]
            total_removed += removed
            total_merged  += merged

            f.write(f"Update {i+1:>{pad}}: removed={removed:>6}, merged={merged:>6}\n")

            fig, ax = plt.subplots(figsize=(12, 5))
            ax.bar(range(20), bins, color='steelblue')
            ax.set_xticks(range(20))
            ax.set_xticklabels(BIN_LABELS, rotation=45, ha='right')
            ax.set_yscale('log')
            ax.set_xlabel('P(E) Range')
            ax.set_ylabel('Map Point Count (log scale)')
            ax.set_title(f'P(E) Distribution — Update {i+1}  '
                         f'(removed={removed}, merged={merged})')
            plt.tight_layout()
            plt.savefig(os.path.join(hist_dir, f'update_{i+1:0{pad}d}.png'), dpi=100)
            plt.close(fig)

        f.write(f"\n=== Totals ===\n")
        f.write(f"Total removed: {total_removed}\n")
        f.write(f"Total merged:  {total_merged}\n")

csv_path = sys.argv[1] if len(sys.argv) > 1 else "VBEEStats.csv"
out_path  = sys.argv[2] if len(sys.argv) > 2 else "VBEEStats_summary.txt"
analyze(csv_path, out_path)
