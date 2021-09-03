import numpy as np 
import sys

cnt=0
res=0
for line in sys.stdin: 
    if "benchmark: " in line:
        temp = line.split()
        lat = float(temp[-2])
        if lat > 0:
            res += lat
            cnt += 1

print("ave", res/cnt)