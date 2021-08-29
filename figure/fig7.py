import numpy as np
import matplotlib.pyplot as plt
import sys

org = [4, 13, 25, 47]
bidl_latency = [65, 25, 15, 17]

plt.plot(org, bidl_latency, marker="s", label="bidl-Smart")
# plt.plot(bidl_latency, bidl_tps, label="bidl")
plt.legend()
plt.ylim(0, 80)
plt.savefig("figure/fig7.pdf")
