/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FifthScriptExample");

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                10 Mbps, 2 ms
//
//
// We want to look at changes in the ns-3 TCP congestion window.  We need
// to crank up a flow and hook the CongestionWindow attribute on the socket
// of the sender.  Normally one would use an on-off application to generate a
// flow, but this has a couple of problems.  First, the socket of the on-off 
// application is not created until Application Start time, so we wouldn't be 
// able to hook the socket (now) at configuration time.  Second, even if we 
// could arrange a call after start time, the socket is not public so we 
// couldn't get at it.
//
// So, we can cook up a simple version of the on-off application that does what
// we want.  On the plus side we don't need all of the complexity of the on-off
// application.  On the minus side, we don't have a helper, so we have to get
// a little more involved in the details, but this is trivial.
//
// So first, we create a socket and do the trace connect on it; then we pass 
// this socket into the constructor of our simple application which we then 
// install in the source node.
// ===========================================================================
//
class MyApp : public Application 
{
	public:

		MyApp ();
		virtual ~MyApp();

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
	*stream->GetStream()<<Simulator::Now ().GetSeconds () << "\t" << newCwnd<<std::endl;
}
/*
   static void
   RxDrop (Ptr<const Packet> p)
   {
   NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
   }
 */

	static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
}

uint64_t lastTotalRx = 0;
uint16_t count = 0;
double t_total = 0;
	void
CalculateThroughput (Ptr<OutputStreamWrapper> stream , Ptr <PacketSink> sink)
{
	Time now = Simulator::Now();
	double cur = (sink->GetTotalRx() - lastTotalRx)*(double)8/1e4;
	count++;
	t_total += cur;
	*stream->GetStream() << now.GetSeconds()<<"\t"<<cur<<"\t"<<(double)(t_total/count)<<std::endl;
	lastTotalRx = sink->GetTotalRx();
	Simulator::Schedule(MilliSeconds(10),&CalculateThroughput,stream,sink);
}

	double ack_throughput[100];

		static void
	Reset_ack(Ptr<OutputStreamWrapper> stream, uint16_t i)
	{
		std::cout<<"Simulator time: "<<Simulator::Now().GetSeconds()<<" for "<<i+1<<std::endl;
		*stream->GetStream () << Simulator::Now().GetSeconds()<<"\t"<<ack_throughput[i]/1e5<<std::endl;
		ack_throughput[i] = 0;
		Simulator::Schedule (MilliSeconds(100), &Reset_ack,stream,i);
	}

		static void
	RxChange (Ptr<OutputStreamWrapper> stream, uint16_t i, const Ptr<const Packet> packet, const TcpHeader &header, const Ptr<const TcpSocketBase> socket)
	{
//		std::cout<<"Rx"<<std::endl;
		ack_throughput[i] += 54*8;
	}

		static void
	Traces(uint16_t nodeNum)
	{
		AsciiTraceHelper asciiTraceHelper;
	/*
		std::ostringstream pathCW;
		pathCW<<"/NodeList/"<<nodeNum+2<<"/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";

		std::ostringstream fileCW;
		fileCW<<"tcp_cwnd_ue"<<nodeNum+1<<".txt";

		std::ostringstream pathRTT;
		pathRTT<<"/NodeList/"<<nodeNum+2<<"/$ns3::TcpL4Protocol/SocketList/0/RTT";

		std::ostringstream fileRTT;
		fileRTT<<"tcp_rtt_ue"<<nodeNum+1<<".txt";
	*//*
		std::ostringstream pathRCWnd;
		pathRCWnd<<"/NodeList/"<<nodeNum+2<<"/$ns3::TcpL4Protocol/SocketList/0/RWND";

		std::ostringstream fileRCWnd;
		fileRCWnd<<"tcp_rcwnd_ue"<<nodeNum+1<<".txt";
	*/
		std::ostringstream pathRx;
		pathRx<<"/NodeList/"<<nodeNum<<"/$ns3::TcpL4Protocol/SocketList/0/Rx";

		std::ostringstream fileRx;
		fileRx<<"tcp_ack_ue"<<nodeNum+1<<".txt";
	/*
		Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (fileCW.str ().c_str ());
		Config::ConnectWithoutContext (pathCW.str ().c_str (), MakeBoundCallback(&CwndChange, stream1));

		Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (fileRTT.str ().c_str ());
		Config::ConnectWithoutContext (pathRTT.str ().c_str (), MakeBoundCallback(&RttChange, stream2));
	*//*
		Ptr<OutputStreamWrapper> stream4 = asciiTraceHelper.CreateFileStream (fileRCWnd.str ().c_str ());
		Config::ConnectWithoutContext (pathRCWnd.str ().c_str (), MakeBoundCallback(&CwndChange, stream4));
	*/
		Ptr<OutputStreamWrapper> stream5 = asciiTraceHelper.CreateFileStream (fileRx.str ().c_str ());
		Config::ConnectWithoutContext (pathRx.str ().c_str (), MakeBoundCallback(&RxChange, stream5, nodeNum));
	}
