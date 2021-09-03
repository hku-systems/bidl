import numpy as np
import sys

def percent_range(dataset, min = 0.10, max = 0.90):
    range_max = np.percentile(dataset, max * 100)
    range_min = np.percentile(dataset, min * 100)
    # remove outliers
    new_data = []
    for value in dataset:
        if value < range_max and value > range_min:
            new_data.append(value)
    return new_data

lines = sys.stdin.readlines()
bidl_latency_raw = []
bidl_mean_latency = 0
for line in lines[int(len(lines)*0.3):int(len(lines)*0.6)]:
	bidl_latency_raw.append(float(line.split()[4]))

bidl_latency_new = percent_range(bidl_latency_raw)
print(bidl_latency_new)
bidl_mean_latency = int(np.mean(bidl_latency_new))
print(bidl_mean_latency, "ms")

