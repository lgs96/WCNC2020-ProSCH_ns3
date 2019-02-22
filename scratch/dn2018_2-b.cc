//* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   Author: Marco Miozzo <marco.miozzo@cttc.es>
 *           Nicola Baldo  <nbaldo@cttc.es>
 *
 *   Modified by: Marco Mezzavilla < mezzavilla@nyu.edu>
 *        	 	  Sourjya Dutta <sdutta@nyu.edu>
 *        	 	  Russell Ford <russell.ford@nyu.edu>
 *        		  Menglei Zhang <menglei@nyu.edu>
 */


#include "ns3/point-to-point-module.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/packet.h>
#include <ns3/tag.h>
#include <ns3/lte-helper.h>
#include <ns3/epc-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/lte-module.h>
#include <ns3/enum.h>
#include "ns3/buildings-propagation-loss-model.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
  Simulation for LTE downlink tcp/udp performance by gsoul 2018-12-06.
  Original code by NYU 2016
 */
NS_LOG_COMPONENT_DEFINE ("dn2018_2-b");

class MyAppTag : public Tag
{
	public:
		MyAppTag ()
		{
		}

		MyAppTag (Time sendTs) : m_sendTs (sendTs)
	{
	}

		static TypeId GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::MyAppTag")
				.SetParent<Tag> ()
				.AddConstructor<MyAppTag> ();
			return tid;
		}

		virtual TypeId  GetInstanceTypeId (void) const
		{
			return GetTypeId ();
		}

		virtual void  Serialize (TagBuffer i) const
		{
			i.WriteU64 (m_sendTs.GetNanoSeconds());
		}

		virtual void  Deserialize (TagBuffer i)
		{
			m_sendTs = NanoSeconds (i.ReadU64 ());
		}

		virtual uint32_t  GetSerializedSize () const
		{
			return sizeof (m_sendTs);
		}

		virtual void Print (std::ostream &os) const
		{
			std::cout << m_sendTs;
		}

		Time m_sendTs;
};


class MyApp : public Application
{
	public:

		MyApp ();
		virtual ~MyApp();
		void ChangeDataRate (DataRate rate);
		void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);



	private:
		virtual void StartApplication (void);
		virtual void StopApplication (void);

		void ScheduleTx (void);
		void SendPacket (void);

		Ptr<Socket>     m_socket;
		Address         m_peer;
		uint32_t        m_packetSize;
		uint32_t        m_nPackets;
		DataRate        m_dataRate;
		EventId         m_sendEvent;
		bool            m_running;
		uint32_t        m_packetsSent;
};

MyApp::MyApp ()
	: m_socket (0),
	m_peer (),
	m_packetSize (0),
	m_nPackets (0),
	m_dataRate (0),
	m_sendEvent (),
	m_running (false),
	m_packetsSent (0)
{
}

MyApp::~MyApp()
{
	m_socket = 0;
}

	void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
	m_socket = socket;
	m_peer = address;
	m_packetSize = packetSize;
	m_nPackets = nPackets;
	m_dataRate = dataRate;
}

	void
MyApp::ChangeDataRate (DataRate rate)
{
	m_dataRate = rate;
}

	void
MyApp::StartApplication (void)
{
	m_running = true;
	m_packetsSent = 0;
	m_socket->Bind ();
	m_socket->Connect (m_peer);
	SendPacket ();
}

	void
MyApp::StopApplication (void)
{
	m_running = false;

	if (m_sendEvent.IsRunning ())
	{
		Simulator::Cancel (m_sendEvent);
	}

	if (m_socket)
	{
		m_socket->Close ();
	}
}

	void
MyApp::SendPacket (void)
{
	Ptr<Packet> packet = Create<Packet> (m_packetSize);
	MyAppTag tag (Simulator::Now ());

	m_socket->Send (packet);
	if (++m_packetsSent < m_nPackets)
	{
		ScheduleTx ();
	}
}



	void
MyApp::ScheduleTx (void)
{
	if (m_running)
	{
		Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
		m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
	}
}

	static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << newCwnd << std::endl;
}


	static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
}


/*
   static void Rx (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet, const Address &from)
   {
 *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize()<< std::endl;
 }
 */
