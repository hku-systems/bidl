# transaction receiving tps
import sys
import datetime

cur = 0
nxt = 0

latency = []
timestamps = []
cnts = []
cnt = 0
# template = 'cut txs into block'
# template = 'Enqueuing message into batch'
# template = 'Transaction index'
template = 'txType=ENDORSER_TRANSACTION'

interval = int(sys.argv[1])

for line in sys.stdin:
    if template in line:
        temp = line.split()
        timestamps.append(temp[1])

# date1=datetime.datetime.strptime(timestamps[0],"%H:%M:%S.%f")
# date2=datetime.datetime.strptime(timestamps[1],"%H:%M:%S.%f")
# print(date1)
# print(date2)
# print((date2-date1).total_seconds())
# date3=date2 + datetime.timedelta(milliseconds=50000)
# print(date2)
# print(date3)
# print(date2 > date3)

# timestamps.sort()

for i in range(len(timestamps)):
    timestamp = datetime.datetime.strptime(timestamps[i],"%H:%M:%S.%f")
    if cur == 0:
        cur = timestamp
        nxt = cur + datetime.timedelta(milliseconds=interval)
    while timestamp >= nxt:
        cur = nxt
        nxt += datetime.timedelta(milliseconds=interval)
        cnts.append(cnt*1e3/interval)
        cnt = 0
    cnt += 1


print(len(cnts))
print(cnts)

