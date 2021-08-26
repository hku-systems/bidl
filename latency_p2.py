import numpy as np
import sys

sent_mp={}
end_mp={}

for line in sys.stdin: 
    if "sent:" in line:
        temp = line.split()
        if temp[2] not in sent_mp:
            sent_mp[temp[2]] = int(temp[1])
    if "end:" in line:
        temp = line.split()
        if temp[2] not in end_mp:
            end_mp[temp[2]] = int(temp[1])

print(len(sent_mp), len(end_mp))
p3 = []
for k in end_mp:
    p3.append((end_mp[k] - sent_mp[k])/1e6)

p3 = sorted(p3)
lat = len(p3)

print("p3(orderer+commit) ave:", np.mean(p3)) 
print("50% tail:", p3[int(0.5*lat)]) 
print("90% tail:", p3[int(0.9*lat)]) 
print("95% tail:", p3[int(0.95*lat)]) 
print("99% tail:", p3[int(0.99*lat)]) 