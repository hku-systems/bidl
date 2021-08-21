import numpy as np
import sys

latency = []
for line in sys.stdin: 
    latency.append(int(line))
    
if len(sys.argv) == 2 and sys.argv[1] == "endorse":
    lat = len(latency)
    print("ave:", np.mean(latency[int(0.5*lat):]))
else:
    lat = len(latency)
    print("ave:", np.mean(latency))


# latency=sorted(latency)
# print("50% tail:", latency[int(0.5*lat)])
# print("90% tail:", latency[int(0.9*lat)])
# print("95% tail:", latency[int(0.95*lat)])
# print("99% tail:", latency[int(0.99*lat)])
