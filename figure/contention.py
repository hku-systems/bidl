import numpy as np
import matplotlib.pyplot as plt
import sys

x = [x/10 for x in range(0, 6)]
ct_ff = []
with open("logs/ff/contention/log.log") as f:
    for line in f.readlines(): 
        if "ave" in line:
            temp = line.split()
            ct_ff.append(float(temp[-1]))
        
plt.plot(x, ct_ff, 'v-', linewidth=2.0, linestyle="--", markersize=10, label=r'FF contention', markerfacecolor='none')

# BIDL: TODO 

plt.legend(loc="center right")
plt.xlabel("rate")
plt.ylabel("tps")

plt.savefig("figure/contention.pdf")