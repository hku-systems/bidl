import numpy as np
import matplotlib.pyplot as plt
import sys

# FastFabric
x_ff = [x*10 for x in range(0, 6)]
nd_ff = []
with open("logs/ff/nondeterminism/log.log") as f:
    for line in f.readlines(): 
        if "ave" in line:
            temp = line.split()
            nd_ff.append(float(temp[-1]))
nd_ff = [tps/1e3 for tps in nd_ff]

# BIDL
nd_bidl = {} 
with open("logs/bidl/nondeterminism/nondeterminism.log") as f: 
    lines = f.readlines()
    for line in lines:
        if "throughput" in line:
            k=int(line.split()[1])
            v=int(line.split()[-2])
            nd_bidl[k] = v
            
plt.plot(x_ff, nd_ff, 's-', linewidth=2.0, linestyle="--", markersize=10, label=r'FastFabric non-determinism', markerfacecolor='none')
plt.plot(nd_bidl.keys(), nd_bidl.values(), 's-', linewidth=3.0, markersize=10, label=r'Bidl non-determinism')
plt.legend(loc="upper right")
plt.xlabel("Non-deterministic Ratio (%)")
plt.ylabel("Throughput (kTxns/s)")
plt.ylim(0)

plt.savefig("figure/nondeterminism.pdf")