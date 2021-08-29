import numpy as np
import matplotlib.pyplot as plt
import sys

time_1 = np.arange(0, 200, 50)
print(time_1)
tput_1 = [35, 35, 25, 20]

time_2 = np.arange(200, 2000, 50)
print(time_2)
tput_2 = [35 for t in time_2]

time_3 = np.arange(2000, 2200, 50)
tput_3 = [30, 25, 25, 25]

time_4 = np.arange(2200, 2500, 50)
tput_4 = [35 for t in time_4]

time = np.arange(0, 2500, 50)
tput = tput_1 + tput_2 + tput_3 + tput_4

plt.plot(time, tput)
plt.ylim(0, 50)
plt.savefig("figure/fig7.pdf")
