import numpy as np
import sys

bidl_tps_raw = []
bidl_mean_tps = 0
lines = []
lines = sys.stdin.readlines()

for line in lines:
	if "Inf" not in line:
		tput = int(line.split()[6])
		bidl_tps_raw.append(tput)

num = len(bidl_tps_raw)

# bidl_mean_tps = int(np.mean(bidl_tps_raw[int(num*0.3):int(num*0.6)]))
bidl_mean_tps = int(np.mean(bidl_tps_raw))
# print(bidl_tps_raw)
print(bidl_mean_tps, "kTxns/s")
