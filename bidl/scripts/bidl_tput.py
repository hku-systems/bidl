import numpy as np
import sys

bidl_tps_raw = []
bidl_mean_tps = 0
lines = []
lines = sys.stdin.readlines()

cap = int(sys.argv[1])
for line in lines:
	if "Inf" not in line:
		tput = int(line.split()[-2])
		if tput < cap:
			bidl_tps_raw.append(tput)

num = len(bidl_tps_raw)

print(bidl_tps_raw)
bidl_mean_tps = int(np.mean(bidl_tps_raw[int(num*0.3):int(num*0.5)]))
print(bidl_mean_tps)
