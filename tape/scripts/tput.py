#import numpy as np
import sys

cur = 0
nxt = 0
interval = 500 *1e6 # ms => ns

latency = []
timestamps = []
cnts = []
cnt = 0
template = 'end: '

interval = int(sys.argv[1])*1e6 # ms => ns

if sys.argv[2] == 'phase1':
    template = 'proposal: '

for line in sys.stdin:
    if template in line:
        temp = line.split()
        timestamps.append(int(temp[1]))

timestamps.sort()

for i in range(len(timestamps)):
    if cur == 0:
        cur = timestamps[i]
        nxt = cur + interval
    while timestamps[i] >= nxt:
        cur = nxt
        nxt += interval
        cnts.append(cnt*1e9/interval)
        cnt = 0
    cnt += 1

print(cnts)

