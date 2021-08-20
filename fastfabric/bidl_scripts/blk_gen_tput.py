# block generation tps at orderer
import sys
import datetime

cur = 0
nxt = 0

latency = []
timestamps = []
cnts = []
cnt = 0
# template = 'Wrote block'
# template = 'START Block Validation'
template = 'Adding payload to local buffer'

interval = int(sys.argv[1]) # number of blocks interval

for line in sys.stdin:
    if template in line:
        temp = line.split()
        timestamps.append(temp[1])

i=0
while i < len(timestamps)-interval:
    ts0 = datetime.datetime.strptime(timestamps[i],"%H:%M:%S.%f")
    ts1 = datetime.datetime.strptime(timestamps[i + interval],"%H:%M:%S.%f")
    time = (ts1 - ts0).total_seconds()
    cnts.append((1500*interval)/time)
    i += interval

print(len(cnts))
print(cnts)

