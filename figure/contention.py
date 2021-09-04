import numpy as np
import matplotlib.pyplot as plt
import sys

# FastFabric
x = [x*10 for x in range(0, 6)]
ct_ff = []
with open("logs/ff/contention/log.log") as f:
    for line in f.readlines(): 
        if "ave" in line:
            temp = line.split()
            ct_ff.append(float(temp[-1]))
ct_ff = [tps/1e3 for tps in ct_ff]

# BIDL
ct_bidl = {} 
with open("logs/bidl/contention/contention.log") as f: 
    lines = f.readlines()
    for line in lines:
        if "throughput" in line:
            k=int(line.split()[1])
            v=int(line.split()[-2])
            ct_bidl[k] = v

plt.plot(ct_bidl.keys(), ct_bidl.values(), 's-', linewidth=3.0, markersize=10, label=r'Bidl contention')
plt.plot(x, ct_ff, 'v-', linewidth=2.0, linestyle="--", markersize=10, label=r'FastFabric contention', markerfacecolor='none')
plt.legend(loc="center right")
plt.xlabel("Contention Ratio (%)")
plt.ylabel("Throughput (kTxns/s)")
plt.ylim(0)

plt.savefig("figure/contention.pdf")