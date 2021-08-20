f = open('/home/jqi/tps.log', 'r')
# tps=[0 for _ in range(60)]
tps={}
for line in f.readlines():
    strs = line.split(" ")
    timestamp = strs[1][:-3]
    # print(timestamp)
    if timestamp in tps.keys():
        tps[timestamp] = tps[timestamp] + 1
    else:
        tps[timestamp] = 1


print(tps) 
    # print(line) 