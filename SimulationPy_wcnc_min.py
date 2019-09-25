#!/usr/bin/python

import sys
import os
import subprocess
from multiprocessing import Pool
	

def start_simulation(data):
	buildingIndex =data[0]
	x2_delay = data[1]
	s1_delay = data[2]
	buildingNum = data[3]
	throughput = data[4]
	scheme = data[5]
	location = scheme+"_"+str(buildingIndex)+"index_"+x2_delay+"ms_"+s1_delay+"ms_"+buildingNum+"buildings_"+throughput
	try:	
		subprocess.check_call('mkdir %s'%(location),shell=True)
		subprocess.check_call('cp %s %s/%s'%("BuildingFolder/"+str(buildingIndex)+"_BuildingPosition.txt",location,str(buildingIndex)+"_BuildingPosition.txt"),shell=True)
		subprocess.check_call('./waf --cwd=%s --command-template="%%s --BuildingIndex=%d --X2LinkDelay=%s  --S1Delay=%s --BuildingNum=%s --SourceRate=%s" --run wcnc2020_min' % (location,buildingIndex, x2_delay,s1_delay, buildingNum, throughput),shell=True)
	except: 
		#subprocess.check_call('rm -r %s'%(location),shell=True)
		#start_simulation(data)
		pass

#if len(sys.argv) != 5:
#	print "usage: ./parallel [max_homes] [step] [sub_runs] [processes]"
#	sys.exit(0)

#max_homes = int(sys.argv[1])
#step = int(sys.argv[2])
#sub_runs = int(sys.argv[3])
#processes = int(sys.argv[4])

# create params
#params = []
#run = 1
#for i in range(0, max_homes+step, step):
#	homes = i
#	if homes == 0 and step > 1:
#		homes = 1
#	elif homes == 0 and step == 1:
#		continue
#	for j in range(sub_runs):
#		params.append([run, j+1, homes])
#	run += 1

buildingNumSet =['100']
x2DelaySet = ['1','5','10','20']
throughputSet = ['500Mbps']
s1DelaySet = ['0.030']
schemeSet = ['X2','ProxyMin','PBH']
whichScheme = 1
paramsSet = []
for  i in range(100,200):
	index = i+1
	for j in range(len(buildingNumSet)):
		buildingNum = buildingNumSet[j]
		for k in range(len(x2DelaySet)):
			for m in range(len(s1DelaySet)):
				for n in range(len(throughputSet)):
					x2Delay = x2DelaySet[k]
					throughput = throughputSet[n]
					s1Delay = s1DelaySet[m]
					scheme = schemeSet[whichScheme]
					params = [index,x2Delay,s1Delay,buildingNum,throughput,scheme]
					paramsSet.append(params)

# run)
for i in range(len(paramsSet)):
	start_simulation(paramsSet[i])
#start_simulation(paramsSet[0])



