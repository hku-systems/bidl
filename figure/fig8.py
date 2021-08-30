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
        
plt.plot(x, ct_ff, marker='x', label="contention")
plt.plot(x, nd_ff, marker='o', label="nondeterminism")
plt.legend()
plt.xlabel("contention/nondeterminism rate")
plt.ylabel("throughput")

plt.savefig("figure/fig8.pdf")