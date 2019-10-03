#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
//#include "ns3/gtk-config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/random-variable-stream.h>
#include <ns3/lte-ue-net-device.h>

#include <iostream>
#include <ctime>
#include <stdlib.h>
#include <list>


using namespace ns3;
using namespace std;

int main()
{
	int BuildingStart = 200;
	int BuildingIndex = 300;
	int BuildingNum = 500;
	int Building_xlim_low = 5;
	int Building_xlim_high = 195;
	int Building_ylim_low = 5;
	int Building_ylim_high = 395;

	BuildingList::Iterator marker;

	for(int  j = BuildingStart; j < BuildingIndex; j++){
		for (int i = 0; i<BuildingNum; i++){

			double xcoordinate = (double)((int)rand()%(Building_xlim_high-Building_xlim_low)+Building_xlim_low);
			double ycoordinate = (double)((int)rand()%(Building_ylim_high-Building_ylim_low)+Building_ylim_low);
			double xlength = rand()%6+1;
			double ylength = rand()%6+1;

			Ptr<Building> building1;
			building1 = Create<Building>();
			building1->SetBoundaries(Box(xcoordinate, xcoordinate + xlength, ycoordinate, ycoordinate + ylength,0,35));

		}
	
		int count = 0;

		std::ostringstream buildingfile;
		buildingfile <<j+1<<"_BuildingPosition.txt";	
		AsciiTraceHelper asciiTraceHelper_build;
		Ptr<OutputStreamWrapper> build_stream = asciiTraceHelper_build.CreateFileStream(buildingfile.str().c_str());		
		
		for (BuildingList::Iterator it = BuildingList::Begin(); it != BuildingList::End(); ++it) {
			if(count < (j-BuildingStart)*BuildingNum)		
				count = count+1;
			else
			{
				//std::cout<<"write"<<std::endl;
				Box box = (*it)->GetBoundaries();
				*build_stream->GetStream() << box.xMin << ":" << box.xMax << ":" << box.yMin << ":" << box.yMax << std::endl;	
			}
		}	
	}
}
