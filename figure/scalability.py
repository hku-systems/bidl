import numpy as np
import matplotlib.pyplot as plt

# hotstuff TODO
# SBFT TODO
# Zyzz TODO

smart_consensus_latency = {}
smart_execution_latency = {} 
smart_commit_latency = {}
with open("logs/bidl/scalability/scalability.log") as f: 
    lines = f.readlines()
    for line in lines:
        k=int(line.split()[1])
        v=int(line.split()[-2])
        if "consensus latency" in line:
            smart_consensus_latency[k] = v
        if "execution latency" in line:
            smart_execution_latency[k] = v
        if "commit latency" in line:
            smart_commit_latency[k] = v

smart_latency={}
for key in smart_commit_latency.keys():
    consensus_latency = smart_consensus_latency[key]
    execution_latency = smart_execution_latency[key]
    commit_latency = smart_commit_latency[key]
    if consensus_latency > execution_latency:
        smart_latency[key] = consensus_latency + commit_latency
    else:
        smart_latency[key] = execution_latency + commit_latency

# plt.plot(x_ff, nd_ff, 's-', linewidth=2.0, linestyle="--", markersize=10, label=r'FF non-determinism', markerfacecolor='none')
plt.plot(smart_latency.keys(), smart_latency.values(), 's-', linewidth=3.0, markersize=10, label=r'Bidl-Smart')
plt.legend(loc="upper right")
plt.xlabel("Number of organizations")
plt.ylabel("Latency (ms)")
plt.ylim(0)

plt.savefig("figure/scalability.pdf")