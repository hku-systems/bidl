import numpy as np
import matplotlib.pyplot as plt
import sys

bidl_tps_raw = []
bidl_peak_tps = 0
with open("logs/bidl/bidl_performance_tput.log") as f: 
	lines = f.readlines()
	for line in lines[-10:-1]:
		bidl_tps_raw.append(int(line.split()[-2]))
	bidl_peak_tps = np.mean(bidl_tps_raw)

bidl_latency_raw = []
with open("logs/bidl/bidl_performance_latency.log") as f: 
	lines = f.readlines()
	for line in lines[:-1]:
		bidl_latency_raw.append(float(line.split()[3])/1e3)
	bidl_peak_latency = np.mean(bidl_latency_raw)
	
bidl_tps = []
bidl_latency = []
size = len(bidl_latency_raw)
for i in range(2, size):
	bidl_tps.append(bidl_peak_tps*1e3/size*i)
	bidl_latency.append(bidl_latency_raw[i])

ff_tps = []
endorse_latency = []
commit_latency = []
with open("logs/ff/performance/log.log") as f: 
    for line in f.readlines():
        if line.startswith("ave"):
            temp = line.split()
            ff_tps.append(float(temp[-1]))
        if "endorse" in line:
            endorse_latency.append(float(line.split()[-1]))
        if "commit" in line:
            commit_latency.append(float(line.split()[-1]))

ff_latency = []
for i in range(0, 40, 5):
    temp = []
    for j in range(5): 
        idx = i + j
        temp.append(endorse_latency[idx] + commit_latency[idx])
    ff_latency.append(np.mean(temp))

fabric_tps = []
endorse_latency = []
commit_latency = []
with open("logs/fabric/log.log") as f: 
    for line in f.readlines():
        if line.startswith("ave"):
            temp = line.split()
            fabric_tps.append(float(temp[-1]))
        if "endorse" in line:
            endorse_latency.append(float(line.split()[-1]))
        if "commit" in line:
            commit_latency.append(float(line.split()[-1]))

fabric_latency = []
for i in range(0, len(endorse_latency), 5):
    temp = []
    for j in range(5): 
        idx = i + j
        temp.append(endorse_latency[idx] + commit_latency[idx])
    fabric_latency.append(np.mean(temp))
    print(temp)


plt.plot(ff_tps, ff_latency, marker="x", label="fastfabric")
plt.plot(fabric_tps, fabric_latency, marker="o", label="fabric")
plt.plot(bidl_tps, bidl_latency, marker="s", label="bidl")
# plt.plot(bidl_latency, bidl_tps, label="bidl")
plt.legend()
plt.ylim(0, 200)
plt.savefig("figure/fig3_bidl.pdf")
