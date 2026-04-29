import sys
import json
import zipfile

with zipfile.ZipFile(sys.argv[2], 'r') as zip_ref:
    with zip_ref.open('stats.json') as stats_file:
        data = json.load(stats_file)
        keys = ['max', 'mean', 'median', 'min', 'rmse', 'sse', 'std']
        print(sys.argv[1] + ','.join(format(float(data[key]), 'f') for key in keys))
        