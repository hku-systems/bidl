import numpy as np
import matplotlib.pyplot as plt
import sys

# bidl_throughput = {} 
# bidl_consensus_latency = {}
# bidl_execution_latency = {} 
# bidl_commit_latency = {}
# with open("logs/bidl/performance/performance.log") as f: 
#     lines = f.readlines()
#     for line in lines:
#         k=int(line.split()[1])
#         v=int(line.split()[-1])
#         if "throughput" in line:
#             bidl_throughput[k] = v
#         if "consensus latency" in line:
#             bidl_consensus_latency[k] = v
#         if "execution latency" in line:
#             bidl_execution_latency[k] = v
#         if "commit latency" in line:
#             bidl_commit_latency[k] = v

# bidl_tps=[x*1e3 for x in bidl_throughput.values()]
# bidl_latency=[]
# for key in bidl_commit_latency.keys():
#     consensus_latency = bidl_consensus_latency[key]
#     execution_latency = bidl_execution_latency[key]
#     commit_latency = bidl_commit_latency[key]
#     if consensus_latency > execution_latency:
#         bidl_latency.append(consensus_latency + commit_latency)
#     else:
#         bidl_latency.append(execution_latency + commit_latency)



ff_tps = []
endorse_latency = []
commit_latency = []
with open("logs/ff/performance/log.log") as f: 
    for line in f.readlines():
        if line.startswith("ave"):
            temp = line.split()
            ff_tps.append(float(temp[-1]))
        if "endorse " in line:
            endorse_latency.append(float(line.split()[-1]))
        if "commit " in line:
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
        if "endorse " in line:
            endorse_latency.append(float(line.split()[-1]))
        if "commit " in line:
            commit_latency.append(float(line.split()[-1]))

fabric_latency = []
for i in range(0, len(endorse_latency), 5):
    temp = []
    for j in range(5): 
        idx = i + j
        temp.append(endorse_latency[idx] + commit_latency[idx])
    fabric_latency.append(np.mean(temp))
    print(temp)

bidl_tps=[20000, 40000, 50000]
bidl_latency=[25, 19, 23]

plt.plot(ff_tps, ff_latency, marker="x", label="fastfabric")
plt.plot(fabric_tps, fabric_latency, marker="o", label="fabric")
plt.plot(bidl_tps, bidl_latency, marker="s", label="bidl")
# plt.plot(bidl_latency, bidl_tps, label="bidl")
plt.legend()
# plt.ylim(0, 200)
plt.savefig("figure/fig3_bidl.pdf")
