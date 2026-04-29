import sys

def fix_timestamps(in_path, out_path):
    with open(in_path) as f_in, open(out_path, 'w') as f_out:
        for line in f_in:
            parts = line.split()
            if not parts:
                f_out.write(line)
                continue
            parts[0] = f"{float(parts[0]) / 1e9:.9f}"
            f_out.write(' '.join(parts) + '\n')

if len(sys.argv) < 3:
    print("Usage: fix_timestamps.py <input> <output>")
    sys.exit(1)

fix_timestamps(sys.argv[1], sys.argv[2])
