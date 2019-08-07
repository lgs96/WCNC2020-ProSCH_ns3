#!/usr/bin/python

import sys
import os
import subprocess
from multiprocessing import Pool
	

def start_simulation(data):
	scheme = data[0]
	downloadSize = data[1]
	sourceRate = data[2]
	x2Delay = data[3]
	serverDelay = data[4]
	channel = data[5]
	bufferSize = data[6]
	location = scheme+"_fileSize"+str(downloadSize)+"_rate"+str(sourceRate)+"_x2Delay"+str(x2Delay)+"_serverDelay"+str(serverDelay)+"_isNLOS"+str(channel)+"_bufSize"+str(bufferSize)
	
	try:
		subprocess.check_call('mkdir %s'%(location),shell=True)
	except:
		pass

	try:
		subprocess.check_call('./waf --cwd=%s --command-template="%%s --downloadSize=%s --SourceRate=%s --X2LinkDelay=%s --serverDelay=%s --channelVariant=%s --bufferSize=%s" --run ictc2019_mmBS' % (location,downloadSize,sourceRate, x2Delay, serverDelay, channel,bufferSize),shell=True)	
	except:
		start_simulation(data)
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
#x2DelaySet = ['1', '5', '10']
x2DelaySet = ['1','5','10','20']
sourceRateSet = ['1000Mbps','500Mbps','300Mbps','100Mbps']
downloadSizeSet = ['100','300','500']
serverDelaySet = ['30','1']
schemeSet = ['noProxy','mmProxy','sub6Proxy']
channelSet = ['0','1']
bufferSizeSet = ['10485760','1048576']

whichScheme = 1 

paramsSet = []
scheme = schemeSet[whichScheme]

for  i in range(len(downloadSizeSet)):
	downloadSize = downloadSizeSet[i];
	for j in range(len(sourceRateSet)):
		sourceRate = sourceRateSet[j]
		for k in range(len(x2DelaySet)):
			x2Delay = x2DelaySet[k]
			for l in range(len(serverDelaySet)):
				serverDelay = serverDelaySet[l]
				for m in range(len(channelSet)):
					channel = channelSet[m]
					for n in range(len(bufferSizeSet)):
						bufferSize = bufferSizeSet[n]
						params = [scheme,downloadSize,sourceRate,x2Delay,serverDelay,channel,bufferSize]
						paramsSet.append(params)
#		if i==0:
#			location = params[4] +"_"+str(params[0])
#			subprocess.check_call('mkdir %s'%(location),shell=True)



# run)


for i in range(0,len(paramsSet)):
	start_simulation(paramsSet[i])
#start_simulation(paramsSet[0])



