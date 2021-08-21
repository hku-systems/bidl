#import numpy as np
import sys

cur = 0
nxt = 0
interval = 500000000 # 500ms

latency = []
timestamps = []
blocks = []
cnts = []
cnt = 0

interval = int(sys.argv[1])*1e6 # ms
pattern = sys.argv[2]

for line in sys.stdin:
    if "end: " in line and pattern in line:
        temp = line.split()
        timestamps.append(int(temp[1]))
        # blocks.append(int(temp[2]))

timestamps.sort()

print("ave: ", len(timestamps)/(timestamps[-1] - timestamps[0])*1e9)

for i in range(len(timestamps)):
    if cur == 0:
        cur = timestamps[i]
        nxt = cur + interval
    while timestamps[i] >= nxt:
        cur = nxt
        nxt += interval
        cnts.append(cnt*1e9/interval)
        cnt = 0
    cnt += 1 #blocks[i]

print(cnts)
# print("latency = {:.3f}ms".format(sum(latency) / len(latency)))

