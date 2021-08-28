#import numpy as np
import sys

cur = 0
nxt = 0
interval = 1000 # 1000ms

latency = []
timestamps = []
cnts = []
cnt = 0

for line in sys.stdin:
    if "bEnchmark" in line:
        temp = line.split()
        timestamps.append(int(temp[1]))
        latency.append(float(temp[2]) / 1000)

timestamps.sort()

for timestamp in timestamps:
    if cur == 0:
        cur = timestamp
        nxt = cur + interval
    while timestamp >= nxt:
        cur = nxt
        nxt += interval
        cnts.append(cnt)
        cnt = 0
    cnt += 1

print(cnts)
print("latency = {:.3f}ms".format(sum(latency) / len(latency)))

