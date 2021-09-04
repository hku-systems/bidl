import numpy as np
import matplotlib.pyplot as plt
import sys
def remove_outliers(x, outlierConstant = 1.5):
    a = np.array(x)
    upper_quartile = np.percentile(a, 75)
    lower_quartile = np.percentile(a, 25)
    IQR = (upper_quartile - lower_quartile) * outlierConstant
    quartileSet = (lower_quartile - IQR, upper_quartile + IQR)
    resultList = []
    removedList = []
    for y in a.tolist():
        if y >= quartileSet[0] and y <= quartileSet[1]:
            resultList.append(y)
        else:
            removedList.append(y)
    return (resultList, removedList)


ff_tps = []
endorse_latency = []
commit_latency = []
orderer_latency = []
with open("logs/ff/performance/log.log") as f: 
    for line in f.readlines():
        if line.startswith("ave"):
            temp = line.split()
            ff_tps.append(float(temp[-1]))
        if "endorse " in line:
            endorse_latency.append(float(line.split()[-1]))
        if "commit " in line:
            commit_latency.append(float(line.split()[-1]))
        if "orderer ave" in line: 
            orderer_latency.append(float(line.split()[-1]))

ff_latency = []
idj = 0
for i in range(0, len(endorse_latency), 5):
    temp = []
    for j in range(5): 
        idx = i + j
        temp.append(endorse_latency[idx] + commit_latency[idx])
    res = remove_outliers(temp)
    print("remove ff latency", res[1])
    print(orderer_latency[idj])
    ff_latency.append(np.mean(temp[0]) + orderer_latency[idj])
    idj += 1
# ff_latency[-1] = p3_latency[-1]

fabric_tps = []
endorse_latency = []
commit_latency = []
orderer_latency = []
with open("logs/fabric/log.log") as f: 
    for line in f.readlines():
        if line.startswith("ave"):
            temp = line.split()
            fabric_tps.append(float(temp[-1]))
        if "endorse " in line:
            endorse_latency.append(float(line.split()[-1]))
        if "commit " in line:
            commit_latency.append(float(line.split()[-1]))
        if "orderer ave" in line: 
            orderer_latency.append(float(line.split()[-1]))

fabric_latency = []
idj = 0
for i in range(0, len(endorse_latency), 5):
    temp = []
    for j in range(5): 
        idx = i + j
        temp.append(endorse_latency[idx] + commit_latency[idx])
    res = remove_outliers(temp)
    print("remove fabric latency", res[1])
    fabric_latency.append(np.mean(res[0]) + orderer_latency[idj])
    idj += 1

streamchain_latency = []
streamchain_tps = []
with open("logs/streamchain/validation.log") as f:
    for line in f.readlines():
        temp = line.split(',')
        streamchain_latency.append(float(temp[-2]))
        streamchain_tps.append(float(temp[-1]))

with open("logs/streamchain/endorsement.log") as f:
    cnt=0
    for line in f.readlines():
        temp = line.split(',')
        streamchain_latency[cnt] += float(temp[-2])
        cnt += 1

with open("logs/streamchain/ordering.log") as f:
    cnt=0
    for line in f.readlines():
        temp = line.split(',')
        streamchain_latency[cnt] += float(temp[-2])
        cnt += 1

# BIDL
bidl_throughput = {} 
bidl_consensus_latency = {}
bidl_execution_latency = {} 
bidl_commit_latency = {}
with open("logs/bidl/performance/performance.log") as f: 
    lines = f.readlines()
    for line in lines:
        k=int(line.split()[1])
        v=int(line.split()[-2])
        if "throughput" in line:
            bidl_throughput[k] = v
        if "consensus latency" in line:
            bidl_consensus_latency[k] = v
        if "execution latency" in line:
            bidl_execution_latency[k] = v
        if "commit latency" in line:
            bidl_commit_latency[k] = v

bidl_tps=[x for x in bidl_throughput.values()]
bidl_latency=[]
for key in bidl_commit_latency.keys():
    consensus_latency = bidl_consensus_latency[key]
    execution_latency = bidl_execution_latency[key]
    commit_latency = bidl_commit_latency[key]
    if consensus_latency > execution_latency:
        bidl_latency.append(consensus_latency + commit_latency)
    else:
        bidl_latency.append(execution_latency + commit_latency)

plt.plot([t/1e3 for t in ff_tps], ff_latency, linestyle="--", marker='x', label="FastFabric")
plt.plot([t/1e3 for t in fabric_tps], fabric_latency, linestyle="--", marker='o', label="Hyperledger Fabric")
plt.plot([t/1e3 for t in streamchain_tps], streamchain_latency, linestyle="--", marker='s', label="StreamChain")
plt.plot(bidl_tps, bidl_latency, marker='P', label="BIDL")

plt.xlabel("Throughput (kTxns/s)")
plt.ylabel("Latency (ms)")
plt.legend()
plt.savefig("figure/performance.pdf")
