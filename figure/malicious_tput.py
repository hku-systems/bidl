import numpy as np
import matplotlib.pyplot as plt
import sys
import datetime

template = 'BIDL transaction commit throughput'
tputs = []

x = []
tick = 0
for line in sys.stdin:
    if template in line:
        tput = line.split()[6]
        tputs.append(tput)
        x.append(tick)
        tick = tick+1
        # tputs.append(temp[2][:-1])
# print(tputs[:10])

# t = datetime.datetime.strptime(timestamps[0],"%H:%M:%S.%f")
# print(t)

# interval=400
# cur = 0
# nxt = 0
# cnt = 0
# cnts = []
# time = 0
# times = []

# for i in range(len(timestamps)):
#     timestamp = datetime.datetime.strptime(timestamps[i],"%H:%M:%S.%f")
#     # print(timestamp)
#     if cur == 0:
#         cur = timestamp
#         nxt = cur + datetime.timedelta(milliseconds=interval)
#     while timestamp >= nxt:
#         cur = nxt
#         nxt += datetime.timedelta(milliseconds=interval) 
#         cnts.append(cnt*1e3/interval)
#         time += interval
#         times.append(time)
#         cnt = 0
#     cnt += 1

# x=[]
# tputs=[]
# ignore = 0
# tick = 0
# xticks = []
# xlabels = []
# zeros = 0
# for i in range(1, len(cnts)-1):
#     if cnts[i] == 0:
#         zeros = zeros + 1
#     else:
#         zeros = 0
#     if zeros < 10:
#         tputs.append(cnts[i])
#         x.append(tick)
#         tick = tick + 1
#         if tick % 5 == 0:
#             xticks.append(tick)
#             xlabels.append(i*interval)

fig, ax = plt.subplots()
plt.plot(x, tputs)
plt.xlabel('Time (ms)', fontsize=15)
plt.ylabel('Throughput (kTxns/s)', fontsize=15)
plt.ylim(0)
plt.xticks(rotation=90)

plt.tight_layout()
plt.savefig("figure/malicious.pdf")
