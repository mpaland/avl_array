import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
opts = ["insert","find","erase&insert","erase"]
#opts = ["erase"]
fp = open("result.txt","r")
ll = fp.readlines()
fp.close()
plt.subplots(3,3, figsize=(14, 9))
i = 1
for opt in opts:
	y = []
	y1 = []
	startFind = False
	for l in ll:
		arr = l.split("missPercent:")
		if len(arr) > 1 and int(arr[1]) == 10:
			startFind = True
			continue
		elif len(arr) > 1 and int(arr[1]) != 10:
			startFind = False
			continue
		if startFind and l.find(" " + opt + " ") >= 0:
			if l.find("avl") >= 0:
				y.append(int(l.split("QPS:")[1]))
			elif l.find("std::map") >= 0:
				y1.append(int(l.split("QPS:")[1]))
	#print(y)
	#print(y1)
	y = list(map(lambda x: x / 10000, y))
	y1 = list(map(lambda x: x / 10000, y1))
	x1 = [0,1,2,3,4,5,6]
	x2 = ['128','1024','65535','500000','5000000','50000000','60000000']
	if opt == "erase":
		y = y[2:]
		y1 = y1[2:]
		x1 = x1[:-2]
		x2 = x2[2:]
	plt.subplot(2,2,i)
	i += 1
	plt.xticks(x1,x2)
	y2 = [np.max(y),np.mean(y),np.median(y),np.min(y)]
	plt.yticks(y2,list(map(lambda x: str(int(x)) + "w", y2)))
	plt.plot(y, marker = 'o',label="static_avl")
	plt.plot(y1, marker = 'o',label="std::map")
	plt.title(opt)
	plt.xlabel("Size")
	plt.ylabel("QPS(w)")
	plt.legend()
plt.show()