static void
Loss (Ptr<OutputStreamWrapper> stream, uint64_t received, uint32_t loss){
	*stream->GetStream()<<Simulator::Now().GetSeconds()<<"\t"<<loss<<"\t"<<received<<std::endl;
}
	int 
main (int argc, char *argv[])
{

	
	// Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (ns3::LteEnbRrc::RLC_AM_ALWAYS));
//	Config::SetDefault("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(3));
//	Config::SetDefault("ns3::McUePdcp::EnableReordering", BooleanValue(isEnablePdcpReordering));
//	Config::SetDefault("ns3::McEnbPdcp::EnableDuplication", BooleanValue(isDuplication));
//	Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlcAmEnabled));
//	Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(harqEnabled));
//	Config::SetDefault ("ns3::MmWaveEnbPhy::TxPower",DoubleValue(TxPower));
//	Config::SetDefault ("ns3::MmWaveSpectrumPhy::DisableInterference",BooleanValue(true));
//	Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
//	Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
//	Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::FixedTti", BooleanValue(fixedTti));
//	Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::SymPerSlot", UintegerValue(6));
//	Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue(1));
	//Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue(72));
//	Config::SetDefault ("ns3::MmWavePhyMacCommon::SymbolsPerSubframe", UintegerValue(symPerSf));
//	Config::SetDefault ("ns3::MmWavePhyMacCommon::SubframePeriod", DoubleValue(sfPeriod));
//	Config::SetDefault ("ns3::MmWavePhyMacCommon::TbDecodeLatency", UintegerValue(200.0));
//	Config::SetDefault ("ns3::MmWavePhyMacCommon::NumHarqProcess", UintegerValue(100));
//	Config::SetDefault ("ns3::MmWaveBeamforming::LongTermUpdatePeriod", TimeValue (MilliSeconds (100.0)));
//	Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkWidth",DoubleValue(13.889e6));//1000MHz bandwidth
//	Config::SetDefault ("ns3::LteEnbRrc::SystemInformationPeriodicity", TimeValue (MilliSeconds (5.0)));
	// Config::SetDefault ("ns3::MmWavePropagationLossModel::ChannelStates", StringValue ("n"));
//	Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
//	Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
//	Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
//	Config::SetDefault ("ns3::LteEnbRrc::FirstSibTime", UintegerValue (2));
//	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDelay", TimeValue (MicroSeconds(x2Latency)));
//	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDataRate", DataRateValue(DataRate ("1000Gb/s")));
//	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkMtu",  UintegerValue(10000));
//	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1uLinkDelay", TimeValue (MicroSeconds(1000)));
//	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1apLinkDelay", TimeValue (MicroSeconds(mmeLatency)));
//	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
	Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1024*1024*1000));
	Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1024*1024*1000));
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));

//	Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (100 * 1024 * 1024));
//	Config::SetDefault ("ns3::LteRlcUmLowLat::MaxTxBufferSize", UintegerValue (100 * 1024 * 1024));
//	Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(1.0)));
//	Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (100 *1024 * 1024));

//	Config::SetDefault ("ns3::PointToPointEpcHelper::X2LinkDelay", TimeValue (MilliSeconds(x2LinkDelay)));
//	Config::SetDefault ("ns3::PointToPointEpcHelper::X2LinkDataRate", DataRateValue (DataRate(X2dataRate)));

	LogComponentEnable("TcpCongestionOps",LOG_LEVEL_INFO);

	bool tcp =true;
	bool isNewReno = true;
	bool ForLoss = false;

	CommandLine cmd;
	cmd.AddValue("tcp","Enable tcp communcation",tcp);
	cmd.AddValue("isNewReno","Enable tcp newreno or tcp vegas",isNewReno);
	cmd.Parse (argc, argv);

	if(isNewReno){
		Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
	}
	else{
		Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
	}

	NodeContainer nodes;
	nodes.Create (2);

	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

	NetDeviceContainer devices;
	devices = pointToPoint.Install (nodes);