/*static void Sstresh (Ptr<OutputStreamWrapper> stream, uint32_t oldSstresh, uint32_t newSstresh)
  {
 *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldSstresh << "\t" << newSstresh << std::endl;
 }*/

	void
ChangeSpeed(Ptr<Node>  n, Vector speed)
{
	n->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (speed);
}

uint64_t lastTotalRx[100];
uint16_t count[100];
double t_total[100];

	void
CalculateThroughput (Ptr<OutputStreamWrapper> stream , Ptr <PacketSink> sink,uint16_t i)
{
	Time now = Simulator::Now();
	double cur = (sink->GetTotalRx() - lastTotalRx[i])*(double)8/1e4;
	count[i]++;
	t_total[i] += cur;
	*stream->GetStream() << now.GetSeconds()<<"\t"<<cur<<"\t"<<(double)(t_total[i]/count[i])<<std::endl;
	lastTotalRx[i] = sink->GetTotalRx();
	Simulator::Schedule(MilliSeconds(10),&CalculateThroughput,stream,sink,i);
}


static void
Loss (Ptr<OutputStreamWrapper> stream, uint64_t received, uint32_t loss){
	*stream->GetStream()<<Simulator::Now().GetSeconds()<<"\t"<<loss<<"\t"<<received<<std::endl;
} 

	int
