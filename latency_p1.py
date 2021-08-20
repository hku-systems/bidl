import numpy as np
import sys

start_mp={}
proposal_mp={}

for line in sys.stdin: 
    if "start" in line:
        temp = line.split()
        if temp[2] not in start_mp:
            start_mp[temp[2]] = int(temp[1])
    if "proposal" in line:
        temp = line.split()
        if temp[2] not in proposal_mp:
            proposal_mp[temp[2]] = int(temp[1])

p1 = []
for k in proposal_mp:
    p1.append((proposal_mp[k] - start_mp[k])/1e6)
p1 = sorted(p1)
lat = len(p1)

print("p1(endorserment) ave:", np.mean(p1)) 
print("50% tail:", p1[int(0.5*lat)]) 
print("90% tail:", p1[int(0.9*lat)]) 
print("95% tail:", p1[int(0.95*lat)]) 
print("99% tail:", p1[int(0.99*lat)]) 