//	Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
//	em->SetAttribute ("ErrorRate", DoubleValue (0.0000));
//	devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

	InternetStackHelper stack;
	stack.Install (nodes);

	Ipv4AddressHelper address;
	address.SetBase ("10.1.1.0", "255.255.255.252");
	Ipv4InterfaceContainer interfaces = address.Assign (devices);

	if(tcp){
		uint16_t sinkPort = 8080;
		Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));
		PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
		ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
		sinkApps.Start (Seconds (0.5));
		sinkApps.Stop (Seconds (30.));

		Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
		AsciiTraceHelper asciiTraceHelper;

		std::string Tcp;

		if(isNewReno){
			Tcp="NewReno";
		}
		else{
			Tcp="Vegas";
		}

		std::ostringstream fileName_1;
		fileName_1<<"a-Tcp_Window_"<<Tcp<<".txt";
		Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (fileName_1.str().c_str());
		ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));

		std::ostringstream fileName_2;
		fileName_2<<"a-Tcp_Rtt_"<<Tcp<<".txt";
		Ptr<OutputStreamWrapper> stream4 = asciiTraceHelper.CreateFileStream (fileName_2.str().c_str());
		ns3TcpSocket->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange, stream4));

		std::ostringstream fileName_3;
		fileName_3<<"a-Tcp_Throughput_"<<Tcp<<".txt";
		Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (fileName_3.str().c_str());
		Simulator::Schedule (Seconds(1),&CalculateThroughput,stream2,sinkApps.Get(0)->GetObject<PacketSink>());

		std::ostringstream fileName_4;
		fileName_4<<"a-Tcp_Ack_" <<Tcp<<".txt";
		AsciiTraceHelper asciiTraceHelper_4;
		Ptr<OutputStreamWrapper> stream_4 = asciiTraceHelper_4.CreateFileStream(fileName_4.str().c_str());
		Simulator::Schedule (Seconds (0.1),&Reset_ack,stream_4,0);

		Ptr<MyApp> app = CreateObject<MyApp> ();
		app->Setup (ns3TcpSocket, sinkAddress, 1400, 0xffffffff, DataRate ("4000Mbps"));
		nodes.Get (0)->AddApplication (app);

		Simulator::Schedule (Seconds (0.2001), &Traces, 0);

		app->SetStartTime (Seconds (1.));
		app->SetStopTime (Seconds (30.));
	}
	else if(ForLoss){

		uint16_t sinkPort = 8080;
		Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));


		UdpServerHelper udpServerHelper (sinkPort);
		ApplicationContainer sinkApps = udpServerHelper.Install (nodes.Get (1));

		sinkApps.Start (Seconds (0.5));
		sinkApps.Stop (Seconds (30.));


		AsciiTraceHelper asciiTraceHelper;

		std::ostringstream fileName_1;
		fileName_1<<"a-Udp_Loss.txt";
		Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (fileName_1.str().c_str());
		sinkApps.Get(0)->TraceConnectWithoutContext ("Loss",MakeBoundCallback(&Loss,stream1));		

		UdpClientHelper dlClient (interfaces.GetAddress(1), sinkPort);
		uint32_t packetSize = 1040;
		DataRate udpRate =DataRate("10Mbps");
		double interPacketInterval = packetSize * 8 / static_cast<double> (udpRate.GetBitRate ());
		dlClient.SetAttribute ("Interval", TimeValue (Seconds(interPacketInterval)));
		dlClient.SetAttribute ("MaxPackets", UintegerValue(0xFFFFFFFF));
		dlClient.SetAttribute ("PacketSize", UintegerValue(1040));
		ApplicationContainer clientApps = dlClient.Install (nodes.Get(0));
		
		clientApps.Start(Seconds(1.0));
		clientApps.Stop(Seconds(30.0));

	}	
	else{

		uint16_t sinkPort = 8080;
		Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));



		PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
		ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));


		sinkApps.Start (Seconds (0.5));
		sinkApps.Stop (Seconds (30.));


		AsciiTraceHelper asciiTraceHelper;
		Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (nodes.Get (0), UdpSocketFactory::GetTypeId ());


		std::ostringstream fileName_3;
		fileName_3<<"a-Udp_Throughput.txt";
		Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (fileName_3.str().c_str());
		Simulator::Schedule (Seconds(1),&CalculateThroughput,stream2,sinkApps.Get(0)->GetObject<PacketSink>());

		Ptr<MyApp> app = CreateObject<MyApp> ();
		app->Setup (ns3UdpSocket, sinkAddress, 1400, 0xffffffff, DataRate ("4000Mbps"));
		nodes.Get (0)->AddApplication (app);
		app->SetStartTime (Seconds (1.));
		app->SetStopTime (Seconds (30.));

	}
	//  devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));

	Simulator::Stop (Seconds (20));
	Simulator::Run ();
	Simulator::Destroy ();

	return 0;
}

