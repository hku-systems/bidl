import sys

cur = 0
nxt = 0
interval = 500 *1e6 # ms => ns

latency = []
timestamps = []
cnts = []
cnt = 0
template = 'end: '

interval = int(sys.argv[1])

# print(interval)

# if sys.argv[2] == 'phase1':
#     template = 'proposal: '

# obtain timestamps
for line in sys.stdin:
    if template in line:
        temp = line.split()
        timestamps.append(int(temp[1]))
timestamps.sort()

# remove redundant timestamps
tss = [timestamps[0]]
for ts in timestamps:
    if ts != tss[len(tss)-1]:
        tss.append(ts)
# print("All timestamps:", tss)

# compute TPS between each block
i=0
while i < len(tss)-interval:
    ts0 = tss[i]
    ts1 = tss[i+interval]
    time = (ts1 - ts0)/1e9
    cnts.append((1500*interval)/time)
    i += interval

print("End-to-end TPS:", cnts)
print("Overall end-to-end TPS:", 1500*(len(tss)-1)/(tss[len(tss)-1] - tss[0])*1e9)

