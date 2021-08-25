import numpy as np
import sys

bidl_tps_raw = []
bidl_mean_tps = 0
lines = []
lines = sys.stdin.readlines()

for line in lines[int(len(lines)*0.4):int(len(lines)*0.7)]:
	bidl_tps_raw.append(int(line.split()[-2]))
bidl_mean_tps = int(np.mean(bidl_tps_raw))
print(bidl_mean_tps)