main (int argc, char *argv[])
{
	//	LogComponentEnable ("MmWaveUePhy", LOG_LEVEL_DEBUG);
	//	LogComponentEnable ("MmWaveEnbPhy", LOG_LEVEL_DEBUG);
	//	LogComponentEnable ("MmWaveFlexTtiMacScheduler", LOG_LEVEL_DEBUG);
	//	LogComponentEnable ("MmWaveFlexTtiMaxWeightMacScheduler", LOG_LEVEL_DEBUG);
	LogComponentEnable("TcpCongestionOps",LOG_LEVEL_INFO);
	/*
	 * scenario 1: 1 building;
	 * scenario 2: 3 building;
	 * scenario 3: 6 random located small building, simulate tree and human blockage.
	 * */
	int scenario = 3;
	double stopTime = 30;
	double simStopTime = 20;
	bool harqEnabled = false;
	bool rlcAmEnabled = false;
	bool tcp = false;
	bool isNewReno = false;
	bool ForLoss = false;
	CommandLine cmd;
	//	cmd.AddValue("numEnb", "Number of eNBs", numEnb);
	//	cmd.AddValue("numUe", "Number of UEs per eNB", numUe);
	cmd.AddValue("simTime", "Total duration of the simulation [s])", simStopTime);
	//	cmd.AddValue("interPacketInterval", "Inter-packet interval [us])", interPacketInterval);
	cmd.AddValue("harq", "Enable Hybrid ARQ", harqEnabled);
	cmd.AddValue("rlcAm", "Enable RLC-AM", rlcAmEnabled);
	cmd.AddValue("tcp", "Enable tcp communication",tcp);
	cmd.AddValue("isNewReno","Enable tcp newreno or tcp vegas",isNewReno);
	cmd.AddValue("ForLoss","losscounter",ForLoss);
	cmd.Parse(argc, argv);

	//	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));
	//	Config::SetDefault ("ns3::PointToPointNetDevice::Mtu", UintegerValue (3000));
	//	Config::SetDefault ("ns3::VirtualNetDevice::Mtu", UintegerValue (3000));
	Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth",UintegerValue (50));
	Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (10 * 1024));
	//	Config::SetDefault ("ns3::LteRlcUmLowLat::MaxTxBufferSize", UintegerValue (1024 * 1024));
		Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (131072*50));
		Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (131072*50));
	//	Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue(1));
	//	Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue(72));
	Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (2));
	//	Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(harqEnabled));
	//	Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(true));
	//	Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue(true));
	//	Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(true));
	//	Config::SetDefault ("ns3::MmWaveBeamforming::LongTermUpdatePeriod", TimeValue (MilliSeconds (100.0)));*/
	Config::SetDefault ("ns3::LteRlcAm::PollRetransmitTimer", TimeValue(MilliSeconds(4.0)));
	Config::SetDefault ("ns3::LteRlcAm::ReorderingTimer", TimeValue(MilliSeconds(2.0)));
	Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(1.0)));
	Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MilliSeconds(4.0)));
	//	Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (20 *1024 * 1024));

	double TxPower = 10;
	Config::SetDefault ("ns3::LteEnbPhy::TxPower",DoubleValue(TxPower));
	
	//   uint16_t TcpName = 0;
	   if(isNewReno){
	   Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
	   }
	   else{
	   Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
	//   TcpName = 1;
	   }
	 
	//	Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
	/*
	   lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::BuildingsPropagationLossModel"));
	   lteHelper->Initialize();
	   lteHelper->SetHarqEnabled(true);
	 */
	//	Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
	//	LteHelper->SetEpcHelper (epcHelper);

	Ptr<LteHelper> mmwaveHelper = CreateObject<LteHelper> ();
	Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
	mmwaveHelper->SetEpcHelper (epcHelper);
	Config::SetDefault ("ns3::RrFfMacScheduler::HarqEnabled", BooleanValue (harqEnabled));
	Config::SetDefault ("ns3::PfFfMacScheduler::HarqEnabled", BooleanValue (harqEnabled));	
	Config::SetDefault ("ns3::CqaFfMacScheduler::HarqEnabled", BooleanValue (harqEnabled));
	mmwaveHelper->SetSchedulerAttribute ("HarqEnabled",  BooleanValue (harqEnabled));
	//	mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisPropagationLossModel"));
	mmwaveHelper->Initialize();
	//	mmwaveHelper->SetHarqEnabled(true);
	mmwaveHelper->SetSchedulerType ("ns3::RrFfMacScheduler");
	Ptr<Node> pgw = epcHelper->GetPgwNode ();

	// Create a single RemoteHost
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create (1);
	Ptr<Node> remoteHost;
	InternetStackHelper internet;
	internet.Install (remoteHostContainer);
	Ipv4Address remoteHostAddr[1];
	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	// Create the Internet
	for(uint16_t i = 0; i<1;i++){
		remoteHost = remoteHostContainer.Get(i);
		PointToPointHelper p2ph;
		p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
		p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
		p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.002)));
		NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

        	Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
	      	em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
		internetDevices.Get(0)->SetAttribute("ReceiveErrorModel",PointerValue(em));
		Ipv4AddressHelper ipv4h;
		std::ostringstream subnet;
		subnet<<i+1<<".1.0.0";
		ipv4h.SetBase (subnet.str().c_str(), "255.255.0.0");
		Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
		// interface 0 is localhost, 1 is the p2p device
		//	Ipv4Address remoteHostAddr;
		remoteHostAddr[i] = internetIpIfaces.GetAddress (1);
		//	Ipv4StaticRoutingHelper ipv4RoutingHelper;
		Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
		remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.0.0"), 1);
	}

	switch (scenario)
	{
		case 1:
			{
				Ptr < Building > building;
				building = Create<Building> ();
				building->SetBoundaries (Box (40.0,60.0,
							0.0, 6,
							0.0, 15.0));
				break;
			}
		case 2:
			{
				Ptr < Building > building1;
				building1 = Create<Building> ();
				building1->SetBoundaries (Box (60.0,64.0,
							0.0, 2.0,
							0.0, 1.5));

				Ptr < Building > building2;
				building2 = Create<Building> ();
				building2->SetBoundaries (Box (60.0,64.0,
							6.0, 8.0,
							0.0, 15.0));

				Ptr < Building > building3;
				building3 = Create<Building> ();
				building3->SetBoundaries (Box (60.0,64.0,
							10.0, 11.0,
							0.0, 15.0));
				break;
			}
		case 3:
			{
				Ptr < Building > building1;
				building1 = Create<Building> ();
				building1->SetBoundaries (Box (69.5,70.0,
							4.5, 5.0,
							0.0, 1.5));

				Ptr < Building > building2;
				building2 = Create<Building> ();
				building2->SetBoundaries (Box (60.0,60.5,
							9.5, 10.0,
							0.0, 1.5));

				Ptr < Building > building3;
				building3 = Create<Building> ();
				building3->SetBoundaries (Box (54.0,54.5,
							5.5, 6.0,
							0.0, 1.5));
				Ptr < Building > building4;
				building1 = Create<Building> ();
				building1->SetBoundaries (Box (60.0,60.5,
							6.0, 6.5,
							0.0, 1.5));

				Ptr < Building > building5;
				building2 = Create<Building> ();
				building2->SetBoundaries (Box (70.0,70.5,
							0.0, 0.5,
							0.0, 1.5));

				Ptr < Building > building6;
				building3 = Create<Building> ();
				building3->SetBoundaries (Box (50.0,50.5,
							4.0, 4.5,
							0.0, 1.5));
				break;
				break;
			}
		default:
			{
				NS_FATAL_ERROR ("Invalid scenario");
			}
	}


	NodeContainer ueNodes;
	NodeContainer enbNodes;

	enbNodes.Create(1);
	//	row =4;
	//	col =4;
	ueNodes.Create(1);
	Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	enbPositionAlloc->Add (Vector (0.0, 0.0, 20));
	MobilityHelper enbmobility;
	enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	enbmobility.SetPositionAllocator(enbPositionAlloc);
	enbmobility.Install (enbNodes);
	BuildingsHelper::Install (enbNodes);
	MobilityHelper uemobility;
	uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
	uemobility.Install (ueNodes);


	ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (1000, 0, 1.5));
	ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (-100, 0, 0));

