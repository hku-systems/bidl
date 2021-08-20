import numpy as np
import sys

latency = []
for line in sys.stdin: 
    latency.append(int(line))
    
latency=sorted(latency)
lat = len(latency)
print("ave:", np.mean(latency))
# print("50% tail:", latency[int(0.5*lat)])
# print("90% tail:", latency[int(0.9*lat)])
# print("95% tail:", latency[int(0.95*lat)])
# print("99% tail:", latency[int(0.99*lat)])
