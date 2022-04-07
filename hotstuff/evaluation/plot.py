import sys
import re
import argparse
import numpy as np
from datetime import datetime, timedelta

def plot_thr(fname, values):
    import matplotlib.pyplot as plt
    x = range(len(values)) # len = n means n seconds
    y = values
    plt.xlabel(r"time in sec")
    plt.ylabel(r"txn/sec")
    plt.plot(x, y)
    plt.savefig(fname)

if __name__ == '__main__':
    plot_thr(
        "n4_udp_pay0_b400_2",
        [63516, 67726, 68822, 66437, 69876, 71279, 68451, 65051, 71310, 69206, 68625, 46641, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1039, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 288, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 459, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 247, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 219, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 108, 0, 0, 24419, 65822, 67547, 69219, 66246, 70535, 71106, 66709, 66128, 72747, 67765, 69159, 21658, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 175, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 112, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 175, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 78]
    )