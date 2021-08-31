import numpy as np
import sys

lines = sys.stdin.readlines()

bidl_latency_raw = []
bidl_mean_latency = 0
# for line in lines[int(len(lines)*0.3):int(len(lines)*0.6)]:
for line in lines:
	bidl_latency_raw.append(float(line.split()[3])/1e3)
bidl_mean_latency = int(np.min(bidl_latency_raw))

print(bidl_mean_latency, "ms")