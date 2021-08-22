import numpy as np
import matplotlib.pyplot as plt
import sys

with open("logs/bidl_performance_tput.log") as f: 
	lines = f.readlines()
	for line in lines[5:]:
		print(line)