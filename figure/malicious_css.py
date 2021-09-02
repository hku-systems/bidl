import numpy as np
import matplotlib.pyplot as plt
import sys
import datetime

template = 'operations/sec'
tputs = []

x = []
tick = 0
# viewchange = []
avgs = []
nums = []
avg = 0
num = 0
lines = []
for line in sys.stdin:
    if template in line:
        lines.append(line)
    
for i in range(len(lines)):
    line = lines[i]
    # print(line)
    tput = float(line.split()[2])
    if tput != "Infinity" and tput < 90.0:
        tputs.append(float(tput))
        x.append(tick)
        tick = tick+1
        avg += tput
        num += 1
    if tput < 1.0 or i == len(lines)-1:
        avg = avg / num
        avgs.append(avg)
        nums.append(num)
        num = 0
        avg = 0

fig, ax = plt.subplots()
# plt.plot(x[:300], tputs[:300])

plt.plot(tputs)
start = 0
end = 0
plt.vlines(0, 0, 100, colors="b", linestyle="--", linewidth=1)
xticks = []
for i in range(1, len(avgs)):
    avg = avgs[i]
    num = nums[i]
    start = end
    end = end + num 
    plt.hlines(avg, start, end, colors="r", linestyle="--")
    plt.vlines(end, 0, 100, colors="b", linestyle="--", linewidth=1)
    xticks.append((start+end)/2)



# plt.xlabel('Views', fontsize=15)
plt.ylabel('Throughput (kTxns/s)', fontsize=15)
plt.ylim(0)
ax.set_xticks(xticks)
ax.set_xticklabels(["view 0", "view 1", "view 2", "view 3", "view 4"])

plt.text(xticks[0], 100,
	"Misbehave",
	fontsize=8,
    weight='bold',
	verticalalignment="top",
	horizontalalignment="center"
)

plt.text(xticks[2], 100,
	"Misbehave",
	fontsize=8,
    weight='bold',
	verticalalignment="top",
	horizontalalignment="center"
)

plt.tight_layout()
plt.savefig("figure/malicious.pdf")
