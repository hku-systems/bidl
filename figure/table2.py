import numpy as np 

latency = {}

for phase in ["endorse", "commit"]:
    latency[phase] = []
    with open("logs/ff/breakdown/" + phase + "/log.log") as f:
        for line in f.readlines():
            if phase in line:
                latency[phase].append(float(line.split()[-1]))

endorse_latency = []
commit_latency = []
for i in range(0, len(latency["endorse"]), 5):
    endorse_latency.append(np.mean(latency["endorse"][i:i+5]))
for i in range(0, len(latency["commit"]), 5):
    commit_latency.append(np.mean(latency["commit"][i:i+5]))
print("endorse latency", endorse_latency)
print("commit latency", commit_latency)
