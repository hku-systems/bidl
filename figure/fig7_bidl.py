import numpy as np
import matplotlib.pyplot as plt
import sys

timestamps = []
# with open("logs/bidl/malicious/malicious.log") as f:
with open("/home/jqi/logs/normal_0.log") as f:
    for line in f.readlines(): 
        # if "transaction commit throughput" in line:
            temp = line.split()
            timestamps.append(temp[0][6:-1])
print(timestamps)


# plt.plot(time, tput)
# plt.ylim(0, 50)
# plt.savefig("figure/fig7.pdf")
