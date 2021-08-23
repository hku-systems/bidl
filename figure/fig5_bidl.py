import numpy as np
import matplotlib.pyplot as plt
import sys

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


streamchain_latency = []
streamchain_tps = []
with open("logs/streamchain/validation.log") as f:
    for line in f.readlines():
        temp = line.split(',')
        streamchain_latency.append(float(temp[-2]))
        streamchain_tps.append(float(temp[-1]))

streamchain_latency = []
streamchain_tps = []
with open("logs/streamchain/validation.log") as f:
    for line in f.readlines():
        temp = line.split(',')
        streamchain_latency.append(float(temp[-2]))
        streamchain_tps.append(float(temp[-1]))

with open("logs/streamchain/endorsement.log") as f:
    cnt=0
    for line in f.readlines():
        temp = line.split(',')
        streamchain_latency[cnt] += float(temp[-2])
        cnt += 1

with open("logs/streamchain/ordering.log") as f:
    cnt=0
    for line in f.readlines():
        temp = line.split(',')
        streamchain_latency[cnt] += float(temp[-2])
        cnt += 1

# BIDL 
bidl_tps_raw = []
bidl_peak_tps = 0
with open("logs/bidl/bidl_performance_tput_backup.log") as f: 
	lines = f.readlines()
	for line in lines[-10:-1]:
		bidl_tps_raw.append(int(line.split()[-2]))
	bidl_peak_tps = np.mean(bidl_tps_raw)

bidl_latency_raw = []
with open("logs/bidl/bidl_performance_latency_backup.log") as f: 
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


plt.plot(ff_tps, ff_latency, marker="x", label="fastfabric")
plt.plot(streamchain_tps, streamchain_latency, marker="o", label="streamchain")
plt.plot(bidl_tps, bidl_latency, marker="s", label="bidl")
plt.legend()
plt.savefig("figure/fig5_bidl.pdf")