//	ueNodes.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (-1000,0,1.5));
//	ueNodes.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (100,0,0));
	/*	
		for (uint16_t i = 0;  i<row; i++){
		for (uint16_t j = 0; j<col; j++){
		uePositionAlloc -> Add (Vector (300-150*i,300+150*j,1.5));
		}
		}

		mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
		"Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)));
	 */
	//	Simulator::Schedule (Seconds (2), &ChangeSpeed, ueNodes.Get (0), Vector (0, 1.5, 0));
	//	Simulator::Schedule (Seconds (22), &ChangeSpeed, ueNodes.Get (0), Vector (0, 0, 0));

	BuildingsHelper::Install (ueNodes);

	// Install LTE Devices to the nodes
	NetDeviceContainer enbDevs = mmwaveHelper->InstallEnbDevice (enbNodes);
	NetDeviceContainer ueDevs = mmwaveHelper->InstallUeDevice (ueNodes);

	// Install the IP stack on the UEs
	// Assign IP address to UEs, and install applications
	internet.Install (ueNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

	mmwaveHelper->AttachToClosestEnb (ueDevs, enbDevs);
	mmwaveHelper->EnableTraces ();

	// Set the default gateway for the UE
	for(uint32_t u =0; u < ueNodes.GetN(); u++){
		Ptr<Node> ueNode = ueNodes.Get (u);
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}

	ApplicationContainer sinkApps;

	/*
	   Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
	   em->SetAttribute ("ErrorRate", DoubleValue (0.00001));

	   for(uint32_t u = 0; u<ueNodes.GetN();u++){
	   remoteHostContainer.Get(u)->SetAttribute("ReceiveErrorModel",PointerValue(em));
	   }*/
	//	devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

	std::string Tcp;

	if(isNewReno){
		Tcp = "NewReno";
	}
	else{
		Tcp = "Vegas";
	}

	for(uint32_t u = 0; u<ueNodes.GetN();++u){
		if(tcp)
		{
			// Install and start applications on UEs and remote host
			uint16_t sinkPort = 20000;

			Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (u), sinkPort));
			PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
			sinkApps.Add(packetSinkHelper.Install (ueNodes.Get (u)));

			sinkApps.Start (Seconds (0.5));
			sinkApps.Stop (Seconds (simStopTime));

			Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (remoteHostContainer.Get (u), TcpSocketFactory::GetTypeId ());
			Ptr<MyApp> app = CreateObject<MyApp> ();
			app->Setup (ns3TcpSocket, sinkAddress, 1040, 0xffffffff, DataRate ("10Mb/s"));

			remoteHostContainer.Get (u)->AddApplication (app);

			AsciiTraceHelper asciiTraceHelper;

			std::ostringstream fileName_1;
			fileName_1<<"b-Tcp_Window_"<<Tcp<<"_node"<<u<<".txt";
			Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (fileName_1.str().c_str());
			ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));

			std::ostringstream fileName_2;
			fileName_2<<"b-Tcp_Rtt_"<<Tcp<<"_node"<<u<<".txt";
			Ptr<OutputStreamWrapper> stream4 = asciiTraceHelper.CreateFileStream (fileName_2.str().c_str());
			ns3TcpSocket->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange, stream4));

			std::ostringstream fileName_3;
			fileName_3<<"b-Tcp_Throughput_"<<Tcp<<"_node"<<u<<".txt";
			Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (fileName_3.str().c_str());
			Simulator::Schedule (Seconds(1),&CalculateThroughput,stream2,sinkApps.Get(u)->GetObject<PacketSink>(),u);

			//Ptr<OutputStreamWrapper> stream3 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-sstresh-newreno.txt");
			//ns3TcpSocket->TraceConnectWithoutContext("SilowStartThreshold",MakeBoundCallback (&Sstresh, stream3));
			app->SetStartTime (Seconds (1));
			app->SetStopTime (Seconds (stopTime));
		}

		else if(ForLoss){

			uint16_t sinkPort = 8080;
			Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (u), sinkPort));


			UdpServerHelper udpServerHelper (sinkPort);
			ApplicationContainer sinkApps = udpServerHelper.Install (ueNodes.Get (u));

			sinkApps.Start (Seconds (0.5));
			sinkApps.Stop (Seconds (simStopTime));

			AsciiTraceHelper asciiTraceHelper;

			std::ostringstream fileName_1;
			fileName_1<<"b-Udp_Loss.txt";
			Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (fileName_1.str().c_str());
			sinkApps.Get(0)->TraceConnectWithoutContext ("Loss",MakeBoundCallback(&Loss,stream1));		

			UdpClientHelper dlClient (ueIpIface.GetAddress(u), sinkPort);
			uint32_t packetSize = 1040;
			DataRate udpRate =DataRate("10Mbps");
			double interPacketInterval = packetSize * 8 / static_cast<double> (udpRate.GetBitRate ());
			dlClient.SetAttribute ("Interval", TimeValue (Seconds(interPacketInterval)));
			dlClient.SetAttribute ("MaxPackets", UintegerValue(0xFFFFFFFF));
			dlClient.SetAttribute ("PacketSize", UintegerValue(1040));
			ApplicationContainer clientApps = dlClient.Install (remoteHostContainer.Get(u));

			clientApps.Start(Seconds(1.0));
			clientApps.Stop(Seconds(stopTime));

		}	
		else
		{
			// Install and start applications on UEs and remote host
			uint16_t sinkPort = 20000;

			Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (u), sinkPort));
			PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
			sinkApps.Add(packetSinkHelper.Install (ueNodes.Get (u)));

			sinkApps.Start (Seconds (0.5));
			sinkApps.Stop (Seconds (simStopTime));

			Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (remoteHostContainer.Get (u), UdpSocketFactory::GetTypeId ());
			Ptr<MyApp> app = CreateObject<MyApp> ();
			app->Setup (ns3UdpSocket, sinkAddress, 1040, 0xffffffff, DataRate ("10Mb/s"));

			remoteHostContainer.Get (u)->AddApplication (app);
			/*   AsciiTraceHelper asciiTraceHelper;
			     Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream ("mmWave-udp-data-am.txt");
			     sinkApps.Get(0)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream2));
			 */
			AsciiTraceHelper asciiTraceHelper;		

			std::ostringstream fileName_3;
			fileName_3<<"b-Udp_Throughput_node"<<u<<".txt";
			Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (fileName_3.str().c_str());
			Simulator::Schedule (Seconds(1),&CalculateThroughput,stream2,sinkApps.Get(u)->GetObject<PacketSink>(),u);

			app->SetStartTime (Seconds (1));
			app->SetStopTime (Seconds (stopTime));

		}
	}

	//p2ph.EnablePcapAll("mmwave-sgi-capture");
	BuildingsHelper::MakeMobilityModelConsistent ();
	//	Config::Set ("/NodeList/*/DeviceList/*/TxQueue/MaxPackets", UintegerValue (1000*1000));

	Simulator::Stop (Seconds (simStopTime));
	Simulator::Run ();
	Simulator::Destroy ();

	return 0;

}
