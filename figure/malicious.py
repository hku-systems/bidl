import numpy as np
import matplotlib.pyplot as plt
import sys
import datetime

template = 'operations/sec'

lines = []
with open("./bidl/logs/consensus_0.log") as f:
    for line in f.readlines():
        if template in line:
            lines.append(line)

num = 0
tputs = []
nums = []
avgs = []
temps = []
for i in range(len(lines)):
    line = lines[i]
    tput = 0
    if "Throughput = Infinity operations/sec" in line:
        tput = 200.0
    else:
        tput = float(line.split()[2])
    if tput > 200.0:
        tput = 200.0
        
    tputs.append(tput/2) # batch size = 500
    temps.append(tput/2)
    num += 1
    if num % 50 == 0:
        avg = np.mean(temps[5:-5])
        avgs.append(avg)
        nums.append(num)
        num = 0
        temps = []
print(avgs)

for i in range(1, len(tputs)-1):
    tputs[i] = (tputs[i-1] + tputs[i] + tputs[i+1])/3

fig, ax = plt.subplots()
plt.plot(tputs, label="BIDL tput")
plt.axvline(x=0, linestyle="--", linewidth=1)
start = 0
end = 0
xticks = []
for i in range(0, len(avgs)):
    avg = avgs[i]
    num = nums[i]
    start = end
    end = end + num
    if i==1:
        plt.hlines(avg, start, end, colors="r", linestyle="--", label="Avg tput") # to prevent generating multiple lines in the legend
    else:
        plt.hlines(avg, start, end, colors="r", linestyle="--")
    plt.axvline(x=end, linestyle="--", linewidth=1)
    xticks.append((start+end)/2)

plt.ylabel('Throughput (kTxns/s)', fontsize=15)
plt.ylim(0, 120)
ax.set_xticks(xticks)
ax.set_xticklabels(["view 0", "view 1", "view 2", "view 3", "view 4"])

for i in [0, 3, 4]:
    t = plt.text(xticks[i], 100,
        "Misbehave",
        fontsize=8,
        weight='bold',
        verticalalignment="top",
        horizontalalignment="center")
    t.set_bbox(dict(facecolor='white', alpha=1))

t2 = plt.text(xticks[2], 100,
    "Misbehave\nadd to\ndenylist",
    fontsize=8,
    weight='bold',
    verticalalignment="top",
    horizontalalignment="center")
t2.set_bbox(dict(facecolor='white', alpha=1))


plt.legend(loc="lower right")
plt.tight_layout()
plt.savefig("figure/malicious.pdf")
