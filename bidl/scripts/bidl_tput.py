import numpy as np
import sys

bidl_tps_raw = []
bidl_mean_tps = 0
lines = []
lines = sys.stdin.readlines()

for line in lines:
	if "Inf" not in line:
		bidl_tps_raw.append(int(line.split()[-2]))

num = len(bidl_tps_raw)
# print(bidl_tps_raw[int(num*0.5):int(num*0.8)])
bidl_mean_tps = int(np.mean(bidl_tps_raw[int(num*0.5):int(num*0.8)]))
print(bidl_mean_tps)
