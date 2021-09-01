import numpy as np
import matplotlib.pyplot as plt
import sys

x = [x/10 for x in range(0, 6)]
nd_ff = []
with open("logs/ff/nondeterminism/log.log") as f:
    for line in f.readlines(): 
        if "ave" in line:
            temp = line.split()
            nd_ff.append(float(temp[-1]))

        

# BIDL: TODO
# bidl_throughput = {} 
# with open("logs/bidl/nondeterminism/nondeterminism.log") as f: 
#     lines = f.readlines()
#     for line in lines:
#         k=int(line.split()[1])
#         v=int(line.split()[-1])
#         if "throughput" in line:
#             bidl_throughput[k] = v
# 
# xx = [x/100 for x in range(0, 60, 25)]
# bidl_tps_nd=[x*1e3 for x in bidl_throughput.values()]


plt.plot(x, nd_ff, 's-', linewidth=2.0, linestyle="--", markersize=10, label=r'FF non-determinism', markerfacecolor='none')
# plt.plot(xx, bidl_tps_nd, 's-', linewidth=3.0, markersize=10, label=r'Bidl non-determinism')
plt.legend(loc="center right")
plt.xlabel("rate")
plt.ylabel("tps")

plt.savefig("figure/nondeterminism.pdf")