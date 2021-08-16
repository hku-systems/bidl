#import numpy as np
import sys

valid = 0
invalid = 0
tot = 0

for line in sys.stdin:
    if "end: " in line:
        tot += 1
        if "VALID" in line: 
            valid += 1
        elif "MVCC" in line:
            invalid += 1

print("valid: %d/%d %.2f%% invalid: %d/%d %.2f%%" % (valid, tot, valid/tot*100, invalid, tot, invalid/tot*100))
