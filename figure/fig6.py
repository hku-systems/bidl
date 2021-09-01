import numpy as np
import matplotlib.pyplot as plt
import sys

org = [4, 13, 25, 47]
smart_latency = [45, 25, 18, 20]
SBFT_latency = [40, 20, 15, 20]
hotstuff_latency = [40, 25, 20, 23]
zyzz_latency = [45, 25, 15, 15]


plt.plot(org, smart_latency, marker="s", label="bidl-Smart")
plt.plot(org, SBFT_latency, marker="D", label="bidl-SBFT")
plt.plot(org, hotstuff_latency, marker="x", label="bidl-Hotstuff")
# plt.plot(org, zyzz_latency, marker="v", label="bidl-Zyzzyva")
# plt.plot(bidl_latency, bidl_tps, label="bidl")
plt.legend()
plt.ylim(0, 60)
plt.savefig("figure/fig6.pdf")
