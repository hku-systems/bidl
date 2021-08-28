import numpy as np
import matplotlib.pyplot as plt
import sys

x = [x/10 for x in range(1, 6)]
nd_ff = []
ct_ff = []
with open("logs/ff/nondeterminism/log.log") as f:
    for line in f.readlines(): 
        if "ave" in line:
            temp = line.split()
            nd_ff.append(float(temp[-1]))

with open("logs/ff/contention/log.log") as f:
    for line in f.readlines(): 
        if "ave" in line:
            temp = line.split()
            ct_ff.append(float(temp[-1]))
        

# BIDL 
bidl_throughput = {} 
with open("logs/bidl/nondeterminism/nondeterminism.log") as f: 
    lines = f.readlines()
    for line in lines:
        k=int(line.split()[1])
        v=int(line.split()[-1])
        if "throughput" in line:
            bidl_throughput[k] = v

xx = [x/100 for x in range(0, 60, 25)]
bidl_tps_nd=[x*1e3 for x in bidl_throughput.values()]

plt.plot(x, ct_ff, 'v-', linewidth=2.0, linestyle="--", markersize=10, label=r'FF contention', markerfacecolor='none')
# plt.plot(x, bidl_tps_contention, 'v-', linewidth=3.0, markersize=10, label=r'Bidl contention')
plt.plot(x, nd_ff, 's-', linewidth=2.0, linestyle="--", markersize=10, label=r'FF non-determinism', markerfacecolor='none')
plt.plot(xx, bidl_tps_nd, 's-', linewidth=3.0, markersize=10, label=r'Bidl non-determinism')

# plt.plot(x, bidl_tps_contention, label="BIDL contention")
# plt.plot(x, bidl_tps_nd, label="BIDL nondeterminism")
# plt.plot(x, ct_ff, label="FF contention")
# plt.plot(x, nd_ff, label="FF nondeterminism")
plt.legend()
plt.xlabel("rate")
plt.ylabel("tps")

plt.savefig("figure/fig8_bidl.pdf")