#!/usr/bin/python

import sys
import os
import subprocess
from multiprocessing import Pool
	

def start_simulation(data):
	buildingIndex =data[0]
	x2_delay = data[1]
	buildingNum = data[2]
	throughput = data[3]
	scheme = data[4]
	location = scheme+"_"+str(buildingIndex)+"index_"+x2_delay+"ms_"+buildingNum+"buildings_"+throughput
	subprocess.check_call('mkdir %s'%(location),shell=True)
	subprocess.check_call('cp %s %s/%s'%(str(buildingIndex)+"_BuildingPosition.txt",location,str(buildingIndex)+"_BuildingPosition.txt"),shell=True)
	subprocess.check_call('./waf --cwd=%s --command-template="%%s --BuildingIndex=%d --X2LinkDelay=%s --BuildingNum=%s --SourceRate=%s" --run globecom2019_pbh' % (location,buildingIndex, x2_delay, buildingNum, throughput),shell=True)

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

buildingNumSet =['80']
x2DelaySet = ['10','20']
throughputSet = ['1000Mbps']
schemeSet = ['X2','Proxy','PBH']
whichScheme = 0
paramsSet = []
for  i in range(0,20,1):
	index = i+1
	for j in range(len(buildingNumSet)):
		buildingNum = buildingNumSet[j]
		for k in range(len(x2DelaySet)):
			x2Delay = x2DelaySet[k]
			throughput = throughputSet[0]
			scheme = schemeSet[whichScheme]
			params = [index,x2Delay,buildingNum,throughput,scheme]
			paramsSet.append(params)

# run)
for i in range(len(paramsSet)):
	start_simulation(paramsSet[i])
#start_simulation(paramsSet[0])



