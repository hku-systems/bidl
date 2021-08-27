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
		# else:
			# bidl_tps_raw.append(cap)


num = len(bidl_tps_raw)
bidl_mean_tps = int(np.mean(bidl_tps_raw[int(num*0.6):int(num*0.9)]))
print(bidl_mean_tps)
