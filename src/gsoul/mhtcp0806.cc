/*
 * Author: Minho Kim (mhkim@ramo.yonsei.ac.kr)
 * Yonsei Univ. in Seoul, South-Korea.
 * This is to simulate and analysis for wireless-TCP
 * TCP + LTE + mmwave + mobility + earlyAck(considering RLC buffer status)
 * modification for window update procedure in a base station. 6/24/2016
 */

#include "ns3/point-to-point-module.h"
#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"

#include "ns3/epc-gtpu-header.h"
#include "ns3/tcp-option-winscale.h"
//#include <algorithm>;
#include "ns3/building.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/mobility-building-info.h>
#include "ns3/internet-trace-helper.h"
#include "ns3/lte-rlc.h"
#include "ns3/lte-rlc-um.h"
#include "ns3/lte-rlc-am.h"
#include "ns3/lte-ue-rrc.h"
#include <ns3/lte-radio-bearer-info.h>
#include <stdio.h>
#include <ns3/log.h>
#include <ns3/building-list.h>
#include <ns3/angles.h>
#include <map>
#include "ns3/packet-metadata.h"
#include <ns3/timer.h>

/*
 *
 *
 *
#include "ns3/point-to-point-module.h"
#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
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
 *
 *
 *
 *
 */

/*
 *
 *     Server ----------[PGW]---------------[eNB]---------------UEs
 *     1.0.0.1       1.0.0.2                                  7.0.0.2~
 *
 *
 *
 * */



Ptr<Node> enb_node;
Ptr<Node> ue_node;
static Ptr<Packet> sample_datapacket;

enum cacheCongestionPolicy {
	  sendAsMuchAsAcked,
	  noSendWhileCongestion,
	  AdditionalSendWhileCongestion,
	  congestionPolicy0313


};



enum cachePriorityPolicy {
	  nlosFirst,
	  evenPriority,
	  losFirst,
	  customizedPriority,
	  proposed0330,
	  cacheOptimal
};

/*Simulation parameters*/

static const bool enableEA = true;
static const bool isAdvancedProxy = enableEA;
static const bool enableCC = true;
static const bool isRlcAm = false;
static const bool enableHarq = true;
static const bool enableSack = false;
static const bool enablePcap = true;
static const bool adaptiveRate = true;

//static const int cachePP = cacheOptimal;
static const int cachePP = proposed0330;
static const int cacheCP = AdditionalSendWhileCongestion;



static const int  numUEs = 2;
static const int  numNlosUEs = 2;
static const int  numExtraUEs = 0;
static const bool massiveSim = false;
static const bool allStationary = false;
static const bool mmwaveTrace = true;
static const bool needLog = true;
static const int  numOfBatch = 50;

double baseXPosition = 4;
double nonLosDuration = 1.0;
double losDuration = 1;
double positionDiffer = 1;


//double dataRate_double = 500000000;
//std::string bandwidthInet = "2Gb/s";

//std::string nameBase = "massiveT-02";
std::string nameBase = "thesis_10_2UE";

static const uint32_t  maxCacheLength = 65534*1000*1.0;







//double simTime = 2 + 4/ueSpeed;
double simTime = 15;
double pump_start=0.001;
double pump_stop = simTime;
double pump_stop_ext = 10;
double sink_start = 0.0;
double sink_stop = simTime;
double sink_stop_ext = 12;
static const Time  extraUeAppear = Seconds(3.5);


static const double congestionRatio = 0.8 ;
static const double needFreeUpRatio = 0.9 ;
static const uint32_t  distance = 300; // meters, between a bs and mobile
double ueSpeed = (double) 1/1; // must be larger than 0
//double ueSpeed = (double) 0.0; // must be larger than 0
Vector ueVelocityMo = Vector( ueSpeed,0,0);
Vector ueVelocitySt = Vector( 0,0,0);
Vector enbPosition = Vector(30,distance,20);
//Vector ueStarting1 = Vector(4,0,1.5);
static const int numBuildings = 30;
//Vector ueStarting1 = Vector(baseXPosition-ueSpeed*2,0,1.5);
//Vector ueStarting1 = Vector(4.5,0,1.5);
Vector ueStarting1 = Vector(2,0,1.5);
Vector ueStarting2 = Vector(10,10,1.5);
Box buildingPosition[numBuildings];

//std::string dataRate = "200Mbps";
//double dataRate_double = 200000000;
//
//std::string dataRateArray[5] = {"200Mbps", "200Mbps", "200Mbps", "200Mbps", "200Mbps"};
//double dataRate_doubleArray[5] = {200000000, 200000000, 200000000, 200000000, 200000000};



static const int arraySize = 5;

std::string dataRateArray[arraySize] = {"100Mbps", "100Mbps", "100Mbps", "100Mbps", "100Mbps"};
double dataRate_doubleArray[arraySize] = {500000000, 500000000, 500000000, 500000000, 500000000};
std::string bandwidthInet[arraySize] = dataRateArray;
std::string bandwidthBsUe = "10Gb/s";
std::string dataRateExtra = "500Mbps";


//std::string dataRateArray[5] = {"500Mbps", "500Mbps", "500Mbps", "500Mbps", "500Mbps"};
//double dataRate_doubleArray[5] = {500000000, 500000000, 500000000, 500000000, 500000000};



//std::string delay = "50ms";
std::string delayArray[arraySize]={"25ms","50ms","75ms","100ms","50ms"};
double delayArrayDouble[arraySize]={ 0.025, 0.05, 0.075, 0.10, 0.05 };


//std::string massiveRateArray[arraySize] = {"5Mbps", "10Mbps", "50Mbps", "100Mbps", "300Mbps"};
//double massiveRateDoubleArray[arraySize] = {5000000, 10000000, 50000000, 100000000, 300000000};
//std::string massiveDelayArray[arraySize]={"50ms","40ms","60ms","40ms","100ms"};
//double massiveDelayDoubleArrary[arraySize]={ 0.05, 0.04, 0.06, 0.04, 0.1 };

//std::string delay1 = "60ms";
//std::string delay2 = "40ms";
//std::string delay3 = "80ms";
std::string delayBsUe = "20ms";


static const uint32_t  segmentSize = 1400;

static double ppsTP[numUEs + numExtraUEs];
static const double muEstinAlpha = 1.5;
static const double outerRTT = 0.002; // 2ms
static const uint32_t  sndBufSize = 131072*1000;
static const uint32_t  rcvBufSize = 65534*1000;
static const double tcpRcvBufRatio = 0.5; // TCP receiver buf size = rcvBufSize/tcpRcvBufRatio
static const uint32_t  ratio_rcvToRLC = 1;
static const uint32_t  numRlcBuf = rcvBufSize/ratio_rcvToRLC/(segmentSize+80) ;
static const uint32_t  maxAddiTrans = 50;
static const double uAlpha = 0.5;   // cache size up, for nlos first algorithm
static const double dAlpha = 0.25;   // cache size down, for nlos first algorithm
static const double uBeta = 0.1;  // cache size up, for los first
static const double dBeta = 0.8;  // cache size down, for los first


static const int  maxCacheNum = 30;
//static const uint32_t  maxCacheLength = rcvBufSize/0.1;

static const int  maxCachesSize = maxCacheLength/segmentSize - 10;
static const uint32_t iCacheSize = maxCachesSize/10;
static const Time mmRTO = Seconds(0.005);
static const Time mmiRTO = Seconds(0.15);
static const Time mmMaxTimer = Seconds(2.0);
static const Time mmiATP = Seconds(0.001); // Additional Transmission Period timer, default
static const Time mmATP = Seconds(0.05); // Additional Transmission Period timer
static const Time mmPrintPeriod = Seconds(0.001);
static const TimeValue  zeroWinProbeTime = Seconds(5);
static const Time  muThresh = Seconds(1);






static const uint16_t  retransNTh = 10;
FILE* log_file;
char fname[255];

std::string tcpProtocol = "TcpNewReno";
int mtu = 1500;
uint32_t packetSize = 1400;
uint32_t nPackets = 2000000;
uint32_t currentRlcBuf = 50000;
uint16_t winfOrg = 0;
uint16_t winfNew = 512;
int basicQueueSize = 400000; // num of packets in queue
int maxPackets = 4000000;
int globalSumRi = 0;
SequenceNumber32 lastWinUpdatedAck(0);
unsigned int lastWinUpdatedWin(0);
SequenceNumber32 lastRetransSeq(0);
//SequenceNumber32 lastAck(0);
//Ptr<Ipv4RoutingProtocol> globalRouting ;




//The available channel scenarios are 'RMa', 'UMa', 'UMi-StreetCanyon', 'InH-OfficeMixed', 'InH-OfficeOpen', 'InH-ShoppingMall'
std::string scenario = "UMa";
std::string condition = "a";

struct cache_result{

	Ipv4Address userID;
	int curSi;
	int curRi;
	int maxRi;
	uint32_t firstSeq;
	SequenceNumber32 lastAck;
	Vector speed ;
	Vector startPos ;
	std::string rate;
	std::string delay;
	double ppstp;

}sim_result[numUEs+numExtraUEs+1];

struct conv_result{

	Ipv4Address userID;
	Vector speed ;
	Vector startPos ;
	std::string rate;
	std::string delay;
	SequenceNumber32 lastAck;

}simConv_result[numUEs+numExtraUEs+1];




using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("mhtcp22");



typedef std::map<SequenceNumber32, Ptr<Packet> >::const_iterator Iterator;
typedef std::map<SequenceNumber32, bool >::const_iterator Iterator_Retrans;
typedef std::map<Time, int >::const_iterator Iterator_Mu;

 /*MyTcp application on server*/

class MyTcp : public Application
{
public:
	MyTcp();
	virtual ~MyTcp();

	void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
	virtual void StartApplication(void);
	virtual void StopApplication(void);
	void ScheduledTx(void);
	void SendPacket(void);
	void TraceRTT(Time oldvalue, Time newvalue);
	void PTraceRTT(Time oldvalue, Time newvalue, MyTcp& tcp);
	void UpdateRate(Time newvalue);

	Ptr<Socket> m_socket;
	Ptr<TcpSocket> m_tcpsocket;
	Ptr<TcpSocketBase> m_tcpbase;
//	Ptr<Tcp> m_tcpsocket;
	Address m_peer;
	uint32_t m_packetSize;
	uint32_t m_nPackets;
	DataRate m_idataRate;
	DataRate m_dataRate;
	EventId m_sendEvent;
	bool m_running;
	uint32_t m_packetsSent;
	Time m_refRTT;

};

MyTcp::MyTcp()
	: m_socket(0),
	  m_tcpsocket(0),
	  m_peer(),
	  m_packetSize(0),
	  m_nPackets(0),
	  m_idataRate(0),
	  m_dataRate(0),
	  m_sendEvent(),
	  m_running(false),
	  m_packetsSent(0),
	  m_refRTT( MilliSeconds(1000) )

{

}

MyTcp::~MyTcp()
{
	m_socket=0;
	m_tcpsocket=0;
}

void MyTcp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
	m_socket = socket;
	m_tcpsocket = m_socket->GetObject<TcpSocket>();
	m_tcpbase = m_socket->GetObject<TcpSocketBase>();
	m_peer = address;
	m_packetSize = packetSize;
	m_nPackets = nPackets;
	m_idataRate = m_dataRate = dataRate;
	m_refRTT = m_tcpbase->GetMinRto();
}

void TTraceRTT(Time oldvalue, Time newvalue)
{
	std::cout << "kkdown RTO: old-" << oldvalue.GetMilliSeconds()
			            << "  new-" << newvalue.GetMilliSeconds()
						<< "\t" << Simulator::Now ().GetSeconds () << std::endl;
	return ;
}


void MyTcp::TraceRTT(Time oldvalue, Time newvalue)
{
	std::cout << "kkdown RTO: old-" << oldvalue.GetMilliSeconds()
			            << "  new-" << newvalue.GetMilliSeconds()<< std::endl;

	std::cout << "data rate:" << m_dataRate.GetBitRate()/1000000 << "M"
		      << "\t" << Simulator::Now ().GetSeconds () << std::endl;

	UpdateRate(newvalue);

	return ;
}

void MyTcp::UpdateRate(Time newvalue)
{

	if(!adaptiveRate)
	{
		std::cout << "updated data rate:" << m_dataRate.GetBitRate()/1000000 << "M"
				      << "\t" << Simulator::Now ().GetSeconds () << std::endl;
		return;
	}


	if(newvalue <= m_refRTT)
	{
		m_dataRate = m_idataRate;
	}else if ( newvalue <= m_refRTT+ MilliSeconds(1000) )
	{
		m_dataRate = DataRate( m_idataRate.GetBitRate() / 2 ) ;
	}else if ( newvalue <= m_refRTT+ MilliSeconds(2000) )
	{
		m_dataRate = DataRate( m_idataRate.GetBitRate() / 4 );
	}else if ( newvalue <= m_refRTT+ MilliSeconds(3000) )
	{
		m_dataRate = DataRate( m_idataRate.GetBitRate() / 6 );
	}else if ( newvalue <= m_refRTT+ MilliSeconds(4000) )
	{
		m_dataRate = DataRate( m_idataRate.GetBitRate() / 8 );
	}else if ( newvalue <= m_refRTT+ MilliSeconds(5000) )
	{
		m_dataRate = DataRate( m_idataRate.GetBitRate() / 10 );
	}else
	{
		m_dataRate = DataRate( m_idataRate.GetBitRate() / 16 );
	}

	std::cout << "updated data rate:" << m_dataRate.GetBitRate()/1000000 << "M"
			      << "\t" << Simulator::Now ().GetSeconds () << std::endl;

	return;
}

void MyTcp::PTraceRTT(Time oldvalue, Time newvalue, MyTcp& tcp)
{
	std::cout << "kkdown RTO: old-" << oldvalue.GetMilliSeconds()
			            << "  new-" << newvalue.GetMilliSeconds()<< std::endl;
	return ;
}


void MyTcp::StartApplication(void)
{
//	std::cout << "kkup 0" << std::endl;
	m_running = true;
	m_packetsSent = 0;
//	std::cout << "kkup 1" << std::endl;
	m_socket->Bind();
//	std::cout << "kkup 2" << std::endl;
	m_socket->Connect(m_peer);
//	std::cout << "kkup 3" << std::endl;
	SendPacket();

//	  PointerValue tmp;
//	  m_tcpsocket->GetAttribute("RcvBufSize", tmp);
//	  UintegerValue rcvbuf = tmp.

	UintegerValue rcvbuf ;
	m_tcpsocket->GetAttribute("RcvBufSize", rcvbuf);
//	Time time = m_tcpbase->GetMinRto();

//	std::cout << "kkup rcvbuf:" << rcvbuf.Get() << std::endl;
//	std::cout << "kkup minRTO:" << m_tcpbase->GetMinRto().GetMilliSeconds() << std::endl;

//	m_tcpbase->TraceConnectWithoutContext("RTO" , MakeCallback(&TraceRTT) );
//	m_tcpbase->TraceConnectWithoutContext("RTO" , MakeCallback(&TTraceRTT) );
	m_tcpbase->TraceConnectWithoutContext("RTO" , MakeCallback(&MyTcp::TraceRTT , this) );
}

void MyTcp::StopApplication(void)
{
	m_running = false;

	if(m_sendEvent.IsRunning())
	{
		Simulator::Cancel(m_sendEvent);
	}

	if(m_socket)
	{
		m_socket->Close();
	}
}

void MyTcp::ScheduledTx(void)
{

//	m_tcpsocket->TcpSocket()

	if( m_running)
	{
		Time tNext( Seconds(m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate() ) ));
		m_sendEvent = Simulator::Schedule( tNext , &MyTcp::SendPacket , this );
	}

}

void MyTcp::SendPacket(void)
{

	static int send_num = 0;
//	std::cout << "kkup" << std::endl;
	Ptr<Packet> packet = Create<Packet> (m_packetSize);
//	std::cout << "kkup" << std::endl;
	m_socket->Send (packet);
	std::cout << "kkup minRTO:" << m_tcpbase->GetMinRto().GetMilliSeconds()
								<< "\t" << Simulator::Now ().GetSeconds () << std::endl;
//	std::cout << "kkup" << std::endl;

	NS_LOG_DEBUG ("Sending:    "<<send_num++ << "\t" << Simulator::Now ().GetSeconds ());
//	std::cout << "Sending:    " <<++send_num << "\t" << Simulator::Now ().GetSeconds ()<<std::endl;

  	if (++m_packetsSent < m_nPackets)
	{
	    ScheduledTx ();
	}
}

/************* start.. Buffer Class for Local Retransmission **********************/

class MmCache : public AttributeValue
{
public:



  /** Default constructor. */
  MmCache (uint32_t max, int idx);
//  mcache ();

  Iterator Begin (void) const;

  Iterator End (void) const;

  Iterator Last (void) const;

  uint32_t GetN (void) const;

  uint16_t GetLen (SequenceNumber32 seq) const;

  Ptr<Packet> GetPacket(SequenceNumber32 seq);

  Iterator FindCloseTo (SequenceNumber32 seq) ;
  Iterator_Mu FindCloseTo (Time time, Time threshold) ;


  Iterator_Retrans FindCloseToRetrans (SequenceNumber32 seq) ;

  void Insert (SequenceNumber32 seq, Ptr<Packet> pkt);

//  void UpdateDown (SequenceNumber32 seq);

  void Swap (SequenceNumber32 seq, Ptr<Packet> pkt);

  void Delete (bool isFromFirst, SequenceNumber32 seq);

  void Delete (Iterator it);

  void SetAddrInt (uint32_t addr);

  void Mu (Time eventTime, uint32_t numPackets);

  double GetMu (void);

  void PrintMu (void);

  uint32_t GetAddrInt (void);

  void SetAddr (Ipv4Address addr);

  Ipv4Address GetAddr (void);

  void Retransmission (SequenceNumber32 seq);

  void SetRetrans(SequenceNumber32 seq, bool retrans);

  bool GetRetrans(SequenceNumber32 seq);

  virtual Ptr<AttributeValue> Copy (void) const;

  virtual std::string SerializeToString (Ptr<const AttributeChecker> checker) const;

  virtual bool DeserializeFromString (std::string value, Ptr<const AttributeChecker> checker);


  std::map< SequenceNumber32, Ptr<Packet> > m_buffer;
  std::map< SequenceNumber32, bool > m_onRetrans;
  std::map< Time, int > m_muTable;

  uint32_t maxRi;
  uint32_t curRi;
  uint32_t minRi;
  uint32_t numUnsent;
  uint8_t isUsing;
  uint8_t index;
  SequenceNumber32 seqFirstUnsent;
  uint32_t m_addrInt;
  Ipv4Address m_addr;
  Ipv4Address m_server_addr;
  uint32_t howmany_retrans;

  double m_delay;

  uint16_t preAckAdWin;
  Ipv4Header preIpv4Header;
  TcpHeader preTcpHeader;
  TcpHeader lastTcpHeader;
  uint8_t preTcpFlag;
  Ipv4Header gtpIpv4Header;
  GtpuHeader gtpHeader;
  UdpHeader gtpUdpHeader;
  SequenceNumber32 lastEAAck;
  SequenceNumber32 lastWinUpdateAck;

  Ptr<Ipv4Route> lastIpv4Route;
  Ipv4Header lastIpv4Header;
  Ipv4Header lastGtpIpv4Header;

  Ipv4Header dataGtpIpv4Header;
  GtpuHeader dataGtpHeader;
  UdpHeader dataGtpUdpHeader;

  bool isFirstAck;
  bool wasLowWin;
  bool isCongestion;

};

/************* end.. Buffer Class for Local Retransmission **********************/

/************* start.. Alayer buffer implementation **********************/

MmCache::MmCache(uint32_t initial, int idx)
{
	curRi = initial;
	minRi = initial;
	maxRi = maxCachesSize;
	m_addrInt = 0;
	m_addr = "1.1.1.1";
	howmany_retrans=numOfBatch;

	preAckAdWin = 50000;
	lastEAAck = SequenceNumber32(0);
	lastWinUpdateAck = SequenceNumber32(0);
	isFirstAck = true;
	wasLowWin = false ;
	isCongestion == false;
	numUnsent = 0;
	seqFirstUnsent = SequenceNumber32(0);
	isUsing = 1;
	index = idx;

}

//mcache::mcache()
//{
////do nothing
//}

void MmCache::SetAddrInt (uint32_t addr)
{
	m_addrInt = addr;
	return;
}

uint32_t MmCache::GetAddrInt (void)
{
	return m_addrInt;
}

void MmCache::SetAddr (Ipv4Address addr)
{
	m_addr = addr;
	return;
}

Ipv4Address MmCache::GetAddr (void)
{
	return m_addr;
}

Iterator MmCache::Begin (void) const
{
	return m_buffer.begin();
}

Iterator MmCache::End(void) const
{
	return m_buffer.end();
}

Iterator MmCache::Last(void) const
{
	return --m_buffer.end();
}

uint32_t MmCache::GetN (void) const
{
	return m_buffer.size();
}



void MmCache::SetRetrans(SequenceNumber32 seq, bool retrans)
{


	Iterator_Retrans it = m_onRetrans.find(seq);
	if ( it != m_onRetrans.end() )
	{
		m_onRetrans.find(seq)->second = retrans;
	}else
	{
		std::cout << "system error(SetRetrans), no such sequence number in a retrans map" << std::endl;
	}

	return;

}

bool MmCache::GetRetrans(SequenceNumber32 seq)
{
	Iterator_Retrans it = m_onRetrans.find(seq);
	if ( it != m_onRetrans.end() )
	{
		return m_onRetrans.find(seq)->second;
	}else
	{
		std::cout << "system error(GetRetrans), no such sequence number in a retrans map" << std::endl;
	}

	return false;
}

// packet length for seq#
uint16_t MmCache::GetLen(SequenceNumber32 seq) const
{
	Iterator it = m_buffer.find(seq);
	Ptr<Packet> pkt;
	uint16_t len = 0;
	if ( it != m_buffer.end() )
	{
		pkt = m_buffer.find(seq)->second;

	}
	else
	{
		if(seq == m_buffer.end()->first)
		{
			pkt = m_buffer.end()->second;
		}
		else
		{
			return 0;
		}

	}
	len =  pkt->GetSize()-92; //headers ;
//	len =  pkt->GetSerializedSize() - 32; //tcp header ;

//	std::cout << " getserialized() size : " << pkt->GetSerializedSize() << std::endl;
//	std::cout << " getsize() size : " << pkt->GetSize() << std::endl;

	return len;

}


//return the packet for seq#
Ptr<Packet> MmCache::GetPacket(SequenceNumber32 seq)
{
	Iterator it = m_buffer.find(seq);
	Ptr<Packet> pkt ;
	if ( it != m_buffer.end() )
	{
		pkt = m_buffer.find(seq)->second;

	}
	return pkt->Copy();
}



//return the closed iterator for seq#
Iterator MmCache::FindCloseTo (SequenceNumber32 seq)
{
	Iterator it ;
	for ( it = m_buffer.begin() ; it!= m_buffer.end() ; it++)
	{
		if(it->first == seq)
		{
			return (it);
		}
		if(it->first > seq)
		{
			return (--it);
		}
		if(it == m_buffer.end() )
		{
			return (--it);
		}
	}
	return it;
}



Iterator_Mu MmCache::FindCloseTo (Time time, Time threshold)
{
	Iterator_Mu it ;
	int64_t timeThresh = time.GetNanoSeconds()-threshold.GetNanoSeconds() > 0 ? time.GetNanoSeconds()-threshold.GetNanoSeconds() :0;

	for ( it = m_muTable.begin() ; it!= m_muTable.end() ; it++)
	{
		if(it->first.GetNanoSeconds() >= timeThresh)
		{
			break;
		}

		if(it == m_muTable.end() )
		{
			return (it);
		}
	}

	if(it == m_muTable.begin())
	{
		return it;
	}
	else
	{

		m_muTable.erase( m_muTable.begin(), it._M_const_cast() );
		return m_muTable.begin();
	}

}

void MmCache::Mu (Time eventTime, uint32_t numPackets)
{
	Iterator_Mu it = m_muTable.find(eventTime);
	if ( it == m_muTable.end() )
	{
		m_muTable.insert( std::pair<Time, int > (eventTime, numPackets) );
	}
	else
	{
		uint16_t temp = it->second;
		m_muTable.erase( it._M_const_cast() );
		m_muTable.insert( std::pair<Time, int > (eventTime, numPackets+temp) );
	}


	return;

}


void MmCache::PrintMu (void)
{

	if ( FindCloseTo (Simulator::Now(), muThresh) == m_muTable.end() )
	{
		std::cout << "Mu,"
				  << " ID:"    << this->GetAddr()
				  << " mu:" << 0
				  << " T:"     << Simulator::Now().GetSeconds()
				  << std::endl;
	}

	uint32_t total_packets = 0;
	double mu=0;

	Iterator_Mu it ;

	for ( it = m_muTable.begin() ; it!= m_muTable.end() ; it++)
	{
		total_packets += it->second ;

	}

	if( Simulator::Now().GetNanoSeconds() < muThresh.GetNanoSeconds())
	{
		mu = total_packets * packetSize * 8 / Simulator::Now().GetSeconds();
	}
	else
	{
		mu = total_packets * packetSize * 8 / muThresh.GetSeconds();
	}

	std::cout << "Mu,"
			  << " ID:"    << this->GetAddr()
			  << " mu:"    << mu
			  << " totalpkt:" << total_packets
			  << " T_i:"   << m_muTable.begin()->first.GetSeconds()
			  << " T:"     << Simulator::Now().GetSeconds()
			  << std::endl;


}

double MmCache::GetMu (void)
{

	if ( FindCloseTo (Simulator::Now(), muThresh) == m_muTable.end() )
	{
		return 0;
	}

	uint32_t total_packets = 0;
	double mu=0;

	Iterator_Mu it ;

	for ( it = m_muTable.begin() ; it!= m_muTable.end() ; it++)
	{
		total_packets += it->second ;

	}

	if( Simulator::Now().GetNanoSeconds() < muThresh.GetNanoSeconds())
	{
		mu = total_packets * packetSize * 8 / Simulator::Now().GetSeconds();
	}
	else
	{
		mu = total_packets * packetSize * 8 / muThresh.GetSeconds();
	}


	return (mu);



}

Iterator_Retrans MmCache::FindCloseToRetrans (SequenceNumber32 seq)
{
	Iterator_Retrans it ;

	for ( it = m_onRetrans.begin() ; it!= m_onRetrans.end() ; it++)
	{
		if(it->first == seq)
		{
			return (it);
		}
		if(it->first > seq)
		{
			return (--it);
		}
		if(it == m_onRetrans.end() )
		{
			return (--it);
		}
	}
	return it;
}


void MmCache::Delete (bool isFromFirst, SequenceNumber32 seq)
{

//	std::cout << "deleting...." << std::endl;
//	std::cout << "the sequence number is " << seq << std::endl;

	Iterator it = m_buffer.find(seq);
	Iterator_Retrans it_retrans = m_onRetrans.find(seq);
	uint32_t num_deleted = 0;
	uint32_t num_before = 0;
	uint32_t num_after = 0;

	if( isFromFirst )
	{

		if( seq == SequenceNumber32(0) )
		{
			num_before = m_buffer.size();
			m_buffer.erase( m_buffer.begin(), --m_buffer.end() );
			num_after = m_buffer.size();
			num_deleted = num_before-num_after;
			Mu( Simulator::Now(), num_deleted);

			m_onRetrans.erase( m_onRetrans.begin(), --m_onRetrans.end() );
			return;
		}

		if(it == m_buffer.end() )
		{

			it = FindCloseTo(seq);
			it_retrans = FindCloseToRetrans(seq);

			num_before = m_buffer.size();

			if(it != m_buffer.end() )
			{
				Delete(it);
				m_onRetrans.erase( m_onRetrans.begin(), it_retrans._M_const_cast() );
			}
			num_after = m_buffer.size();

			num_deleted = num_before-num_after;
			Mu( Simulator::Now(), num_deleted);
		}
		else
		{

			num_before = m_buffer.size();
			Delete(it);
			num_after = m_buffer.size();
			num_deleted = num_before-num_after;
			Mu( Simulator::Now(), num_deleted);

			m_onRetrans.erase( m_onRetrans.begin(), it_retrans._M_const_cast() );
		}

		return;

	}else
	{
		std::cout << "It is deleting for one packet" << std::endl;

		if(it == m_buffer.end() )
		{
			std::cout << "No data for specific Ack info in mcache... for a single packet" << std::endl;
		}
		else
		{
			Mu( Simulator::Now(), 1 );
			m_buffer.erase( it._M_const_cast()  );
		}

		m_onRetrans.erase(it_retrans._M_const_cast());


		return;

	}


}


// delete consecutive packets from begin to specific packets
void MmCache::Delete (Iterator it)
{
	m_buffer.erase( m_buffer.begin(), it._M_const_cast() );
	return;
}


void MmCache::Retransmission (SequenceNumber32 seq)
{

}

void MmCache::Insert(SequenceNumber32 seq, Ptr<Packet> pkt)
{




	if(m_buffer.size() >= maxRi )
	{

		std::cout << "BufferOverFlow, during inserting at an mcache: " << this->GetAddr()
				                                           << " seq:"<< seq << " T:" << Simulator::Now().GetSeconds() <<std::endl;
//		return;
	}

	if(m_buffer.size() >= curRi )
	{

		std::cout << "CacheIsFull, during inserting at an mcache: " << this->GetAddr()
				                                           << " seq:"<< seq <<" T:" << Simulator::Now().GetSeconds() <<std::endl;
//		return;
	}


//	std::cout << "m_buffer.insert" << Simulator::Now().GetSeconds() <<std::endl;
	m_buffer.insert( std::pair<SequenceNumber32, Ptr<Packet> > (seq, pkt) );
//	std::cout << "m_buffer.insert complete" << Simulator::Now().GetSeconds() <<std::endl;
	m_onRetrans.insert( std::pair<SequenceNumber32, bool>(seq, false));

	std::cout << "Insert,"
			  << " ID:"    << this->GetAddr()
			  << " maxRi:" << maxRi
			  << " curSi:" << this->GetN()
			  << " curRi:" << curRi
			  << " Seq:"   << seq
			  << " T:"     << Simulator::Now().GetSeconds()
			  << std::endl;

	PrintMu();
	return;
}

//void MmCache::UpdateDown(SequenceNumber32 seq)
//{
//
//	switch(cachePP){
//
//
//	case nlosFirst:
//	{
//
//		if ( (curRi * dAlpha) > GetN() )
//		{
//
//			curRi -=(seq - m_buffer.begin()->first)/segmentSize;
//
//		}
//
//
//		if (curRi < minRi)
//		{
//			curRi = minRi;
//		}
//
//
//		break;
//	}
//
//	case evenPriority:
//	{
//		break;
//	}
//
//	case losFirst:
//	{
//
//		if ( (curRi * dBeta) < GetN() )
//		{
//
//			curRi -=(seq - m_buffer.begin()->first)/segmentSize;
//
//		}
//
//
//		if (curRi < minRi)
//		{
//			curRi = minRi;
//		}
//
//
//		break;
//	}
//
//	case proposed0330:
//	{
//
//		break;
//	}
//
//	case cacheOptimal:
//	{
//		break;
//	}
//
//	default :
//	{}
//
//	}
//
//
//	std::cout << "update Down,"
//			  << " ID:" << GetAddr()
//			  << " maxRi:" << maxRi
//			  << " curSi:" << GetN()
//			  << " curRi:" << curRi
//			  << " T:"<< Simulator::Now().GetSeconds()
//			  << std::endl;
//
//}


void MmCache::Swap (SequenceNumber32 seq, Ptr<Packet> pkt)
{

	Iterator it = m_buffer.find(seq);
	if(it != m_buffer.end() )
	{
		// not used function

	}
}

Ptr<AttributeValue> MmCache::Copy (void) const
{
	return Create<MmCache>(*this);
}

std::string MmCache::SerializeToString (Ptr<const AttributeChecker> checker) const
{
  NS_LOG_FUNCTION (this << checker);
  std::ostringstream oss;
  Iterator it;
  for (it = Begin (); it != End (); ++it)
    {
      oss << (*it).second;
      if (it != End ())
        {
          oss << " ";
        }
    }
  return oss.str ();
}

bool MmCache::DeserializeFromString (std::string value, Ptr<const AttributeChecker> checker)
{
  NS_LOG_FUNCTION (this << value << checker);
  NS_FATAL_ERROR ("cannot deserialize a set of object pointers.");
  return true;
}




// MmCacheContainer

typedef std::map< uint32_t , Ptr<MmCache> >::const_iterator Iterator_Cache;

class MmCacheContainer : public AttributeValue
{
public:



  /** Default constructor. */
  MmCacheContainer (uint32_t numCaches);
  MmCacheContainer ();
  ~MmCacheContainer ();

  Iterator_Cache Begin (void) const;

  Iterator_Cache End (void) const;

  Iterator_Cache Last (void) const;

  uint32_t GetN (void) const;

  uint32_t GetCN (void) const;

  Ptr<MmCache> GetCache(uint32_t addr);

  void UpdateUp(Ptr<MmCache> mcache);

  void UpdateDown(Ptr<MmCache> mcache);

  bool DoesExist(uint32_t addr);

  bool IsCongestion(void);

  bool NeedFreeUp(void);

//  Iterator_Cache FindCloseTo (uint32_t addr) ;

  void Insert (uint32_t addr, Ptr<MmCache> cache);

  void Swap (uint32_t addr, Ptr<MmCache> cache);

  void Delete (uint32_t addr);

  void Delete (Iterator_Cache it);

  virtual Ptr<AttributeValue> Copy (void) const;

  virtual std::string SerializeToString (Ptr<const AttributeChecker> checker) const;

  virtual bool DeserializeFromString (std::string value, Ptr<const AttributeChecker> checker);


  std::map< uint32_t , Ptr<MmCache> > m_mmCaches;

  uint32_t numCaches;


};



MmCacheContainer::MmCacheContainer(uint32_t numMax)
{
	numCaches = numMax;
}

MmCacheContainer::MmCacheContainer()
{
//do nothing
}


MmCacheContainer::~MmCacheContainer()
{

//	int index=0;
//	for (Iterator_Cache it = m_mmCaches.begin() ; it != m_mmCaches.end() ; it++)
//	{
//		sim_result[index].userID = it->second->GetAddr();
//		sim_result[index].curRi = it->second->curRi;
//		sim_result[index].curSi = it->second->GetN();
//		sim_result[index].firstSeq = it->second->Begin()->first.GetValue();
//	    sim_result[index].maxRi = it->second->maxRi;
//	    index++;
//	}



}

Iterator_Cache MmCacheContainer::Begin (void) const
{
	return m_mmCaches.begin();
}

Iterator_Cache MmCacheContainer::End(void) const
{
	return m_mmCaches.end();
}

Iterator_Cache MmCacheContainer::Last(void) const
{
	return --m_mmCaches.end();
}

uint32_t MmCacheContainer::GetN (void) const
{
	return m_mmCaches.size();
}


uint32_t MmCacheContainer::GetCN (void) const
{
	uint32_t totalmmCache = 0;

	for (Iterator_Cache it = m_mmCaches.begin(); it != m_mmCaches.end() ; it++)
	{
		totalmmCache += it->second->isUsing ;
	}

	return totalmmCache;
}


void MmCacheContainer::UpdateUp(Ptr<MmCache> mcache)
{


	uint32_t sumRi = 0;
	uint32_t target_Ri = 0;
	double usage = (double)mcache->GetN()/mcache->curRi;

	for (Iterator_Cache it = m_mmCaches.begin(); it != m_mmCaches.end() ; it++)
	{
		sumRi += it->second->curRi ;
	}

	if( sumRi == maxCachesSize)
	{
		std::cout << "sum-mmcache is full...." << sumRi;

		for (Iterator_Cache it = m_mmCaches.begin(); it != m_mmCaches.end() ; it++)
		{

			std::cout << " ID:" << it->second->GetAddr()
					  << " curRi:" << it->second->curRi;
		}

		std::cout << " T:"<< Simulator::Now().GetSeconds();
		std::cout << std::endl;
		return;
	}


	if (sumRi >= maxCachesSize*needFreeUpRatio)
	{
		std::cout << "MMcaches is almost full, sumRi:" << sumRi << std::endl;
		return;
	}


	switch(cachePP){


	case nlosFirst: // NLOS first
	{


		if (  mcache->GetN() > mcache->curRi * uAlpha )
		{

			if(sumRi - (mcache->curRi) + ( mcache->GetN() * (1/uAlpha)) <= maxCachesSize)
			{
				sumRi += (mcache->GetN() * (1/uAlpha)) - (mcache->curRi);
				mcache->curRi = mcache->GetN() * (1/uAlpha);
			}else
			{
				mcache->curRi += maxCachesSize - sumRi;
				sumRi = maxCachesSize;
			}


		}


		break;
	}


	case evenPriority:  // even Priority
	{

		target_Ri = maxCachesSize*needFreeUpRatio/ GetCN();
		if( mcache->curRi < target_Ri)
		{
			mcache->curRi++;
		}
		return;
	}


	case losFirst:  // LOS first
	{


		if ( mcache->GetN()  < (mcache->curRi * uBeta )  )
		{

			if( maxCachesSize - sumRi ) // if maxCachesSize is greater than sumRi,
			{
				mcache->curRi++;
			}else
			{
				break;
			}


		}


		break;
	}


	case proposed0330:
	{

		Iterator_Cache it_best = m_mmCaches.end();
		double delta_best = 0;

		for (Iterator_Cache it = m_mmCaches.begin(); it != m_mmCaches.end() ; it++)
		{
			double expectedDeltaTP = (it->second->curRi - it->second->GetN() + it->second->numUnsent)/ (it->second->m_delay*2) ;
			if( it->second->GetMu()/8/segmentSize*muEstinAlpha  >= expectedDeltaTP )
			{
				expectedDeltaTP = 1/(it->second->m_delay*2);
				if(expectedDeltaTP > delta_best)
				{
					it_best = it;
					delta_best = expectedDeltaTP;
				}
			}
		}


		if( it_best == m_mmCaches.end())
		{
			std::cout << "none to increase at MmCacheContainer::UpdateUp()"
					  << std::endl;
			break;
		}
		else
		{
			it_best->second->curRi++;
			break;
		}


		break;
	}

	case cacheOptimal:
	{




		if( usage > 0.5)
		{
			target_Ri = ppsTP[mcache->index]*(mcache->m_delay*2) + (nonLosDuration)*(0.3)*ppsTP[mcache->index];
		}
		else
		{
			target_Ri = ppsTP[mcache->index]*(mcache->m_delay*2) + outerRTT*ppsTP[mcache->index];

		}

		if( mcache->curRi >= target_Ri)
		{
			break;
		}
		else
		{
			if( maxCachesSize*needFreeUpRatio >= sumRi +(target_Ri - mcache->curRi) )
			{
				mcache->curRi = target_Ri;
				sumRi += (target_Ri - mcache->curRi);
			}
			else
			{
				mcache->curRi +=  maxCachesSize*needFreeUpRatio - sumRi;
				sumRi += maxCachesSize*needFreeUpRatio - sumRi;
				break;
			}
		}


		break;
	}

	}

	std::cout << "update Up,"
			  << " ID:" << mcache->GetAddr()
			  << " maxRi:" << mcache->maxRi
			  << " curSi:" << mcache->GetN()
			  << " curRi:" << mcache->curRi
			  << " targetRi:" << target_Ri
			  << " usage:" << usage
			  << " sumRi:" << sumRi
			  << " T:"<< Simulator::Now().GetSeconds()
			  << std::endl;

	globalSumRi = sumRi;
}



bool MmCacheContainer::NeedFreeUp(void)
{
	uint32_t sumRi = 0;

	for (Iterator_Cache it = m_mmCaches.begin(); it != m_mmCaches.end() ; it++)
	{
		sumRi += it->second->curRi ;
	}

	if( sumRi >= maxCachesSize*needFreeUpRatio)
	{
		return true;
	}
	else
	{
		return false;

	}
}



bool MmCacheContainer::IsCongestion(void)
{

	uint32_t totalSent = 0;

	for (Iterator_Cache it = m_mmCaches.begin(); it != m_mmCaches.end() ; it++)
	{


		totalSent += it->second->GetN() - it->second->numUnsent ;

	}

	if( totalSent  >= numRlcBuf * congestionRatio )
	{
		return true;
	}

	return false;
}


void MmCacheContainer::UpdateDown(Ptr<MmCache> mcache)
{
	uint32_t sumRi = 0;
	uint32_t target_Ri = 0;

	if( mcache->curRi <= mcache->minRi)
	{
		return;
	}


	for (Iterator_Cache it = m_mmCaches.begin(); it != m_mmCaches.end() ; it++)
	{
		if( it->second->isUsing > 0)
		{
			sumRi += it->second->curRi ;
		}

	}



	switch(cachePP){


	case nlosFirst: // NLOS first
	{



		break;
	}


	case evenPriority:  // even Priority
	{
		target_Ri = maxCachesSize*needFreeUpRatio/ GetCN();
		if( mcache->curRi > target_Ri)
		{
			mcache->curRi--;
			mcache->curRi--;
			sumRi--;
			sumRi--;
		}
		return;
	}


	case losFirst:  // LOS first
	{

		break;
	}


	case proposed0330:
	{

		double expectedDeltaTP = (mcache->curRi - mcache->GetN() + mcache->numUnsent)/ (mcache->m_delay*2) ;
		if( mcache->GetMu()/8/segmentSize*muEstinAlpha  < expectedDeltaTP )
		{
			mcache->curRi --;
			mcache->curRi --;

			sumRi--;
			sumRi--;
		}


		break;
	}

	case cacheOptimal:
	{


		uint32_t target_Ri = 0;
		double usage = mcache->GetN()/mcache->curRi;
		if( usage > 0.5)
		{
			target_Ri = ppsTP[mcache->index]*(mcache->m_delay*2) + (nonLosDuration/5)*ppsTP[mcache->index];
		}
		else
		{
			target_Ri = ppsTP[mcache->index]*(mcache->m_delay*2) + outerRTT*ppsTP[mcache->index];
		}

		if( mcache->curRi < target_Ri)
		{
			break;
		}
		else
		{
			if( maxCachesSize*needFreeUpRatio >= sumRi +(target_Ri - mcache->curRi) )
			{
				mcache->curRi = target_Ri;
				sumRi =+(target_Ri - mcache->curRi);
			}
			else
			{
				mcache->curRi =  sumRi + target_Ri - maxCachesSize*needFreeUpRatio;
				break;
			}
		}


		break;
	}

	}



	std::cout << "update Down,"
			  << " ID:" << mcache->GetAddr()
			  << " maxRi:" << mcache->maxRi
			  << " curSi:" << mcache->GetN()
			  << " curRi:" << mcache->curRi
			  << " sumRi:" << sumRi
			  << " T:"<< Simulator::Now().GetSeconds()
			  << std::endl;

	globalSumRi = sumRi;

}



Ptr<MmCache> MmCacheContainer::GetCache(uint32_t addr)
{
	Iterator_Cache it = m_mmCaches.find(addr);
	Ptr<MmCache> cache ;
	if ( it != m_mmCaches.end() )
	{
		cache = m_mmCaches.find(addr)->second;

	}
	return cache;
}

bool MmCacheContainer::DoesExist(uint32_t addr)
{
	Iterator_Cache it = m_mmCaches.find(addr);
	Ptr<MmCache> cache ;
	if ( it != m_mmCaches.end() )
	{
		return true;

	}
	return false;
}


void MmCacheContainer::Delete (uint32_t addr)
{

	std::cout << "deleting.... mCache..." << std::endl;
	Iterator_Cache it = m_mmCaches.find(addr);
	Delete(it);
	return;
}

void MmCacheContainer::Delete (Iterator_Cache it)
{
	m_mmCaches.erase( it._M_const_cast() );
	return;
}



void MmCacheContainer::Insert(uint32_t addr, Ptr<MmCache> cache)
{

	std::cout << "m_mmCaches-insert, addri :" << addr <<std::endl;

	if(m_mmCaches.size() >= maxCacheNum )
	{
		std::cout << "too many caches... no more cache object.." << Simulator::Now() <<std::endl;
		return;
	}
	m_mmCaches.insert( std::pair< uint32_t, Ptr<MmCache> > (addr, cache) );

	std::cout << "m_mmCaches-insert-complete"  << std::endl;

	return;
}


Ptr<AttributeValue> MmCacheContainer::Copy (void) const
{
	return Create<MmCacheContainer>(*this);
}

std::string MmCacheContainer::SerializeToString (Ptr<const AttributeChecker> checker) const
{
  NS_LOG_FUNCTION (this << checker);
  std::ostringstream oss;
  Iterator_Cache it;
  for (it = Begin (); it != End (); ++it)
    {
      oss << (*it).second;
      if (it != End ())
        {
          oss << " ";
        }
    }
  return oss.str ();
}
bool MmCacheContainer::DeserializeFromString (std::string value, Ptr<const AttributeChecker> checker)
{
  NS_LOG_FUNCTION (this << value << checker);
  NS_FATAL_ERROR ("cannot deserialize a set of object pointers.");
  return true;
}


MmCacheContainer caches(maxCachesSize);


/************* end.. Alayer buffer implementation **********************/



//bool IsCongestion (void)
//{
//	caches.
//}




bool IsLos (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice)
{


	Ptr<MobilityModel> ue_mm = ueDevice->GetNode()->GetObject<MobilityModel>();
	Ptr<MobilityModel> enb_mm = enbDevice->GetNode()->GetObject<MobilityModel>();

	bool los = true;
	for (BuildingList::Iterator bit = BuildingList::Begin (); bit != BuildingList::End (); ++bit)
	{
		Box boundaries = (*bit)->GetBoundaries ();
		Vector locationA = ue_mm->GetPosition ();
		Vector locationB = enb_mm->GetPosition ();
		Angles pathAngles (locationB, locationA);
		double angle = pathAngles.phi;
		if (angle >= M_PI/2 || angle < -M_PI/2)
		{
			locationA = enb_mm->GetPosition ();
			locationB = ue_mm->GetPosition ();
			Angles pathAngles (locationB, locationA);
			angle = pathAngles.phi;
		}

		if (angle >=0 && angle < M_PI/2 )
		{
			Vector loc1(boundaries.xMax,boundaries.yMin,boundaries.zMin);
			Vector loc2(boundaries.xMin,boundaries.yMax,boundaries.zMin);
			Angles angles1 (loc1,locationA);
			Angles angles2 (loc2,locationA);
			if (angle > angles1.phi && angle < angles2.phi && locationB.x > boundaries.xMin && locationB.y > boundaries.yMin)
			{
				los = false;
				break;
			}
		}
		else if (angle >= -M_PI/2 && angle < 0)
		{
			Vector loc1(boundaries.xMin,boundaries.yMin,boundaries.zMin);
			Vector loc2(boundaries.xMax,boundaries.yMax,boundaries.zMin);
			Angles angles1 (loc1,locationA);
			Angles angles2 (loc2,locationA);
			if (angle > angles1.phi && angle < angles2.phi && locationB.x > boundaries.xMin && locationB.y < boundaries.yMax)
			{
				los = false;
				break;
			}
		}
	}

	return los;

}




static void CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd )
{
	*stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldCwnd << "\t" << newCwnd << std::endl ;
}





void SendEarlyAck1( uint16_t ipLen, Ptr<Packet> dataPacket, Ptr<Ipv4> ipv4, uint32_t interface,Ptr<MmCache> mcache)
{

	std::cout << "SendEarlyAck1"  << std::endl;

	Ipv4Header ipv4HeaderEA; // Early Ack header corresponding to received data
	ipv4HeaderEA = mcache->preIpv4Header;
	ipv4HeaderEA.SetDscp( Ipv4Header::DSCP_AF13 );
	uint32_t tempWindow = 0;





	uint16_t tcpTotalSize = ipLen;


	mcache->gtpIpv4Header.EnableChecksum();

	Ptr<Ipv4Route> ipv4RouteEA = Create<Ipv4Route>();
	ipv4RouteEA->SetDestination( mcache->gtpIpv4Header.GetDestination() );
	ipv4RouteEA->SetGateway( mcache->gtpIpv4Header.GetDestination() );
	ipv4RouteEA->SetOutputDevice( ipv4->GetNetDevice(interface)); // send back to the interface where it comes from
	ipv4RouteEA->SetSource( mcache->gtpIpv4Header.GetSource() );

	TcpHeader originTcpHeader;
	dataPacket->PeekHeader(originTcpHeader);
	dataPacket->RemoveHeader(originTcpHeader);
	TcpHeader tcpHeaderEA;

	tcpHeaderEA = mcache->preTcpHeader;

	SequenceNumber32 dataSeqNum = originTcpHeader.GetSequenceNumber();
	uint32_t tcpHeaderSize = originTcpHeader.GetLength()*4;
	uint16_t tcpdatasize = tcpTotalSize - tcpHeaderSize; //for calculate ack num for data

	if( mcache->lastEAAck < dataSeqNum + tcpdatasize)
	{
		tcpHeaderEA.SetAckNumber(dataSeqNum + tcpdatasize);
	}
	else
	{
		return;
//		tcpHeaderEA.SetAckNumber( mcache->lastEAAck );

	}


//	tempWindow =  ( mcache->curRi - mcache->GetN() ) > mcache->numUnsent ? ((mcache->curRi - mcache->GetN()- mcache->numUnsent) *segmentSize):0 ;

	tempWindow =  ( mcache->curRi > mcache->GetN() ) ? ((mcache->curRi - mcache->GetN()) *segmentSize):0 ;
	tempWindow = (uint32_t) ( tempWindow/winfOrg) > 3 ? (tempWindow/winfOrg)-3 : 0 ; // make it conservative for 3..

	tempWindow = (tempWindow > 65535) ? 65535:tempWindow;



	tcpHeaderEA.SetWindowSize( tempWindow ); //
	tcpHeaderEA.EnableChecksums();

	Ptr<Packet> newpacket = Create<Packet>(0);
	newpacket->AddHeader(tcpHeaderEA);
	newpacket->AddHeader(ipv4HeaderEA);
	newpacket->AddHeader(mcache->gtpHeader);
	newpacket->AddHeader(mcache->gtpUdpHeader);


	mcache->lastTcpHeader = tcpHeaderEA;
	mcache->lastIpv4Route = ipv4RouteEA;
	mcache->lastIpv4Header = ipv4HeaderEA;

	mcache->lastEAAck = tcpHeaderEA.GetAckNumber();
	ipv4->SendWithHeader(newpacket, mcache->gtpIpv4Header, ipv4RouteEA);


	std::cout << "SendEarlyAck1_e: "<<  mcache->lastEAAck  << " ID:"<< mcache->GetAddr()
			                                               << " lastEAAck:"<< mcache->lastEAAck << std::endl;

}


void SendEarlyAck2( uint16_t ipLen, Ptr<Packet> dataPacket, Ptr<Ipv4> ipv4, uint32_t interface,Ptr<MmCache> mcache)
{

	std::cout << "SendEarlyAck2"  << std::endl;
	// for cache congestion control

	Ipv4Header ipv4HeaderEA; // Early Ack header corresponding to received data
	ipv4HeaderEA = mcache->lastIpv4Header;
	ipv4HeaderEA.SetDscp( Ipv4Header::DSCP_AF12 );
	uint32_t tempWindow = 0;



	Ptr<Ipv4Route> ipv4RouteEA = Create<Ipv4Route>();
	ipv4RouteEA->SetDestination( mcache->preIpv4Header.GetDestination() );
	ipv4RouteEA->SetGateway( mcache->preIpv4Header.GetDestination() );
	ipv4RouteEA->SetOutputDevice( ipv4->GetNetDevice(interface)); // send back to the interface where it comes from
	ipv4RouteEA->SetSource( mcache->preIpv4Header.GetSource() );

	Ipv4Header originIpv4Header;
	TcpHeader originTcpHeader;

	dataPacket->PeekHeader(originIpv4Header);
	dataPacket->RemoveHeader(originIpv4Header);
	dataPacket->PeekHeader(originTcpHeader);
	dataPacket->RemoveHeader(originTcpHeader);


	TcpHeader tcpHeaderEA = mcache->lastTcpHeader;

	SequenceNumber32 dataSeqNum = originTcpHeader.GetSequenceNumber();
	uint32_t tcpHeaderSize = originTcpHeader.GetLength()*4;
	uint16_t tcpdatasize = ipLen - tcpHeaderSize; //for calculate ack num for data



	if( mcache->lastEAAck < dataSeqNum + tcpdatasize)
	{
		tcpHeaderEA.SetAckNumber(dataSeqNum + tcpdatasize);
	}
	else
	{
		return;
//		tcpHeaderEA.SetAckNumber( mcache->lastEAAck );
	}

//	tempWindow =  ( mcache->curRi - mcache->GetN() ) > mcache->numUnsent ? ((mcache->curRi - mcache->GetN()- mcache->numUnsent) *segmentSize):0 ;


	tempWindow =  ( mcache->curRi > mcache->GetN() ) ? ((mcache->curRi - mcache->GetN()) *segmentSize):0 ;
	tempWindow = (uint32_t) ( tempWindow/winfOrg) > 3 ? (tempWindow/winfOrg)-3 : 0 ; // make it conservative for 3..
	tempWindow = (tempWindow > 65535) ? 65535:tempWindow;


	tcpHeaderEA.SetWindowSize( tempWindow );
	tcpHeaderEA.EnableChecksums();







	Ptr<Packet> newpacket = Create<Packet>(0);
	newpacket->AddHeader(tcpHeaderEA);
//	newpacket->AddHeader(ipv4HeaderEA);
//	newpacket->AddHeader(mcache->gtpHeader);
//	newpacket->AddHeader(mcache->gtpUdpHeader);


	mcache->lastTcpHeader = tcpHeaderEA;
//	mcache->lastIpv4Route = ipv4RouteEA;
	mcache->lastIpv4Header = ipv4HeaderEA;

	mcache->lastEAAck = tcpHeaderEA.GetAckNumber();
	ipv4->SendWithHeader(newpacket, ipv4HeaderEA, ipv4RouteEA);
	std::cout << "SendEarlyAck2_e: " << " ID:"<< mcache->GetAddr() << " lastEAAck:"<< mcache->lastEAAck << std::endl;

}



void SendWinUpdate( Ptr<Ipv4> ipv4, uint32_t interface , Ptr<MmCache> mcache, unsigned int win, SequenceNumber32 seq)
{


	std::cout << " SEND WINDOW UPDATE PROCEDURE!   ID:"  << mcache->GetAddr() << std::endl;


	if( interface < 0 )
	{
		std::cout << " SEND WINDOW UPDATE PROCEDURE!   but interface < 0, and just return w/o SWU, at mcache:"  << mcache->GetAddr() << std::endl;
		return;
	}
	TcpHeader tcpHeader = mcache->lastTcpHeader;
	Ipv4Header ipv4Header = mcache->lastIpv4Header;
	Ptr<Ipv4Route> ipv4Route = mcache->lastIpv4Route;
	uint32_t tempWindow = 0;
	tempWindow =  ( mcache->curRi - mcache->GetN())>0 ? ((mcache->curRi - mcache->GetN()) *segmentSize):0 ;
	tempWindow = (uint32_t) ( (tempWindow)/winfOrg) ;
	tempWindow = (tempWindow > win/segmentSize ) ? (tempWindow-win/segmentSize):0;
	tempWindow = (tempWindow > 65535) ? 65535:tempWindow;

	tcpHeader.SetWindowSize( tempWindow );
	tcpHeader.SetAckNumber( mcache->lastEAAck) ;


	Ptr<Packet> newpacket = Create<Packet>(0);
	newpacket->AddHeader(tcpHeader);
	ipv4Header.SetDscp(Ipv4Header::DSCP_AF23);
	newpacket->AddHeader(ipv4Header);
	newpacket->AddHeader(mcache->gtpHeader);
	newpacket->AddHeader(mcache->gtpUdpHeader);


	std::cout << " SEND WINDOW UPDATE PROCEDURE!!   ID:"  << mcache->GetAddr() << std::endl;

	ipv4->SendWithHeader(newpacket, mcache->gtpIpv4Header, ipv4Route);


	std::cout << " SEND WINDOW UPDATE PROCEDURE!!!   ID:"  << mcache->GetAddr() << std::endl;

}





void Retransmission(SequenceNumber32 targetSeq,  Ptr<Ipv4> ipv4, uint32_t interface, Ptr<MmCache> mcache, uint32_t reRetrans)
{  // for advanced mhtcp


	Time RTO = mmRTO ;
	for (int index=0; index<reRetrans; index++)
	{
		RTO = 2* RTO;
	}


	if ( mcache->Begin()->first > targetSeq)
	{

		return;
	}


	Ptr<Ipv4Route> ipv4RouteRetrans = Create<Ipv4Route>();
	ipv4RouteRetrans->SetDestination( mcache->GetAddr() );
//	ipv4RouteRetrans->SetOutputDevice( ipv4->GetNetDevice(interface));
	ipv4RouteRetrans->SetOutputDevice( enb_node->GetDevice(1) ); // toward LTE netdevice

//	ipv4RouteRetrans->SetSource( Ipv4Address("1.0.0.1") );
	ipv4RouteRetrans->SetSource( mcache->m_server_addr );

	Ptr<Packet> newpacket;

	if( mcache->GetLen(targetSeq)==0) // the packet does't exist in the mmcache.
	{

		return;

	}

	newpacket= mcache->GetPacket(targetSeq);
	std::cout << "retransmission for seq, " << targetSeq
			  << " ID: " << mcache->GetAddr()
			  << " RTO:" << RTO
			  << " T:" << Simulator::Now().GetSeconds()
			  << std::endl;

	Ipv4Header gtpIpv4Header;
	newpacket->PeekHeader(gtpIpv4Header);
	newpacket->RemoveHeader(gtpIpv4Header);


	UdpHeader gtpUdpHeader;
	newpacket->PeekHeader(gtpUdpHeader);
	newpacket->RemoveHeader(gtpUdpHeader);

	GtpuHeader gtpHeader;
	newpacket->PeekHeader(gtpHeader);
	newpacket->RemoveHeader(gtpHeader);


	Ipv4Header ipv4Header;
	newpacket->PeekHeader(ipv4Header);
	newpacket->RemoveHeader(ipv4Header);
	ipv4Header.SetDscp(Ipv4Header::DSCP_AF33);


//	TcpHeader tcpHeader;
//	newpacket->PeekHeader(tcpHeader);
//	newpacket->RemoveHeader(tcpHeader);
//
//	newpacket->AddHeader(tcpHeader);

	newpacket->AddHeader(ipv4Header);
	newpacket->AddHeader(gtpHeader);
	newpacket->AddHeader(gtpUdpHeader);

	ipv4->SendWithHeader(newpacket, gtpIpv4Header, ipv4RouteRetrans);
//	mcache->SetRetrans( targetSeq, true );


	RTO = (RTO>mmMaxTimer)? mmMaxTimer:RTO;
	Simulator::Schedule( RTO ,&Retransmission,targetSeq,  ipv4, interface, mcache, reRetrans+1);


}




void AdditionalTransmission0(Ptr<Ipv4> ipv4, uint32_t interface, Ptr<MmCache> mcache )
{


	double delayParam = ((double) (rand()%100) /100 + 0.0000001)*2; // 0~2
	if( mcache->numUnsent == 0 )
	{

//		ATO = (ATO>mmMaxTimer)? mmMaxTimer:ATO;

//		std::cout << "rand().. : " << delayParam << std::endl;
//		std::cout << "delay time.. : " << mmiATP*(rand()+0.000001)*2 << std::endl;
		Simulator::Schedule( mmiATP*delayParam ,&AdditionalTransmission0, ipv4, interface, mcache);



		return;
	}


	std::cout << "AdditionalTransmission0, mcache->seqFirstUnsent:" << mcache->seqFirstUnsent << std::endl;
	std::cout << "                         mcache->Begin()->first:" << mcache->Begin()->first << std::endl;
	std::cout << "                         mcache->Last()->first:" << mcache->Last()->first << std::endl;
	std::cout << "                         mcache->mcache->curRi:" << mcache->curRi << std::endl;
	std::cout << "                         mcache->GetN():" << mcache->GetN() << std::endl;
	std::cout << "                         mcache->numUnsent:" << mcache->numUnsent << std::endl;
	std::cout << "                         mcache->AddrInt():" << mcache->m_addr
		      << " T:" <<  Simulator::Now().GetSeconds()
			  << std::endl;

	Ptr<Ipv4Route> ipv4RouteRetrans = Create<Ipv4Route>();
	ipv4RouteRetrans->SetDestination( mcache->GetAddr() );
	ipv4RouteRetrans->SetOutputDevice( enb_node->GetDevice(1) ); // toward LTE netdevice
	ipv4RouteRetrans->SetSource( mcache->m_server_addr );


	uint32_t numDelPkt = mcache->numUnsent;
	numDelPkt = (numDelPkt>maxAddiTrans) ? maxAddiTrans:numDelPkt;

	for (int index=0 ; index < numDelPkt ; index++)
	{
		std::cout << "additional transmission 0 for seq, " << mcache->seqFirstUnsent
					  << "  ID: " << mcache->GetAddr()
					  << std::endl;

		if( mcache->seqFirstUnsent > mcache->Last()->first || mcache->seqFirstUnsent < mcache->Begin()->first )
		{

			Simulator::Schedule( mmiATP*delayParam ,&AdditionalTransmission0, ipv4, interface, mcache);


			return;
		}

		Ptr<Packet> newpacket = mcache->GetPacket(mcache->seqFirstUnsent);

		Ipv4Header gtpIpv4Header;
		newpacket->PeekHeader(gtpIpv4Header);
		newpacket->RemoveHeader(gtpIpv4Header);


		UdpHeader gtpUdpHeader;
		newpacket->PeekHeader(gtpUdpHeader);
		newpacket->RemoveHeader(gtpUdpHeader);

		GtpuHeader gtpHeader;
		newpacket->PeekHeader(gtpHeader);
		newpacket->RemoveHeader(gtpHeader);


		Ipv4Header ipv4Header;
		newpacket->PeekHeader(ipv4Header);
		newpacket->RemoveHeader(ipv4Header);
		ipv4Header.SetDscp(Ipv4Header::DSCP_AF41);

		newpacket->AddHeader(ipv4Header);
		newpacket->AddHeader(gtpHeader);
		newpacket->AddHeader(gtpUdpHeader);

		ipv4->SendWithHeader(newpacket, gtpIpv4Header, ipv4RouteRetrans);
//			Simulator::Schedule( mmRTO ,&Retransmission,targetSeq,  ipv4, interface, mcache);

		mcache->numUnsent --;//123123
		mcache->seqFirstUnsent +=segmentSize;

		if( mcache->numUnsent == 0)
		{

			if( mcache->isCongestion == true)
			{
				mcache->isCongestion = false;
				std::cout << "0 - Congestion off for mcache:" << mcache->GetAddr()
						  << " T:" << Simulator::Now().GetSeconds()
						  << std::endl ;
			}

			mcache->seqFirstUnsent = SequenceNumber32(0);
//			ATO = (ATO>mmMaxTimer)? mmMaxTimer:ATO;
			Simulator::Schedule( mmiATP*delayParam ,&AdditionalTransmission0, ipv4, interface, mcache);
//			if ( 2*ATO > mmMaxTimer)
//			{
//				ATO = mmMaxTimer;
//			}
//			else
//			{
//				ATO = 2*ATO;
//			}
			return;
		}


		if( caches.IsCongestion() )
		{

			Simulator::Schedule( mmiATP*delayParam ,&AdditionalTransmission0, ipv4, interface, mcache);

			return;
		}



	}



	Simulator::Schedule( mmiATP*delayParam ,&AdditionalTransmission0, ipv4, interface, mcache);

	return;
}





void AdditionalTransmission1(Ptr<Ipv4> ipv4, uint32_t interface, Ptr<MmCache> mcache, int numDelPkt)
{
	if( numDelPkt <= 0)
	{
		std::cout << "no packet for additionalTransmission, numDelPkt:" << numDelPkt << std::endl;
		return;
	}

//	std::cout << "Additional transmission for seq:" << targetSeq
//			  << " ID:" << mcache->GetAddr()
//			  << " T:"  << Simulator::Now().GetSeconds()
//			  << std::endl;

	std::cout << "AdditionalTransmission1, numDelPkt:" << numDelPkt << std::endl;
	std::cout << "						  mcache->seqFirstUnsent:" << mcache->seqFirstUnsent << std::endl;
	std::cout << "						  mcache->Begin()->first:" << mcache->Begin()->first << std::endl;
	std::cout << "						  mcache->Last()->first:" << mcache->Last()->first << std::endl;
	std::cout << "						  mcache->mcache->curRi:" << mcache->curRi << std::endl;
	std::cout << "						  mcache->GetN():" << mcache->GetN() << std::endl;
	std::cout << "						  mcache->numUnsent:" << mcache->numUnsent << std::endl;
	std::cout << "						  mcache->m_addr:" << mcache->m_addr
			  << " T:" <<  Simulator::Now().GetSeconds()
		      << std::endl;

	Ptr<Ipv4Route> ipv4RouteRetrans = Create<Ipv4Route>();
	ipv4RouteRetrans->SetDestination( mcache->GetAddr() );
	ipv4RouteRetrans->SetOutputDevice( enb_node->GetDevice(1) ); // toward LTE netdevice
	ipv4RouteRetrans->SetSource( mcache->m_server_addr );





	for (int index=0 ; index < numDelPkt ; index++)
	{
		std::cout << "additional transmission 1 for seq, " << mcache->seqFirstUnsent
					  << "  ID: " << mcache->GetAddr()
					  << std::endl;

		Ptr<Packet> newpacket = mcache->GetPacket(mcache->seqFirstUnsent);

		Ipv4Header gtpIpv4Header;
		newpacket->PeekHeader(gtpIpv4Header);
		newpacket->RemoveHeader(gtpIpv4Header);


		UdpHeader gtpUdpHeader;
		newpacket->PeekHeader(gtpUdpHeader);
		newpacket->RemoveHeader(gtpUdpHeader);

		GtpuHeader gtpHeader;
		newpacket->PeekHeader(gtpHeader);
		newpacket->RemoveHeader(gtpHeader);


		Ipv4Header ipv4Header;
		newpacket->PeekHeader(ipv4Header);
		newpacket->RemoveHeader(ipv4Header);
		ipv4Header.SetDscp(Ipv4Header::DSCP_AF42);

		newpacket->AddHeader(ipv4Header);
		newpacket->AddHeader(gtpHeader);
		newpacket->AddHeader(gtpUdpHeader);

		ipv4->SendWithHeader(newpacket, gtpIpv4Header, ipv4RouteRetrans);
//			Simulator::Schedule( mmRTO ,&Retransmission,targetSeq,  ipv4, interface, mcache);

		mcache->numUnsent --; //123123
		if( mcache->numUnsent == 0)
		{

			if( mcache->isCongestion == true)
			{
				mcache->isCongestion = false;
				std::cout << "Congestion off for mcache:" << mcache->GetAddr()
						  << " T:" << Simulator::Now().GetSeconds()
						  << std::endl ;
			}

			mcache->seqFirstUnsent = SequenceNumber32(0);
			return;
		}
		mcache->seqFirstUnsent +=segmentSize;


	}





	return;
}





void AdditionalTransmission2(Ptr<Ipv4> ipv4, uint32_t interface, Ptr<MmCache> mcache, uint32_t reRetrans)
{

	Time ATP = mmATP;
	for (int index=0; index<reRetrans; index++)
	{
		ATP = 2* ATP;
	}


	// for AdditionalSendWhileCongestion Congestion Control algorithm.
	if( mcache->numUnsent == 0)
	{
		return;
	}




	Ptr<Ipv4Route> ipv4RouteRetrans = Create<Ipv4Route>();
	ipv4RouteRetrans->SetDestination( mcache->GetAddr() );
	ipv4RouteRetrans->SetOutputDevice( enb_node->GetDevice(1) ); // toward LTE netdevice
	ipv4RouteRetrans->SetSource( mcache->m_server_addr );


	uint32_t numDelPkt = mcache->numUnsent;
	numDelPkt = (numDelPkt > maxAddiTrans) ? maxAddiTrans:numDelPkt;

	std::cout << "AdditionalTransmission2, mcache->seqFirstUnsent:" << mcache->seqFirstUnsent << std::endl;
	std::cout << "                         mcache->Begin()->first:" << mcache->Begin()->first << std::endl;
	std::cout << "                         mcache->Last()->first:" << mcache->Last()->first << std::endl;
	std::cout << "                         mcache->mcache->curRi:" << mcache->curRi << std::endl;
	std::cout << "                         mcache->GetN():" << mcache->GetN() << std::endl;
	std::cout << "                         mcache->numUnsent:" << mcache->numUnsent << std::endl;
	std::cout << "                         mcache->m_addr:" << mcache->m_addr<< std::endl;
	std::cout << "                         numDelPkt:" << numDelPkt
		      << " T:" <<  Simulator::Now().GetSeconds()
			  << std::endl;


	for (int index=0 ; index < numDelPkt ; index++)
	{
		std::cout << "additional transmission 2 for seq, " << mcache->seqFirstUnsent
					  << "  ID: " << mcache->GetAddr()
					  << std::endl;

		Ptr<Packet> newpacket = mcache->GetPacket(mcache->seqFirstUnsent);

		Ipv4Header gtpIpv4Header;
		newpacket->PeekHeader(gtpIpv4Header);
		newpacket->RemoveHeader(gtpIpv4Header);


		UdpHeader gtpUdpHeader;
		newpacket->PeekHeader(gtpUdpHeader);
		newpacket->RemoveHeader(gtpUdpHeader);

		GtpuHeader gtpHeader;
		newpacket->PeekHeader(gtpHeader);
		newpacket->RemoveHeader(gtpHeader);


		Ipv4Header ipv4Header;
		newpacket->PeekHeader(ipv4Header);
		newpacket->RemoveHeader(ipv4Header);
		ipv4Header.SetDscp(Ipv4Header::DSCP_AF43);

		newpacket->AddHeader(ipv4Header);
		newpacket->AddHeader(gtpHeader);
		newpacket->AddHeader(gtpUdpHeader);

		ipv4->SendWithHeader(newpacket, gtpIpv4Header, ipv4RouteRetrans);
//			Simulator::Schedule( mmRTO ,&Retransmission,targetSeq,  ipv4, interface, mcache);

		mcache->numUnsent --; //123123
		if( mcache->numUnsent == 0)
		{

			if( mcache->isCongestion == true)
			{
				mcache->isCongestion = false;
				std::cout << "Congestion off for mcache:" << mcache->GetAddr()
						  << " T:" << Simulator::Now().GetSeconds()
						  << std::endl ;
			}

			mcache->seqFirstUnsent = SequenceNumber32(0);
			return;
		}
		mcache->seqFirstUnsent +=segmentSize;

		if( caches.IsCongestion() )
		{
			ATP = (ATP>mmMaxTimer)? mmMaxTimer:ATP;
			Simulator::Schedule( ATP ,&AdditionalTransmission2, ipv4, interface, mcache, reRetrans+1);

			return;
		}



	}





	return;
}



void BatchRetransmission(SequenceNumber32 targetSeq,  Ptr<Ipv4> ipv4, uint32_t interface, Ptr<MmCache> mcache)
{  // for advanced mhtcp



	if(targetSeq != mcache->Begin()->first)
	{
		return;
	}

	std::cout << "batch retransmission for seq:" << targetSeq
			  << " ID:" << mcache->GetAddr()
			  << " T:"  << Simulator::Now().GetSeconds()
			  << std::endl;

//	Ptr<Ipv4Route> ipv4RouteRetrans = Create<Ipv4Route>();
//	ipv4RouteRetrans->SetDestination( mcache->GetAddr() );
//	ipv4RouteRetrans->SetOutputDevice( ipv4->GetNetDevice(interface));
//	ipv4RouteRetrans->SetSource( Ipv4Address("1.0.0.1") );

	Ptr<Packet> newpacket;


	uint16_t howmanyRetrans = (mcache->howmany_retrans < mcache->GetN())? mcache->howmany_retrans:mcache->GetN();

	for (uint16_t index = 0 ; index < howmanyRetrans; index++)
	{


		if( mcache->GetLen(targetSeq)==0) // the packet does't exist in the mmcache.
		{

			break;

		}

		if( mcache->seqFirstUnsent > SequenceNumber32(0) and targetSeq >= mcache->seqFirstUnsent)
		{
			break;
		}

		newpacket= mcache->GetPacket(targetSeq);

		if(mcache->GetRetrans(targetSeq))
		{
			uint16_t tcpSize = mcache->GetLen(targetSeq);
			targetSeq += tcpSize;
			continue;

		}else
		{
			mcache->SetRetrans(targetSeq, true);
			Retransmission(targetSeq, ipv4, interface, mcache, 0 );
			uint16_t tcpSize = mcache->GetLen(targetSeq);
			targetSeq += tcpSize;
		}


	}


}





void WriteOccupancy(Ptr<MmCache> mcache, Ptr<OutputStreamWrapper> outStream_occ )
{

//	std::cout << "ccoomm1-1" << std::endl;
	*outStream_occ->GetStream() << Simulator::Now().GetMilliSeconds()
			                    << "\t\t" << mcache->GetN()
								<< "\t\t" << mcache->curRi
								<< "\t\t" << mcache->GetN()*100/maxCachesSize
								<< "\t\t" << mcache->Begin()->first;
//								<< "\t\t" << mcache->Last()->first
//								<< std::endl ;
	if( mcache->GetN() > 0 and mcache->isUsing > 0)
	{
		*outStream_occ->GetStream() << " A.GP-FS:" << ( (uint64_t) mcache->Begin()->first.GetValue() * 8 / (Simulator::Now().GetSeconds()-pump_start) /1000000 );
	}
	else if(mcache->GetN() > 0 and mcache->isUsing == 0)
	{
		*outStream_occ->GetStream() << " A.GP-FS:" << ( (uint64_t) mcache->Begin()->first.GetValue() * 8 / (Simulator::Now().GetSeconds()-extraUeAppear.GetSeconds()) /1000000 );

	}
	else{
		*outStream_occ->GetStream() << " A.GP-LA:" << ( (uint64_t) mcache->lastEAAck.GetValue() * 8 / (Simulator::Now().GetSeconds()-pump_start) /1000000 );
	}

	*outStream_occ->GetStream() << std::endl;
//	std::cout << "ccoomm1-2" << std::endl;

	Simulator::Schedule( mmPrintPeriod , &WriteOccupancy,mcache, outStream_occ);

}



//void WriteLastAck(Ipv4Address srcIP, Ptr<OutputStreamWrapper> outStream_occ )
//{
//
////	std::cout << "ccoomm1-1" << std::endl;
//	*outStream_occ->GetStream() << Simulator::Now().GetMilliSeconds()
//			                    << "\t\t" << srcIP
//								<< "\t\t" << mcache->curRi
//								<< "\t\t" << mcache->GetN()*100/maxCachesSize
//								<< "\t\t" << mcache->Begin()->first;
////								<< "\t\t" << mcache->Last()->first
////								<< std::endl ;
//	if( mcache->GetN() > 0 and mcache->isUsing > 0)
//	{
//		*outStream_occ->GetStream() << " A.GP-FS:" << ( (uint64_t) mcache->Begin()->first.GetValue() * 8 / (Simulator::Now().GetSeconds()-pump_start) /1000000 );
//	}
//	else if(mcache->GetN() > 0 and mcache->isUsing == 0)
//	{
//		*outStream_occ->GetStream() << " A.GP-FS:" << ( (uint64_t) mcache->Begin()->first.GetValue() * 8 / (Simulator::Now().GetSeconds()-extraUeAppear.GetSeconds()) /1000000 );
//
//	}
//	else{
//		*outStream_occ->GetStream() << " A.GP-LA:" << ( (uint64_t) mcache->lastEAAck.GetValue() * 8 / (Simulator::Now().GetSeconds()-pump_start) /1000000 );
//	}
//
//	*outStream_occ->GetStream() << std::endl;
////	std::cout << "ccoomm1-2" << std::endl;
//
//	Simulator::Schedule( mmPrintPeriod , &WriteOccupancy,mcache, outStream_occ);
//
//}





uint16_t NeedRetransAdv ( SequenceNumber32 seq, Ptr<MmCache> mcache)
{
	bool isNeedRetrans = false;
	static SequenceNumber32 prevSeq(0);
	static unsigned int howManyAck = 0;

	if(mcache->GetRetrans(seq)) // if already it's on retransmission
		return false;


	if( prevSeq == seq)
	{
		howManyAck++;
	}else
	{
		prevSeq = seq;
		howManyAck = 0;

	}

	if ( howManyAck >= retransNTh)
	{
		isNeedRetrans = true;

	}


	return isNeedRetrans;

}


bool IsNewAck ( SequenceNumber32 seq)
{

	static SequenceNumber32 prevSeq(0);

	if( prevSeq == seq)
	{
		return false;

	}else
	{

		if (seq > prevSeq)
		{
			return true;
		}

	}

	return false;

}

SequenceNumber32 LastAck ( SequenceNumber32 seq)
{

	static SequenceNumber32 prevSeq(0);
	SequenceNumber32 tempSeq(0);

	if( prevSeq == seq)
	{
		return seq;

	}else
	{

		if (seq > prevSeq)
		{
			tempSeq = prevSeq;
			prevSeq = seq;

			return tempSeq;
		}

	}

	return prevSeq;

}

SequenceNumber32 LastWinUpdatedAck ( SequenceNumber32 seq)
{

	static SequenceNumber32 prevSeq(0);
	SequenceNumber32 tempSeq(0);

	if( prevSeq == seq)
	{
		return seq;

	}else
	{

		if (seq > prevSeq)
		{
			tempSeq = prevSeq;
			prevSeq = seq;

			return tempSeq;
		}

	}

	return prevSeq;

}





void RxPacketTrace( Ptr<const Packet> pkt, Ptr<Ipv4> ipv4, uint32_t interface )
{


//	std::cout << "Entering RxPacketTrace, interface:" << interface << std::endl ;

	if (!enableEA)
	{
		return;
	}


	Ptr<MmCache> mcache;

	Ptr<Packet> packet = pkt->Copy();
	Ptr<Packet> originPacket;

	uint16_t tempAckAdWin = 50000;
	Ipv4Header tempIpv4Header;
	uint32_t dstAddri = 0;
	uint32_t srcAddri = 0;
	uint16_t ipLen = 0;
	uint16_t tcpSize = 0;
	TcpHeader tempTcpHeader;
	uint8_t tempTcpFlag;


	Ipv4Header tempGtpIpv4Header;
	GtpuHeader tempGtpHeader;
	UdpHeader tempGtpUdpHeader;


	SequenceNumber32 lastAck = SequenceNumber32(0);


	packet->PeekHeader(tempGtpIpv4Header);
	packet->RemoveHeader(tempGtpIpv4Header);

	packet->PeekHeader(tempGtpUdpHeader);
	packet->RemoveHeader(tempGtpUdpHeader);

	packet->PeekHeader(tempGtpHeader);
	packet->RemoveHeader(tempGtpHeader);

	packet->PeekHeader(tempIpv4Header);
	packet->RemoveHeader(tempIpv4Header);

	packet->PeekHeader(tempTcpHeader);
	tempAckAdWin = tempTcpHeader.GetWindowSize();
	tempTcpFlag = tempTcpHeader.GetFlags();

	srcAddri = tempIpv4Header.GetSource().Get();
	dstAddri = tempIpv4Header.GetDestination().Get();








	if( caches.DoesExist(srcAddri))
	{
//		std::cout << "EA mcache srcAddri : "<< srcAddri << "   interface:"<<interface << std::endl ;
		mcache = caches.GetCache(srcAddri);

	}
	else if (caches.DoesExist(dstAddri))
	{
//		std::cout << "EA mcache dstAddri : "<< dstAddri << "   interface:"<<interface << std::endl ;
		mcache = caches.GetCache(dstAddri);

	}
	else
	{
		std::cout << "non-mmTCP user at eNB_Rx, srcAddri:"<< srcAddri<< " dstAddri:" << dstAddri << std::endl ;
		return;
	}




	/*determine whether earlyack or retransmitted packet*/
	if( tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF13
			|| tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF23
			|| tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF33
			|| tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF43)
	{
		// do nothing for earlyack.. or local retransmission

		return;
	}


	if( mcache->isUsing == 0)
	{
		std::cout << "mcache " << mcache->GetAddr() << "is not using" << std::endl;
		return;
	}



	if( tempTcpFlag == TcpHeader::SYN ||  //if TCP signaling...
		 tempTcpFlag == TcpHeader::RST ||
		 tempTcpFlag == TcpHeader::FIN ||
		 tempTcpFlag == TcpHeader::SYN + TcpHeader::ACK ||
		 tempTcpFlag == TcpHeader::RST + TcpHeader::ACK ||
		 tempTcpFlag == TcpHeader::FIN + TcpHeader::ACK )
	{

		tempGtpHeader.SetLength(1456);
		mcache->dataGtpHeader = tempGtpHeader;
		tempGtpUdpHeader.ForcePayloadSize(1472);
		mcache->dataGtpUdpHeader = tempGtpUdpHeader;
		tempGtpIpv4Header.SetPayloadSize(1492);
		mcache->dataGtpIpv4Header = tempGtpIpv4Header;


		mcache->isFirstAck = true;

		return;
	}
	else
	{


		originPacket = pkt->Copy();
		ipLen = tempIpv4Header.GetPayloadSize();
		tcpSize = ipLen-tempTcpHeader.GetLength()*4;

		if( tcpSize == 0 )
		{
			return;
		}

		mcache->Insert( tempTcpHeader.GetSequenceNumber(), originPacket );
		caches.UpdateUp(mcache);


		if( !mcache->isFirstAck )
		{




			/*modi_0627*/
//			mcache->lastEAAck = tempTcpHeader.GetSequenceNumber() + tcpSize;
			/*modi_0627*/

			SendEarlyAck1( ipLen, packet , ipv4, interface ,mcache);

//			std::cout << "interface : " << interface << "  ip len in enb:" << originPacket->GetSize()<< std::endl;



		}

	}



//	std::cout << "Leaving RxPacketTrace, interface:" << interface << std::endl ;
}





void TxPacketTrace( Ptr<const Packet> pkt, Ptr<Ipv4> ipv4, uint32_t interface )
{

//	std::cout << "Entering TxPacketTrace, interface:" << interface << std::endl ;

	if (!enableEA)
	{
		return;
	}


	Ptr<MmCache> mcache;

	Ptr<Packet> packet = pkt->Copy();
	Ptr<Packet> originPacket;

	uint16_t tempAckAdWin = 50000;
	Ipv4Header tempIpv4Header;
	uint32_t dstAddri = 0;
	uint32_t srcAddri = 0;
	uint16_t ipLen = 0;
	uint16_t tcpSize = 0;
	TcpHeader tempTcpHeader;
	uint8_t tempTcpFlag;


	Ipv4Header tempGtpIpv4Header;
	GtpuHeader tempGtpHeader;
	UdpHeader tempGtpUdpHeader;


	SequenceNumber32 lastAck = SequenceNumber32(0);


	packet->PeekHeader(tempGtpIpv4Header);
	packet->RemoveHeader(tempGtpIpv4Header);

	packet->PeekHeader(tempGtpUdpHeader);
	packet->RemoveHeader(tempGtpUdpHeader);

	packet->PeekHeader(tempGtpHeader);
	packet->RemoveHeader(tempGtpHeader);

	packet->PeekHeader(tempIpv4Header);
	packet->RemoveHeader(tempIpv4Header);

	srcAddri = tempIpv4Header.GetSource().Get();
	dstAddri = tempIpv4Header.GetDestination().Get();



	if( caches.DoesExist(srcAddri))
	{
//		std::cout << "EA mcache srcAddri : "<< srcAddri << "   interface:"<<interface << std::endl ;
		mcache = caches.GetCache(srcAddri);

	}
	else if (caches.DoesExist(dstAddri))
	{
//		std::cout << "EA mcache dstAddri : "<< dstAddri << "   interface:"<<interface << std::endl ;
		mcache = caches.GetCache(dstAddri);

	}
	else
	{
		std::cout << "non-mmTCP user at eNB_Tx, srcAddri:"<< srcAddri<< " dstAddri:" << dstAddri << std::endl ;
		return;
	}








	if( mcache->isUsing == 0)
	{
		std::cout << "mcache " << mcache->GetAddr() << "is not using" << std::endl;
		return;
	}


	packet->PeekHeader(tempTcpHeader);
	tempAckAdWin = tempTcpHeader.GetWindowSize();
	tempTcpFlag = tempTcpHeader.GetFlags();

//	uint16_t targetRwin = tempTcpHeader.GetWindowSize();

	/*determine whether earlyack or retransmitted packet*/
	if( tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF13
			|| tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF23
			|| tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF33
			|| tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF43)
	{
		// do nothing for earlyack.. or local retransmission

		return;
	}




	if ( tempGtpIpv4Header.GetDestination().IsEqual( Ipv4Address("10.0.0.6")) ) //if ack packet...
	{


		if( tempTcpFlag == TcpHeader::SYN ||  // if it is signaling of TCP handshake...
			 tempTcpFlag == TcpHeader::SYN + TcpHeader::ACK ||
			 tempTcpFlag == TcpHeader::RST + TcpHeader::ACK )
		{

			winfOrg = tempTcpHeader.GetOption(TcpOption::WINSCALE)->GetObject<TcpOptionWinScale>()->GetScale();
			winfOrg = 2 << (winfOrg-1);
			std::cout << "original winscale : " << winfOrg << std::endl;



			mcache->isFirstAck = true;

			AdditionalTransmission0( ipv4, interface, mcache );
			return;
		}
		else if( tempTcpFlag == TcpHeader::RST ||  // if TCP session doesn't last anymore...
				 tempTcpFlag == TcpHeader::FIN ||
				 tempTcpFlag == TcpHeader::FIN + TcpHeader::ACK )
		{

			mcache->isUsing = 0;    // clear mcache
			mcache->Delete(true,mcache->Last()->first);
			mcache->curRi = 0;


		}
		else             // if normal Ack
		{
			mcache->preAckAdWin = tempAckAdWin;
			mcache->preIpv4Header = tempIpv4Header;
			mcache->preTcpHeader = tempTcpHeader;
			mcache->preTcpFlag = tempTcpFlag;

//			mcache->lastIpv4Header = tempIpv4Header;
//			mcache->lastTcpHeader = tempTcpHeader;
//			mcache->preTcpFlag = tempTcpFlag;

			if(mcache->isFirstAck) // if this is sample ack packet...
			{


				mcache->gtpIpv4Header = tempGtpIpv4Header;
				mcache->gtpUdpHeader = tempGtpUdpHeader;
				mcache->gtpUdpHeader.ForcePayloadSize(72);
				mcache->gtpHeader = tempGtpHeader;
			}

			mcache->isFirstAck = false;


			SequenceNumber32 targetAck = tempTcpHeader.GetAckNumber();



			std::cout << "RcvAc,"
					  << " ID:" << mcache->GetAddr()
					  << " maxRi:" << mcache->maxRi
					  << " curSi:" << mcache->GetN()
					  << " curRi:" << mcache->curRi
	                  << " Oc:"<< mcache->GetN()*100/maxCachesSize <<"%"
					  << " R-Ack:" <<  targetAck
					  << " F.Unsent:" << mcache->seqFirstUnsent
					  << " N.Unsent:" << mcache->numUnsent
					  << " Con:" << mcache->isCongestion
					  << " T:"<< Simulator::Now().GetSeconds()
					  << std::endl;


			if(targetAck < mcache->Begin()->first)
			{
				return;
			}

			if(targetAck > mcache->Last()->first)
			{
				std::cout << " clear all !!!-------------------------ID:"<< mcache->GetAddr()
						  << " T:" << Simulator::Now().GetSeconds() <<std::endl;
				std::cout << "before buffer info :"  << mcache->GetN()<< "\t" << mcache->Begin()->first << "\t" << mcache->Last()->first << std::endl;

				mcache->Delete(true, SequenceNumber32(0));
				std::cout << "after buffer info :"  << mcache->GetN()<< "\t" << mcache->Begin()->first << "\t" << mcache->Last()->first << std::endl;

				if( mcache->lastWinUpdateAck < targetAck)
				{
					mcache->lastWinUpdateAck = targetAck;
					SendWinUpdate(ipv4, interface ,mcache, segmentSize*3, targetAck );
				}

				if( mcache->isCongestion == true)
				{
					mcache->isCongestion = false;
					std::cout << "1 - Congestion off for mcache:" << mcache->GetAddr()
							  << " T:" << Simulator::Now().GetSeconds()
							  << std::endl ;
				}

				return;
			}




			if(mcache->wasLowWin)
			{
				ipLen = tempIpv4Header.GetPayloadSize();
			}





			if(mcache->GetN() < 2 )
			{
				return;
			}



			if ( isAdvancedProxy && NeedRetransAdv(targetAck, mcache))
			{

				uint16_t packetLength = mcache->GetLen(targetAck);
				if( packetLength > 0 )
				{
					std::cout << " need!!! retransmission!!!-------------------------ID:"<<mcache->GetAddr()
							  << " T:" << Simulator::Now().GetSeconds() <<std::endl;

					BatchRetransmission( targetAck,  ipv4, interface, mcache);

				}else
				{

					std::cout << " need!!! retransmission!!!-----but it's not in an Alayer buffer.. ID:"<<mcache->GetAddr()
							  << " T:" << Simulator::Now().GetSeconds() << std::endl;
					std::cout << "buffer info :"  << mcache->GetN()<< "\t" << mcache->Begin()->first << "\t" << mcache->Last()->first << std::endl;

					std::cout << "retransmissions from the first packet in a Alayer" << std::endl;
					targetAck = mcache->Begin()->first;

					BatchRetransmission( targetAck,  ipv4, interface, mcache);


				}




			}else
			{

				if( enableCC && mcache->isCongestion )
				{

					switch(cacheCP){

					case sendAsMuchAsAcked:
					{

						int numDelPkt = (tempTcpHeader.GetAckNumber().GetValue() - mcache->Begin()->first.GetValue());
						if( numDelPkt <= 0)
						{
							numDelPkt = 0;
						}
						else
						{
							numDelPkt = numDelPkt/segmentSize;
						}

						AdditionalTransmission1(ipv4, interface, mcache, numDelPkt);

						break;
					}

					case noSendWhileCongestion:
					{

						break;
					}

					case AdditionalSendWhileCongestion:
					{
						AdditionalTransmission2(ipv4, interface, mcache, 0);
						break;
					}

					}



				}

				if ( caches.NeedFreeUp() and (tempTcpHeader.GetAckNumber() > mcache->Begin()->first) )
				{

					caches.UpdateDown(mcache);

				}
//				mcache->UpdateDown(tempTcpHeader.GetAckNumber() );
				mcache->Delete( true , tempTcpHeader.GetAckNumber() );
				Simulator::Schedule( mmiRTO ,&BatchRetransmission, mcache->Begin()->first ,  ipv4, interface, mcache);

				if( mcache->Begin()->first >= mcache->seqFirstUnsent )
				{
					if(mcache->isCongestion == true)
					{
						mcache->isCongestion = false;
						std::cout << "2 - Congestion off for mcache:"
								  << " T:" << Simulator::Now().GetSeconds()
								  << mcache->GetAddr() << std::endl ;
					}



				}



			}



			lastAck = LastAck(targetAck);

		}

	}



//	std::cout << "Leaving TxPacketTrace, interface:" << interface << std::endl ;
}


void RxPacketTraceConv( Ptr<const Packet> pkt, Ptr<Ipv4> ipv4, uint32_t interface )
{




	Ipv4Header tempIpv4Header;
	TcpHeader tempTcpHeader;
	uint8_t tempTcpFlag;


	Ipv4Header tempGtpIpv4Header;
	GtpuHeader tempGtpHeader;
	UdpHeader tempGtpUdpHeader;

	static SequenceNumber32 lastEAAck;



	Ptr<Packet> packet = pkt->Copy();

	packet->PeekHeader(tempGtpIpv4Header);
	packet->RemoveHeader(tempGtpIpv4Header);

	packet->PeekHeader(tempGtpUdpHeader);
	packet->RemoveHeader(tempGtpUdpHeader);

	packet->PeekHeader(tempGtpHeader);
	packet->RemoveHeader(tempGtpHeader);

	packet->PeekHeader(tempIpv4Header);
	packet->RemoveHeader(tempIpv4Header);

	packet->PeekHeader(tempTcpHeader);
	tempTcpFlag = tempTcpHeader.GetFlags();

	uint16_t targetRwin = tempTcpHeader.GetWindowSize();



	if ( tempGtpIpv4Header.GetDestination().IsEqual( Ipv4Address("10.0.0.6")) ) //if ack packet...
	{

		if( tempTcpFlag == TcpHeader::SYN ||
			 tempTcpFlag == TcpHeader::RST ||
			 tempTcpFlag == TcpHeader::FIN ||
			 tempTcpFlag == TcpHeader::SYN + TcpHeader::ACK ||
			 tempTcpFlag == TcpHeader::RST + TcpHeader::ACK ||
			 tempTcpFlag == TcpHeader::FIN + TcpHeader::ACK )
		{
			winfOrg = tempTcpHeader.GetOption(TcpOption::WINSCALE)->GetObject<TcpOptionWinScale>()->GetScale();
			winfOrg = 2 << (winfOrg-1);
			std::cout << "winscale : " << winfOrg << std::endl;
			return;
		}
		else
		{

			SequenceNumber32 targetAck = tempTcpHeader.GetAckNumber();
//			if(targetAck > lastAck)
//			{
//				lastAck = targetAck;
//			}

			std::cout << " TraceConv:: ACK num:" << targetAck << "    IP:  " <<tempIpv4Header.GetSource() << "  checksum :" << tempTcpHeader.IsChecksumOk() << "  time:"<< Simulator::Now().GetSeconds() <<std::endl;
			std::cout << " TraceConv:: MS_Rwin:" << targetRwin*winfOrg << std::endl;

			for (int index=0; index<numUEs+numExtraUEs ; index++)
			{
				if (sim_result[index].userID.IsEqual(tempIpv4Header.GetSource())
				    && sim_result[index].lastAck < targetAck )
				{
					sim_result[index].lastAck = targetAck;
					continue;
				}
			}


		}

	}
	else // if data packet...
	{
	}




}

void RxPacketTraceGW( Ptr<const Packet> pkt, Ptr<Ipv4> ipv4, uint32_t interface )
{


	if (!enableEA)
		return;


//	ipv4->SetForwarding(1,true);
//	ipv4->SetForwarding(2,true);
//	ipv4->SetForwarding(3,true);
//	ipv4->SetForwarding(4,true);

	uint8_t baseinterface = numUEs + numExtraUEs + 2; //num of total users +2   (regular users 3, extra ue 1)


	for (uint8_t index=1; index < baseinterface ; index++)
	{
		ipv4->SetForwarding(index,true);
	}


	if(interface == baseinterface) //if it isn't interface=3 && Rx,
	{

		return;
	}

	Ptr<Packet> packet = pkt->Copy();

	Ipv4Header tempIpv4Header;
	TcpHeader tempTcpHeader;




	packet->PeekHeader(tempIpv4Header);
	packet->RemoveHeader(tempIpv4Header);

	packet->PeekHeader(tempTcpHeader);



	/*determine whether earlyack or retransmitted packet*/
	if( tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF12
			|| tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF13
			|| tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF23
			|| tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF33
			|| tempIpv4Header.GetDscp() == Ipv4Header::DSCP_AF43)
	{
		// no nothing for earlyack.. or local retransmission


		return;
	}

	if( tempTcpHeader.GetFlags() == TcpHeader::SYN ||  // if it is signaling of TCP handshake...
		tempTcpHeader.GetFlags() == TcpHeader::RST ||
		tempTcpHeader.GetFlags() == TcpHeader::FIN ||
		tempTcpHeader.GetFlags() == TcpHeader::SYN + TcpHeader::ACK ||
		tempTcpHeader.GetFlags() == TcpHeader::RST + TcpHeader::ACK ||
		tempTcpHeader.GetFlags() == TcpHeader::FIN + TcpHeader::ACK )
	{



		return;
	}






	if(tempTcpHeader.GetAckNumber().GetValue() <= 1401) //if first ack from mobile...
	{
		return;
	}






	Ptr<MmCache> mcache;
	uint32_t dstAddri = 0;
	uint32_t srcAddri = 0;

	srcAddri = tempIpv4Header.GetSource().Get();
	dstAddri = tempIpv4Header.GetDestination().Get();



	if( caches.DoesExist(srcAddri))
	{

		mcache = caches.GetCache(srcAddri);

	}
	else if (caches.DoesExist(dstAddri))
	{

		mcache = caches.GetCache(dstAddri);

	}
	else
	{
		std::cout << "non-mmTCP user at PGW_Rx, srcAddri:"<< srcAddri<< " dstAddri:" << dstAddri << std::endl ;
	}






	if(interface >= 2 && interface < baseinterface) //
	{

		if( enableCC )
		{
			if( caches.IsCongestion() )
			{

				if ( !mcache->isCongestion) // status is now changed from non-congestion -> congestion
				{
					if(mcache->seqFirstUnsent == SequenceNumber32(0) )
					{
						mcache->seqFirstUnsent = tempTcpHeader.GetSequenceNumber();
						mcache->numUnsent = 0;
					}

				}

				if(mcache->isCongestion == false )
				{
					mcache->isCongestion = true;
					std::cout << "1 - Congestion on for mcache:" << mcache->GetAddr()
							  << " T:" << Simulator::Now().GetSeconds()
							  << std::endl ;

//					switch(cacheCP){
//
//					case sendAsMuchAsAcked:
//					{
//
//
//
//						break;
//					}
//
//					case noSendWhileCongestion:
//					{
//
//						AdditionalTransmission2(ipv4, interface, mcache);
//						break;
//					}
//
//					}

				}

			}

			uint16_t ipLen = tempIpv4Header.GetPayloadSize();
			if( ipLen-tempTcpHeader.GetLength()*4 == 0 ) // no operation for empty packet(ack).
			{
//				std::cout << "test code 001" <<  std::endl;
				ipv4->SetForwarding(interface,false);
				return;
			}

			if(mcache->isCongestion)
			{
//				std::cout << "test code 002" << std::endl;
				packet->AddHeader( tempIpv4Header );


				std::cout << "sending EA2 for mcache:" << mcache->GetAddr() << std::endl;

				SendEarlyAck2( ipLen, packet->Copy() , ipv4, interface ,mcache);


				packet->AddHeader( mcache->dataGtpHeader );
				packet->AddHeader( mcache->dataGtpUdpHeader );
				packet->AddHeader( mcache->dataGtpIpv4Header );


				mcache->Insert( tempTcpHeader.GetSequenceNumber(), packet );
				mcache->numUnsent++;


				ipv4->SetForwarding(interface,false);
				return;
			}



		}


	}


	if (interface == 1)
	{
		//	std::cout << "GW received Ack num: "<< tempTcpHeader.GetAckNumber().GetValue() <<
		//			     " interface: " << interface << std::endl;

		std::cout << "test code 003" <<  std::endl;



		ipv4->SetForwarding(1,false);
//		for (uint8_t index=1; index < baseinterface ; index++)
//		{
//			ipv4->SetForwarding(index,false);
//		}


		//	std::cout << "Leaving RxPacketTraceGW, interface:" << interface << std::endl ;
	}





}


void TxPacketTraceGW( Ptr<const Packet> pkt, Ptr<Ipv4> ipv4, uint32_t interface )
{

//	std::cout << "Entering TxPacketTraceGW, interface:" << interface << std::endl ;


	if (!enableEA)
		return;

	uint8_t baseinterface = numUEs + numExtraUEs + 2; //num of total users +2   (regular users 3, extra ue 1)

	if(interface >= 2 && interface <= baseinterface) //if it is from server...
	{
//		std::cout << "interface != 3 in TxtraceGW, interface: " << interface << std::endl;
//		std::cout << "test code 005" <<  std::endl;


		return;
	}


	Ptr<MmCache> mcache;

	Ptr<Packet> packet = pkt->Copy();
	Ipv4Header tempIpv4Header;
	uint32_t dstAddri = 0;
	uint32_t srcAddri = 0;

	TcpHeader tempTcpHeader;
	uint8_t tempTcpFlag;





	SequenceNumber32 lastAck = SequenceNumber32(0);



	packet->PeekHeader(tempIpv4Header);
	packet->RemoveHeader(tempIpv4Header);
	packet->PeekHeader(tempTcpHeader);
	srcAddri = tempIpv4Header.GetSource().Get();
	dstAddri = tempIpv4Header.GetDestination().Get();


	if( caches.DoesExist(srcAddri))
	{
//		std::cout << "EA mcache srcAddri : "<< srcAddri << "   interface:"<<interface << std::endl ;
		mcache = caches.GetCache(srcAddri);

	}
	else if (caches.DoesExist(dstAddri))
	{
//		std::cout << "EA mcache dstAddri : "<< dstAddri << "   interface:"<<interface << std::endl ;
		mcache = caches.GetCache(dstAddri);

	}
	else
	{
//		std::cout << "test code 006: " <<  std::endl;
		std::cout << "non-mmTCP user at PGW_Tx, srcAddri:"<< srcAddri<< " dstAddri:" << dstAddri << std::endl ;
	}



	if(interface == 1) //if it is to eNB...
	{
//		std::cout << "test code 007: "  << std::endl;

	}



}






void SpecialEvent(void)
{
	std::cout << "**********************************************************************************************************************" << std::endl;
	//p2p_2_h->SetDeviceAttribute ("DataRate", StringValue("1Mbps") );
//	p2p_2_h.SetDeviceAttribute("DataRate", StringValue("1kbps") );

	return;
}

void SpecialEvent1( Ptr<const Packet> pkt, const Address &address)
{
//	std::cout << "*****" << std::endl;

//	static uint32_t howmany = 0;

	Ipv4Header ipv4Header;
	TcpHeader tcpHeader;


	Ptr<Packet> packet = pkt->Copy();


//	packet->PeekHeader(ipv4Header);
//	packet->RemoveHeader(ipv4Header);
//	packet->PeekHeader(tcpHeader);

//	std::cout << "sequen num : " <<tcpHeader.GetSequenceNumber() << std::endl;

//	howmany++;
//	std::cout << "how many packets arrive at the ue application.. : " << howmany << std::endl;

	return;
}



void SpecialEvent2( Ptr<NetDevice> nd, Ptr<const Packet> pkt, uint16_t protocol, const Address &address)
//void SpecialEvent2( ns3::NetDevice::ReceiveCallback(Ptr<NetDevice> nd, Ptr<const Packet> pkt, uint16_t protocol, const Address &address))
{
	std::cout << "^^^^^" << std::endl;

	static uint32_t howmany = 0;

	Ipv4Header ipv4Header;
	TcpHeader tcpHeader;


//	Ptr<Packet> packet = pkt->Copy();


//	packet->PeekHeader(ipv4Header);
//	packet->RemoveHeader(ipv4Header);
//	packet->PeekHeader(tcpHeader);

//	std::cout << "sequen num : " <<tcpHeader.GetSequenceNumber() << std::endl;

	howmany++;
	std::cout << "how many packets arrive at the ue netdevice.. : " << howmany << std::endl;

	return;
}


void CourseChange( Ptr<const MobilityModel> model)
{

	std::cout << "position changed" << std::endl;
	Vector pos = model->GetPosition ();
	Vector vel = model->GetVelocity ();
	std::cout << Simulator::Now () << ", model=" << model << ", POS: x=" << pos.x << ", y=" << pos.y
			  << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
			  << ", z=" << vel.z << std::endl;
}

void CurrentPosition(Ptr<ns3::MobilityModel> model)
{

	std::cout << "current position :"<< model->GetPosition() << "   at  "<<Simulator::Now()<< std::endl;

}





void Summary(void)
{
	std::cout <<"hello world" << std::endl;

	std::string fname_result="";
	fname_result.append(fname);
	fname_result.append("_result");
//	fname_result.append( StringValue( UintegerValue(2).).Get() );
	fname_result.append(".txt");


	std::ofstream result_file;
	result_file.open(fname_result.c_str());


	result_file << " ---------------------------------------------------------------" << std::endl ;
	result_file << " Summary" << std::endl ;
	result_file << " ---------------------------------------------------------------" << std::endl ;
	result_file << "Total number of usrs: " << numUEs+numExtraUEs << std::endl;
	result_file << "Traffic duration: " << (pump_stop-pump_start) << " in seconds" << std::endl;

	result_file << "Total number of mcaches, GetN: " << caches.GetN() << std::endl;
	result_file << "Total number of mcaches, GetCN: " << caches.GetCN() << std::endl;



	if(enableEA)
	{

		int totalSumRi=0;
		int i=0;
		for (Iterator_Cache it = caches.Begin(); it != caches.End() ; it++)
		{
			result_file << " --------------------" << std::endl ;
			result_file << "User ID: " << it->second->GetAddr() << std::endl;
			result_file << " -------------0405---" << std::endl ;
			result_file << " curRi:" << it->second->curRi;
			result_file << " maxRi:" << it->second->maxRi;
			result_file << " curSi:"   << it->second->GetN();
			if( it->second->curRi > 0)
			{
				result_file << " C.Usage:" << it->second->GetN()*100/it->second->curRi << "%";
			}

			if( it->second->GetN() > 0)
			{
				result_file << " F.Seq:" << it->second->Begin()->first;
			}
			else
			{
				result_file << " F.Seq: empty";
			}

			result_file << " F.SeqUnsent:" << it->second->seqFirstUnsent;
			result_file << " NumUnsent:" << it->second->numUnsent;

			if( it->second->GetN() > 0 and it->second->isUsing > 0)
			{
				result_file << " A.GP-FS:" << ( (uint64_t) it->second->Begin()->first.GetValue() * 8 / (pump_stop-pump_start) /1000000 );
			}
			else if(it->second->GetN() > 0 and it->second->isUsing == 0)
			{
				result_file << " A.GP-FS:" << ( (uint64_t) it->second->Begin()->first.GetValue() * 8 / (pump_stop_ext-extraUeAppear.GetSeconds()) /1000000 );

			}
			else{
				result_file << " A.GP-LA:" << ( (uint64_t) it->second->lastEAAck.GetValue() * 8 / (pump_stop-pump_start) /1000000 );
			}

			result_file << " Mbps" ;
			result_file << std::endl;
			result_file << " dataRate:" << sim_result[i].rate;
			result_file << " dataRate_doubleArray:" << dataRate_doubleArray[i%arraySize];
			result_file << " delay:"   << sim_result[i].delay;
			result_file << " delayArrayDouble:" << delayArrayDouble[i%arraySize];
			result_file << " S-Pos:" << sim_result[i].startPos;
			result_file << " Speed:" << sim_result[i].speed;
			result_file << " ppsTP: " << sim_result[i].ppstp << std::endl;
			result_file << std::endl;
			result_file << " --------------------" << std::endl ;


			totalSumRi += it->second->GetN();
			i++;
		}

		result_file << " Total Cache Usage: "<< totalSumRi*100/maxCachesSize <<"%" << std::endl ;

	}
	else
	{
		for (int index= 0 ; index < numUEs+numExtraUEs ; index++)
		{
			result_file << " --------------------" << std::endl ;
			result_file << "User ID: " << sim_result[index].userID << std::endl;
			result_file << " -------------" << std::endl ;



			result_file << "Average Goodput: " << (double) (  (uint64_t) sim_result[index].lastAck.GetValue()*8 / (pump_stop-pump_start))/1000000
				     	<< " Mbps"
					    << std::endl ;
			result_file << "dataRateArray:" << dataRateArray[index%arraySize]
					    << " dataRate_doubleArray:" << dataRate_doubleArray[index%arraySize]
						<< " delayArray:"   << delayArray[index%arraySize]
						<< " delayArrayDouble:" << delayArrayDouble[index%arraySize]
					    << std::endl;
			result_file << " --------------------" << std::endl ;
		}
	}






	result_file << " ---------------------------------------------------------------" << std::endl ;
	result_file << " Simulation Parameters" << std::endl ;
	result_file << " ---------------------------------------------------------------" << std::endl ;


	result_file << " ======================" << std::endl ;
	result_file << " Key-Params" << std::endl ;
	result_file << " ======================" << std::endl ;
	result_file << "numUEs: " << numUEs << std::endl;
	result_file << "numNlosUEs: " << numNlosUEs << std::endl;
	result_file << "numExtraUEs: " << numExtraUEs << std::endl;
	result_file << "enableSack: " << enableSack << std::endl;
	result_file << "enableEA: " << enableEA << std::endl;
	result_file << "enableHarq: " << enableHarq << std::endl;
	result_file << "isAdvancedProxy: " << isAdvancedProxy << std::endl;
	result_file << "isRlcAm: " << isRlcAm << std::endl;
	result_file << "cache policy: " << cachePP << std::endl;




	result_file << " ======================" << std::endl ;
	result_file << " Sim-topology" << std::endl ;
	result_file << " ======================" << std::endl ;
	result_file << "distance: " << distance << std::endl;
	result_file << "ueSpeed: " << ueSpeed << std::endl;
	result_file << "nonLosDuration: " << nonLosDuration << std::endl;
	result_file << "losDuration: " << losDuration << std::endl;
	result_file << "positionDiffer: " << positionDiffer << std::endl;
	result_file << "ueVelocityMo: " << ueVelocityMo << std::endl;
	result_file << "ueVelocitySt: " << ueVelocitySt << std::endl;
	result_file << "enbPosition: " << enbPosition << std::endl;
	result_file << "ueStarting1: " << ueStarting1 << std::endl;
	result_file << "ueStarting2: " << ueStarting2 << std::endl;
	result_file << "allStationary: " << allStationary << std::endl;
	result_file << "scenario: " << scenario << std::endl;
	result_file << "condition: " << condition << std::endl;
	result_file << "delayArray: " << delayArray << std::endl;
	result_file << "delayArrayDouble: " << delayArrayDouble << std::endl;

	result_file << " ======================" << std::endl ;
	result_file << " Times" << std::endl ;
	result_file << " ======================" << std::endl ;
	result_file << "mmRTO: " << mmRTO << std::endl;
	result_file << "mmiRTO: " << mmiRTO << std::endl;
	result_file << "mmiATP: " << mmiATP << std::endl;
	result_file << "mmATP: " << mmATP << std::endl;
	result_file << "mmMaxTimer: " << mmMaxTimer << std::endl;
	result_file << "mmPrintPeriod: " << mmPrintPeriod << std::endl;
	result_file << "zeroWinProbeTime: " << zeroWinProbeTime.Get() << std::endl;
	result_file << "mmPrintPeriod: " << mmPrintPeriod << std::endl;
	result_file << "simTime: " << simTime << std::endl;
	result_file << "pump_start: " << pump_start << std::endl;
	result_file << "pump_stop: " << pump_stop << std::endl;
	result_file << "sink_start: " << sink_start << std::endl;
	result_file << "sink_stop: " << sink_stop << std::endl;


	result_file << " ======================" << std::endl ;
	result_file << " Buffer Sizes" << std::endl ;
	result_file << " ======================" << std::endl ;
	result_file << "sndBufSize: " << sndBufSize << std::endl;
	result_file << "rcvBufSize: " << rcvBufSize << std::endl;
	result_file << "iCacheSize: " << iCacheSize << std::endl;
	result_file << "segmentSize: " << segmentSize << std::endl;
	result_file << "maxCacheNum: " << maxCacheNum << std::endl;
	result_file << "maxCacheLength: " << maxCacheLength << std::endl;
	result_file << "maxCachesSize: " << maxCachesSize << std::endl;
	result_file << "basicQueueSize: " << basicQueueSize << std::endl;
	result_file << "maxPackets: " << maxPackets << std::endl;
	result_file << "globalSumRi:" << globalSumRi << std::endl;
	result_file << "numRlcBuf:" << numRlcBuf << std::endl;
	result_file << "congestionRatio: " << congestionRatio << std::endl;


	result_file << " ======================" << std::endl ;
	result_file << " Retransmission" << std::endl ;
	result_file << " ======================" << std::endl ;
	result_file << "retransNTh: " << retransNTh << std::endl;


	result_file << " ======================" << std::endl ;
	result_file << " TCP" << std::endl ;
	result_file << " ======================" << std::endl ;
	result_file << "tcpProtocol: " << tcpProtocol << std::endl;
	result_file << "winfOrg: " << winfOrg << std::endl;
	result_file << "winfNew: " << winfNew << std::endl;
	result_file << "dataRateExtra: " << dataRateExtra << std::endl;


	result_file << " ======================" << std::endl ;
	result_file << " Network" << std::endl ;
	result_file << " ======================" << std::endl ;
	result_file << "bandwidthInet: " << bandwidthInet << std::endl;
	result_file << "bandwidthBsUe: " << bandwidthBsUe << std::endl;
	result_file << "mtu: " << mtu << std::endl;
	result_file << "basedelay: " << delayArray[0] << std::endl;
	result_file << "delayBsUe: " << delayBsUe << std::endl;

	result_file << " ======================" << std::endl ;
	result_file << " Policys" << std::endl ;
	result_file << " ======================" << std::endl ;
	result_file << "cache policy: " << cachePP << "  enum[nlosFirst,evenPriority,losFirst,customizedPriority" <<std::endl;
	result_file << "uAlpha: " << uAlpha << std::endl;
	result_file << "dAlpha: " << dAlpha << std::endl;
	result_file << "uBeta: " << uBeta << std::endl;
	result_file << "dBeta: " << dBeta << std::endl;
	result_file << "numUEs: " << numUEs << std::endl;



	result_file << " ======================" << std::endl ;
	result_file << " the others" << std::endl ;
	result_file << " ======================" << std::endl ;
	result_file << "fname: " << fname << std::endl;
	result_file << "packetSize: " << packetSize << std::endl;
	result_file << "nPackets: " << nPackets << std::endl;
	result_file << "currentRlcBuf: " << currentRlcBuf << std::endl;
	result_file << "enablePcap: " << enablePcap << std::endl;




	result_file << " ======================" << std::endl ;
	result_file << " building position info" << std::endl ;
	result_file << " ======================" << std::endl ;
	for (int index=0; index < numBuildings ; index++)
	{

		result_file << "buildingPosition: "<< index << "  " << buildingPosition[index] << std::endl;
	}

}


void AddExtraUe (Ptr<Ipv4L3Protocol> ue_ipv4L3, Ipv4Address serverAddress, int index)
{


  if(enableEA)
  {
	  uint32_t addr_int = ue_ipv4L3->GetAddress(1,0).GetLocal().Get();
	  std::cout << "mcache addr : " << ue_ipv4L3->GetAddress(1,0).GetLocal() << std::endl;

	  if(cachePP == evenPriority)
	  {
//		  caches.Insert( addr_int, Create<MmCache>( (int) maxCachesSize*(1-needFreeUpRatio) ) );
		  caches.Insert( addr_int, Create<MmCache>( (int) maxCachesSize/(numUEs*3), index) );
	  }
	  else if(cachePP == proposed0330)
	  {
		  caches.Insert( addr_int, Create<MmCache>( (int) maxCachesSize/(numUEs*3), index) );
	  }
	  else if(cachePP == cacheOptimal)
	  {
		  caches.Insert( addr_int, Create<MmCache>( (int) maxCachesSize/(numUEs*3), index) );
	  }
	  else
	  {
		  caches.Insert( addr_int, Create<MmCache>(iCacheSize, index) );
	  }

	  caches.GetCache(addr_int)->SetAddr(ue_ipv4L3->GetAddress(1,0).GetLocal());
	  caches.GetCache(addr_int)->SetAddrInt(addr_int);
	  caches.GetCache(addr_int)->m_server_addr = serverAddress;
	  caches.GetCache(addr_int)->m_delay = delayArrayDouble[numUEs%arraySize];


	  std::ostringstream o_occ;
	  o_occ << fname << "_" << "ex" <<"_occ.txt";
	  std::string fname_occ="";
	  fname_occ.append(o_occ.str());

	  AsciiTraceHelper asciiTrace_h;
 	  Ptr<OutputStreamWrapper> outStream_occ = asciiTrace_h.CreateFileStream(fname_occ);
//		  std::cout << "ccoomm1" << std::endl;
	  WriteOccupancy( caches.GetCache(addr_int) , outStream_occ);
//		  std::cout << "ccoomm2" << std::endl;
  }


	return;

}


/**********************  Main function  ***********************/


int
main (int argc, char *argv[])
{

  /* preparing logging file
   * file name: mhTCP7_~~~~~.txt
   * */

  LogComponentEnable("MmWaveBeamforming",LOG_INFO);
//  LogComponentEnable("MultiModelSpectrumChannel",LOG_LEVEL_ALL);



  if(!enableEA)
  {
	  sprintf(fname, "%s_%s_%s_%s_C%dk_RW%dM_d%dm_%.2fmPs_nlos%.2f_los%.2f",
			  nameBase.c_str(), "Conv", adaptiveRate?"Adap":"Cons" , enableCC?"CC":"NC" , maxCachesSize/1000 , rcvBufSize/1000000, distance, ueSpeed,nonLosDuration,losDuration);
  }else
  {
	  switch(cachePP){

	  case evenPriority:
	  {
		  sprintf(fname, "%s_%s_%s_%s_C%dk_RW%dM_d%dm_%.2fmPs_nlos%.2f_los%.2f",
				  nameBase.c_str(), "eP" , adaptiveRate?"Adap":"Cons" ,enableCC?"CC":"NC" , maxCachesSize/1000 , rcvBufSize/1000000, distance, ueSpeed, nonLosDuration, losDuration);
		  break;
	  }

	  case losFirst:
	  {
		  sprintf(fname, "%s_%s_%s_%s_C%dk_RW%dM_d%dm_%.2fmPs_nlos%.2f_los%.2f",
				  nameBase.c_str(), "lF" , adaptiveRate?"Adap":"Cons" ,enableCC?"CC":"NC" , maxCachesSize/1000 , rcvBufSize/1000000, distance, ueSpeed, nonLosDuration, losDuration);
		  break;
	  }
	  case nlosFirst:
	  {
		  sprintf(fname, "%s_%s_%s_%s_C%dk_RW%dM_d%dm_%.2fmPs_nlos%.2f_los%.2f",
				  nameBase.c_str(), "nF" , adaptiveRate?"Adap":"Cons" , enableCC?"CC":"NC" , maxCachesSize/1000 , rcvBufSize/1000000, distance, ueSpeed, nonLosDuration, losDuration);
		  break;
	  }

	  case proposed0330:
	  {
		  sprintf(fname, "%s_%s_%s_%s_C%dk_RW%dM_d%dm_%.2fmPs_nlos%.2f_los%.2f",
				  nameBase.c_str(), "PP" , adaptiveRate?"Adap":"Cons" , enableCC?"CC":"NC" , maxCachesSize/1000 , rcvBufSize/1000000, distance, ueSpeed, nonLosDuration, losDuration);
		  break;
	  }

	  case cacheOptimal:
	  {
		  sprintf(fname, "%s_%s_%s_%s_C%dk_RW%dM_d%dm_%.2fmPs_nlos%.2f",
				  nameBase.c_str(), "Opt" , adaptiveRate?"Adap":"Cons" , enableCC?"CC":"NC" , maxCachesSize/1000 , rcvBufSize/1000000, distance, ueSpeed,nonLosDuration);
		  break;
	  }
	  }
  }


  for (int index=0; index < numUEs+numExtraUEs ; index++)
  {
	  ppsTP[index] = dataRate_doubleArray[index%arraySize]/8/segmentSize;
	  sim_result[index].ppstp = ppsTP[index];
  }



	std::string fname_log="";
	std::string fname_err="";
	fname_log.append(fname);
	fname_err.append(fname);
	fname_log.append("_log.txt");
	fname_err.append("_err.txt");


	std::ofstream log_file;
	std::ofstream err_file;

  if(needLog)
  {
	log_file.open(fname_log.c_str());
	err_file.open(fname_err.c_str());


	std::streambuf* sbuf = std::cout.rdbuf();
	std::cout.rdbuf(sbuf);
	std::cout.rdbuf( log_file.rdbuf() );

	std::streambuf* ebuf = std::clog.rdbuf();
	std::clog.rdbuf(ebuf);
	std::clog.rdbuf( err_file.rdbuf() );

  }



  /*Default configuration for simulation*/
//  tcp-procotol : NewReno
  if( tcpProtocol.compare("TcpNewReno") == 0)
  {
	  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue( TcpNewReno::GetTypeId() ) );

  }
  else
  {
	  printf("TCP type error\n");
	  exit(1);
  }


  Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(rcvBufSize/ratio_rcvToRLC));

  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(sndBufSize));

  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(rcvBufSize/tcpRcvBufRatio));
  Config::SetDefault("ns3::TcpSocket::PersistTimeout", zeroWinProbeTime );
  Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(enableSack) );

  Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(BooleanValue(isRlcAm)));
  Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(BooleanValue(enableHarq)));

  Config::SetDefault ("ns3::MmWaveHelper::PathlossModel", StringValue("ns3::BuildingsObstaclePropagationLossModel"));
  Config::SetDefault ("ns3::MmWaveHelper::ChannelModel", StringValue("ns3::MmWaveBeamforming"));

//  Config::SetDefault ("ns3::MmWaveHelper::Scheduler", StringValue("ns3::MmWaveFlexTtiMacScheduler"));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(enableHarq));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::FixedMcsDl", BooleanValue(false));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::McsDefaultDl", UintegerValue(15));

  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue(enableHarq));


    Config::SetDefault ("ns3::LteRlcAm::PollRetransmitTimer", TimeValue(MilliSeconds(2.0)));
    Config::SetDefault ("ns3::LteRlcAm::ReorderingTimer", TimeValue(MilliSeconds(1.0)));
    Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(1.0)));
    Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MilliSeconds(2.0)));
  Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (rcvBufSize/ratio_rcvToRLC));
//  Config::SetDefault ("ns3::LteRlcAm::MaxRetxTh", UintegerValue (4));

  Config::SetDefault ("ns3::QueueBase::MaxPackets", UintegerValue (maxPackets));
  Config::SetDefault ("ns3::QueueBase::MaxBytes", UintegerValue (maxPackets * packetSize));

	/*
	Config::SetDefault ("ns3::LteRlcAm::PollRetransmitTimer", TimeValue(MilliSeconds(2.0)));
	Config::SetDefault ("ns3::LteRlcAm::ReorderingTimer", TimeValue(MilliSeconds(1.0)));
	Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(1.0)));
	Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MilliSeconds(2.0)));
	Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (bufferSize));
    Config::SetDefault ("ns3::Queue::MaxPackets", UintegerValue (100*1000));
	 */

	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ChannelCondition", StringValue(condition));
	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue(scenario));
	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::OptionalNlos", BooleanValue(true));
	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Shadowing", BooleanValue(true)); // enable or disable the shadowing effect

	Config::SetDefault ("ns3::MmWave3gppChannel::UpdatePeriod", TimeValue (MilliSeconds (200))); // Set channel update period, 0 stands for no update.
	Config::SetDefault ("ns3::MmWave3gppChannel::CellScan", BooleanValue(false)); // Set true to use cell scanning method, false to use the default power method.
	Config::SetDefault ("ns3::MmWave3gppChannel::Blockage", BooleanValue(true)); // use blockage or not
	Config::SetDefault ("ns3::MmWave3gppChannel::PortraitMode", BooleanValue(false)); // use blockage model with UT in portrait mode
	Config::SetDefault ("ns3::MmWave3gppChannel::NumNonselfBlocking", IntegerValue(5)); // number of non-self blocking obstacles
	Config::SetDefault ("ns3::MmWave3gppChannel::BlockerSpeed", DoubleValue(1)); // speed of non-self blocking obstacles

//	Config::SetDefault ("ns3::MmWavePhyMacCommon::NumHarqProcess", UintegerValue(100));

/*	double hBS = 0; //base station antenna height in meters;
	double hUT = 0; //user antenna height in meters;
	if(scenario.compare("RMa")==0)
	{
		hBS = 35;
		hUT = 1.5;
	}
	else if(scenario.compare("UMa")==0)
	{
		hBS = 25;
		hUT = 1.5;
	}
	else if (scenario.compare("UMi-StreetCanyon")==0)
	{
		hBS = 10;
		hUT = 1.5;
	}
	else if (scenario.compare("InH-OfficeMixed")==0 || scenario.compare("InH-OfficeOpen")==0 || scenario.compare("InH-ShoppingMall")==0)
	{
		hBS = 3;
		hUT = 1;
	}
	else
	{
		std::cout<<"Unkown scenario.\n";
		return 1;
	}*/


  /*Command line arguments*/
  CommandLine cmd;
  cmd.Parse(argc, argv);



  /*Create mmwave objects*/
  Ptr<MmWaveHelper> mmWave_h = CreateObject<MmWaveHelper>();

  mmWave_h->Initialize();
  BuildingsHelper building_h;


  Ptr<MmWavePointToPointEpcHelper> mmWaveEpc_h = CreateObject<MmWavePointToPointEpcHelper>();
  mmWave_h->SetEpcHelper(mmWaveEpc_h);

  Ptr<Node> pgw = mmWaveEpc_h->GetPgwNode();



  /*Create nodes*/

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  NodeContainer servers;

  enbNodes.Create(1);
  ueNodes.Create(numUEs);
  servers.Create(numUEs);

  /*for extra nodes*/
//  NodeContainer extraUEs;
//  NodeContainer extraServers;
//  extraUEs.Create(numExtraUEs);
//  extraServers.Create(numExtraUEs);

  Ptr<Node> extraUE = CreateObject<Node>();
  Ptr<Node> extraServer = CreateObject<Node>();


  /*install internet stack*/
  InternetStackHelper internetStack_h;

  /*for extra nodes*/
  internetStack_h.Install(servers);
  internetStack_h.Install(extraServer);

  /*Create the Internet attribute by using p2p helper*/


  PointToPointHelper p2pInet_h[numUEs+numExtraUEs];

  for( uint32_t index=0 ; index< numUEs+numExtraUEs ; index++)
  {
	  p2pInet_h[index].SetDeviceAttribute ("DataRate", StringValue(bandwidthInet[index%arraySize]) );
	  p2pInet_h[index].SetDeviceAttribute ("Mtu", UintegerValue (mtu));
	  p2pInet_h[index].SetChannelAttribute ("Delay", StringValue(delayArray[index%arraySize]) );
	  sim_result[index].delay = delayArray[index%arraySize];
  }



  NetDeviceContainer server_pgw_ndc[numUEs+numExtraUEs];
  Ipv4InterfaceContainer internetIpv4_ic[numUEs+numExtraUEs];
  Ipv4Address serverAddr[numUEs+numExtraUEs];
  Ipv4StaticRoutingHelper staticRouting_h;
  Ptr<Ipv4StaticRouting> serverStaticRouting[numUEs+numExtraUEs];  // +1 for extra server



  Ipv4AddressHelper ipv4addr_h;
  ipv4addr_h.SetBase ("1.0.0.0", "255.0.0.0");


  for (uint32_t index=0 ; index < numUEs ; index++)
  {
	  server_pgw_ndc[index] = p2pInet_h[index].Install (servers.Get(index),pgw);
	  internetIpv4_ic[index] = ipv4addr_h.Assign (server_pgw_ndc[index]);
	  std::cout << "link0-0 : " << internetIpv4_ic[index].GetAddress(0) << std::endl; 	/*checking code*/
	  std::cout << "link0-1 : " << internetIpv4_ic[index].GetAddress(1) << std::endl; 	/*checking code*/
	  serverAddr[index] = internetIpv4_ic[index].GetAddress (0);
	  std::cout << "server"<<index<<" ipv4address : " << serverAddr[index] << std::endl; 	/*checking code*/
	  serverStaticRouting[index] = staticRouting_h.GetStaticRouting (servers.Get(index)->GetObject<Ipv4> ());
	  serverStaticRouting[index]->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	  staticRouting_h.GetStaticRouting( pgw->GetObject<Ipv4> () )->AddNetworkRouteTo(serverAddr[index],Ipv4Mask ("255.255.255.255"), index +2);

  }


  /*for extra nodes*/

  for (uint32_t index=numUEs ; index < numUEs+numExtraUEs ; index++)
  {
	  server_pgw_ndc[index] = p2pInet_h[index].Install (extraServer,pgw);
	  internetIpv4_ic[index] = ipv4addr_h.Assign (server_pgw_ndc[index]);
	  std::cout << "link0-0 : " << internetIpv4_ic[index].GetAddress(0) << std::endl; 	/*checking code*/
	  std::cout << "link0-1 : " << internetIpv4_ic[index].GetAddress(1) << std::endl; 	/*checking code*/
	  serverAddr[index] = internetIpv4_ic[index].GetAddress (0);
	  std::cout << "extraServer"<<index<<" ipv4address : " << serverAddr[index] << std::endl; 	/*checking code*/
	  serverStaticRouting[index] = staticRouting_h.GetStaticRouting ( extraServer->GetObject<Ipv4> () );
	  serverStaticRouting[index]->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
	  staticRouting_h.GetStaticRouting( pgw->GetObject<Ipv4> () )->AddNetworkRouteTo(serverAddr[3],Ipv4Mask ("255.255.255.255"), 3 +2);

  }




  Ptr<Building> building[numBuildings];
  Ptr<MobilityBuildingInfo> buildingInfo[numBuildings];



  for (int index=0 ; index < numBuildings ; index++)
  {

	  buildingPosition[index] = Box( baseXPosition,baseXPosition+nonLosDuration*ueSpeed,  0.001,0.002,   0,70);
	  baseXPosition = baseXPosition + nonLosDuration*ueSpeed + losDuration*ueSpeed;

	  building[index] = CreateObject<Building>();
	  building[index]->SetBoundaries( buildingPosition[index] );
	  building[index]->SetBuildingType( Building::Office );
	  building[index]->SetExtWallsType( Building::ConcreteWithoutWindows );
	  building[index]->SetNFloors(10);
	  buildingInfo[index] = CreateObject<MobilityBuildingInfo> (building[index]);
	  buildingInfo[index]->SetOutdoor();

  }

  std::cout << "test kkg"<< std::endl;

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();

  enbPositionAlloc->Add(enbPosition);
  MobilityHelper enbMobility_h;
  enbMobility_h.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  enbMobility_h.SetPositionAllocator(enbPositionAlloc);
  enbMobility_h.Install(enbNodes);
  building_h.Install(enbNodes);



  MobilityHelper ueMobility_h;
  ueMobility_h.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  ueMobility_h.Install( ueNodes );





  for (int index=0 ; index < numNlosUEs ; index++)
  {
	  Ptr<ConstantVelocityMobilityModel> consVelocityModel1 = ueNodes.Get(index)->GetObject<ConstantVelocityMobilityModel>();

	  if(ueSpeed <=0)
	  {
		  ueStarting1.x = 2-ueSpeed*2;
	  }

	  consVelocityModel1->SetPosition(ueStarting1);
	  consVelocityModel1->SetVelocity(allStationary? ueVelocitySt:ueVelocityMo);
	  sim_result[index].startPos = ueStarting1;
	  sim_result[index].speed = ueVelocityMo;
	  building_h.Install(ueNodes.Get(index));
	  ueStarting1.x += ueSpeed*positionDiffer;
  }



  for (int index= numNlosUEs ; index< numUEs ; index++) // Stationary user, for from the second user.
  {
	  Ptr<ConstantVelocityMobilityModel> consVelocityModel2 = ueNodes.Get(index)->GetObject<ConstantVelocityMobilityModel>();

	  ueStarting2.x += index;
	  consVelocityModel2->SetPosition(ueStarting2);
	  consVelocityModel2->SetVelocity(allStationary? ueVelocitySt:ueVelocityMo);
	  sim_result[index].startPos = ueStarting2;
	  sim_result[index].speed = ueVelocitySt;
	  building_h.Install(ueNodes.Get(index));
  }



  /*for extra nodes*/


  ueMobility_h.Install( extraUE );
  Ptr<ConstantVelocityMobilityModel> consVelocityModel3 = extraUE->GetObject<ConstantVelocityMobilityModel>();
  ueStarting2.x += 1;
  consVelocityModel3->SetPosition(ueStarting2);
  consVelocityModel3->SetVelocity(allStationary? ueVelocitySt:ueVelocityMo);
  sim_result[numUEs].startPos = ueStarting2;
  sim_result[numUEs].speed = ueVelocitySt;
  building_h.Install( extraUE );


  std::cout << "test kkg 1--"<< std::endl;


  building_h.MakeMobilityModelConsistent();

  std::cout << "test kkg 1-0"<< std::endl;
  /*Configure LTE to enb and ue*/

  NetDeviceContainer enbLte_ndc = mmWave_h->InstallEnbDevice(enbNodes);

  std::cout << "test kkg 1-1"<< std::endl;
  NetDeviceContainer ueLte_ndc = mmWave_h->InstallUeDevice(ueNodes);
  std::cout << "test kkg 1-2"<< std::endl;
  internetStack_h.Install (ueNodes);


  std::cout << "test kkg 2"<< std::endl;

  /*Assign UE's IP and configure default GW*/

  Ipv4InterfaceContainer ueIp_if;
  ueIp_if = mmWaveEpc_h->AssignUeIpv4Address(ueLte_ndc);

  /*Attach ue to enb*/

  mmWave_h->AttachToClosestEnb(ueLte_ndc,enbLte_ndc);



  /*for extra nodes*/

  NetDeviceContainer extraUeLte_ndc = mmWave_h->InstallUeDevice( extraUE );
  internetStack_h.Install ( extraUE );
  Ipv4InterfaceContainer extraUeIp_if;
  extraUeIp_if = mmWaveEpc_h->AssignUeIpv4Address(extraUeLte_ndc);
  mmWave_h->AttachToClosestEnb(extraUeLte_ndc,enbLte_ndc);


  std::cout << "test kkg 4 " << std::endl;




  for (int index=0 ; index < numUEs ; index++)
  {
	  Ptr<Node> ueNode = ueNodes.Get (index);
	  Ptr<Node> enbNode = enbNodes.Get (0);


	  Ptr<Ipv4StaticRouting> ueStaticRouting = staticRouting_h
			                                  .GetStaticRouting (ueNode->GetObject<Ipv4> ());
	  ueStaticRouting->SetDefaultRoute( mmWaveEpc_h->GetUeDefaultGatewayAddress(), 1);


	  /*Sink Application on UE*/
	  uint16_t dlPort = 4380;
//	  Address ueSinkAddress ( InetSocketAddress( ueIp_if.GetAddress(index) , dlPort ) );
//	  std::cout << "ueIP address:" << ueIp_if.Get(index).first->GetAddress(0,0).  << std::cout;
	  Address ueSinkAddress ( InetSocketAddress( ueIp_if.GetAddress(index) , dlPort ) );
//	  std::cout << "ueSink address:" << ueSinkAddress << std::cout;
	  PacketSinkHelper packetSink_h( "ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort ));



	  std::cout << "fname is : " << fname << std::endl;
	  std::string fname_ue="";
	  fname_ue.append( fname );
	  fname_ue.append( "-ue" );
	  std::cout << "fname_ue is : " << fname_ue << std::endl;
	  std::cout << "fname_ue index is : " << index << std::endl;

	  if(enablePcap)
	  {
		  internetStack_h.EnablePcapIpv4(fname_ue, ueNode->GetObject<Ipv4> (), 1 );
	  }



	  ApplicationContainer sink_ac;
	  sink_ac.Add( packetSink_h.Install(ueNode) );
	  sink_ac.Start( Seconds(sink_start));
	  sink_ac.Stop( Seconds(sink_stop) );


	  /*Pump Application on Server*/
	  Ptr<MyTcp> pump_ac = CreateObject<MyTcp>();
//	  Ptr<Socket> tcpSocket = Socket::CreateSocket(server, TcpSocketFactory::GetTypeId());




//	  tcpSocket->SetAttribute("SegmentSize", UintegerValue(segmentSize));




//	  pump_ac->Setup( tcpSocket, ueSinkAddress, packetSize, nPackets, DataRate(dataRate) );


	  std::ostringstream o;
	  o << fname << "_" << index <<"_cwnd.txt";
	  std::string fname_cwnd="";
	  fname_cwnd.append(o.str());

	  AsciiTraceHelper asciiTrace_h;
	  Ptr<OutputStreamWrapper> outStream = asciiTrace_h.CreateFileStream(fname_cwnd);




	  Ptr<Socket> tcpSocket1 = Socket::CreateSocket(servers.Get(index), TcpSocketFactory::GetTypeId());
	  tcpSocket1->SetAttribute("SegmentSize", UintegerValue(segmentSize));

	  pump_ac->Setup( tcpSocket1, ueSinkAddress, packetSize, nPackets, DataRate( dataRateArray[ index%arraySize ]) );
	  sim_result[index].rate = dataRateArray[ index%arraySize ];

	  servers.Get(index)->AddApplication(pump_ac);
	  pump_ac->SetStartTime( Seconds(pump_start));
	  pump_ac->SetStopTime( Seconds(pump_stop) );


	  ueNode->GetApplication(0)->TraceConnectWithoutContext("Rx", MakeCallback(&SpecialEvent1));



	  tcpSocket1->TraceConnectWithoutContext("CongestionWindow" , MakeBoundCallback(&CwndChange, outStream));




	  std::cout << "server node id : " << servers.Get(index)->GetId() << std::endl;
	  std::cout << "enb node id : " << enbNode->GetId() << std::endl;
	  std::cout << "ue node id : " << ueNode->GetId() << std::endl;
	  std::cout << "Pgw node id : " << pgw->GetId() << std::endl;


	  /*Ue Mobility trace*/
	  ueNode->GetObject<MobilityModel>()->TraceConnectWithoutContext("CourseChange", MakeCallback(&CourseChange));



	  std::cout << "how many Netdevices server has... : " << servers.Get(index)->GetNDevices() << std::endl ;
	  std::cout << "how many Netdevices enb has... : " << enbNode->GetNDevices() << std::endl ;
	  std::cout << "how many Netdevices ue has... : " << ueNode->GetNDevices() << std::endl ;



	  Ptr<Ipv4L3Protocol> enb_ipv4L3 = enbNode->GetObject<Ipv4L3Protocol>();
	  std::cout << "enb N-device : " << enbNode->GetNDevices() << std::endl;

	  std::cout << "enb N-interface : " << enb_ipv4L3->GetNInterfaces() << std::endl;

	  for (int index2 = 0 ; index2 < enb_ipv4L3->GetNInterfaces(); index2++ )
	  {
		  std::cout << "enb address " << index2 <<": " <<enb_ipv4L3->GetAddress(index2,0) << std::endl;
	  }

//	  std::cout << "enb address : " << enb_ipv4L3->GetAddress(1,0) << std::endl;
	  std::cout << "enb->GetDevice(2) : "<< enbNode->GetDevice(2)  << std::endl;
	  std::cout << "enb_ipv4L3->GetNetDevice(1) : "<< enb_ipv4L3->GetNetDevice(1)  << std::endl;
	  std::cout << "enb_ipv4L3->GetNetDevice(1) : "<< enb_ipv4L3->GetNetDevice(1)->GetIfIndex()  << std::endl;



	  Ptr<Ipv4L3Protocol> ue_ipv4L3 = ueNode->GetObject<Ipv4L3Protocol>();
	  std::cout << "ue N-device : " << ueNode->GetNDevices() << std::endl;
	  std::cout << "ue N-interface : " << ue_ipv4L3->GetNInterfaces() << std::endl;
	  std::cout << "ue address : " << ue_ipv4L3->GetAddress(0,0) << std::endl;
	  std::cout << "ue address : " << ue_ipv4L3->GetAddress(1,0) << std::endl;
	  std::cout << "ue address local : " << ue_ipv4L3->GetAddress(1,0).GetLocal() << std::endl;
	  sim_result[index].userID = ue_ipv4L3->GetAddress(1,0).GetLocal();

	  std::cout << "ue address to int32 : " << ue_ipv4L3->GetAddress(1,0).GetLocal().Get() << std::endl;
//	  std::string temp = "1.1.1.1";
//	  temp.
//	  Ptr<Ipv4L3Protocol> server_ipv4L3 = server->GetObject<Ipv4L3Protocol>();
//	  std::cout << "server N-device : " << server->GetNDevices() << std::endl;
//	  std::cout << "server N-interface : " << server_ipv4L3->GetNInterfaces() << std::endl;
//	  std::cout << "server address : " << server_ipv4L3->GetAddress(0,0) << std::endl;
//	  std::cout << "server address : " << server_ipv4L3->GetAddress(1,0) << std::endl;

	  Ptr<Ipv4L3Protocol> pgw_ipv4L3 = pgw->GetObject<Ipv4L3Protocol>();
	  std::cout << "pgw N-device : " << pgw->GetNDevices() << std::endl;
	  std::cout << "pgw N-interface : " << pgw_ipv4L3->GetNInterfaces() << std::endl;
	  for (int index2 = 0 ; index2 < pgw_ipv4L3->GetNInterfaces() ; index2++ )
	  {
		  std::cout << "pgw address " << index2 << ": " << pgw_ipv4L3->GetAddress(index2,0) << std::endl;
	  }
//	  std::cout << "pgw address : " << pgw_ipv4L3->GetAddress(0,0) << std::endl;
//	  std::cout << "pgw address : " << pgw_ipv4L3->GetAddress(1,0) << std::endl;
//	  std::cout << "pgw address : " << pgw_ipv4L3->GetAddress(2,0) << std::endl;
//	  std::cout << "pgw address : " << pgw_ipv4L3->GetAddress(3,0) << std::endl;

	  std::cout << "UE position : " << ueNodes.Get(index)->
	  	  	  	  	  GetObject<ConstantVelocityMobilityModel>()->GetPosition() << std::endl;
	  std::cout << "UE speed : " << ueNodes.Get(index)->
	  	  	  	  	  GetObject<ConstantVelocityMobilityModel>()->GetVelocity() << std::endl;



	  std::cout << "---------------------------------------------" << std::endl;

	  if(enableEA)
	  {
		  uint32_t addr_int = ue_ipv4L3->GetAddress(1,0).GetLocal().Get();
		  std::cout << "mcache addr : " << ue_ipv4L3->GetAddress(1,0).GetLocal() << std::endl;

		  if(cachePP == evenPriority)
		  {
//			  caches.Insert( addr_int, Create<MmCache>( (int) maxCachesSize*needFreeUpRatio/(numUEs)) );
			  caches.Insert( addr_int, Create<MmCache>( (int) maxCachesSize/(numUEs*3), index) );
		  }
		  else if(cachePP == proposed0330)
		  {
			  caches.Insert( addr_int, Create<MmCache>( (int) maxCachesSize/(numUEs*3), index) );
		  }
		  else if(cachePP == cacheOptimal)
		  {
			  caches.Insert( addr_int, Create<MmCache>( (int) maxCachesSize/(numUEs*3), index) );
		  }
		  else
		  {
			  caches.Insert( addr_int, Create<MmCache>(iCacheSize,  index) );
		  }

		  caches.GetCache(addr_int)->SetAddr(ue_ipv4L3->GetAddress(1,0).GetLocal());
		  caches.GetCache(addr_int)->SetAddrInt(addr_int);
		  caches.GetCache(addr_int)->m_server_addr = serverAddr[index];
		  caches.GetCache(addr_int)->m_delay = delayArrayDouble[index%arraySize];


		  std::ostringstream o_occ;
		  o_occ << fname << "_" << index <<"_occ.txt";
		  std::string fname_occ="";
		  fname_occ.append(o_occ.str());

		  Ptr<OutputStreamWrapper> outStream_occ = asciiTrace_h.CreateFileStream(fname_occ);
//		  std::cout << "ccoomm1" << std::endl;
		  WriteOccupancy( caches.GetCache(addr_int) , outStream_occ);
//		  std::cout << "ccoomm2" << std::endl;
	  }




  }

  std::cout << "test kkg 5 " << std::endl;


  /*for extra nodes*/
  for( int index=numUEs ; index < numUEs +numExtraUEs ; index++)
  {
  	  Ptr<Node> ueNode = extraUE;
  	  Ptr<Node> enbNode = enbNodes.Get (0);


  	  Ptr<Ipv4StaticRouting> ueStaticRouting = staticRouting_h
  			                                  .GetStaticRouting (ueNode->GetObject<Ipv4> ());
  	  ueStaticRouting->SetDefaultRoute( mmWaveEpc_h->GetUeDefaultGatewayAddress(), 1);

  	  /*Sink Application on UE*/
  	  uint16_t dlPort = 4380;
  	  Address ueSinkAddress ( InetSocketAddress( extraUeIp_if.GetAddress(0) , dlPort ) );
  	  PacketSinkHelper packetSink_h( "ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort ));



  	  std::cout << "fname is : " << fname << std::endl;
  	  std::string fname_ue="";
  	  fname_ue.append( fname );
  	  fname_ue.append( "-ue" );
  	  std::cout << "fname_ue is : " << fname_ue << std::endl;
  	  std::cout << "fname_ue index is : " << 3 << std::endl;

  	  if(enablePcap)
  	  {
  		  internetStack_h.EnablePcapIpv4(fname_ue, ueNode->GetObject<Ipv4> (), 1 );
  	  }

  	  ApplicationContainer sink_ac;
  	  sink_ac.Add( packetSink_h.Install(ueNode) );
  	  sink_ac.Start( extraUeAppear);
  	  sink_ac.Stop( Seconds(sink_stop_ext) );


  	  /*Pump Application on Server*/
  	  Ptr<MyTcp> pump_ac = CreateObject<MyTcp>();
   	  std::ostringstream o;
  	  o << fname << "_" << 3 <<"_cwnd.txt";
  	  std::string fname_cwnd="";
  	  fname_cwnd.append(o.str());
  	  AsciiTraceHelper asciiTrace_h;
  	  Ptr<OutputStreamWrapper> outStream = asciiTrace_h.CreateFileStream(fname_cwnd);
  	  Ptr<Socket> tcpSocket1 = Socket::CreateSocket( extraServer, TcpSocketFactory::GetTypeId());
  	  tcpSocket1->SetAttribute("SegmentSize", UintegerValue(segmentSize));
  	  pump_ac->Setup( tcpSocket1, ueSinkAddress, packetSize, nPackets, DataRate(dataRateExtra) );
  	  extraServer->AddApplication(pump_ac);
  	  pump_ac->SetStartTime( extraUeAppear );
  	  pump_ac->SetStopTime( Seconds(pump_stop_ext) );
  	  ueNode->GetApplication(0)->TraceConnectWithoutContext("Rx", MakeCallback(&SpecialEvent1));
  	  tcpSocket1->TraceConnectWithoutContext("CongestionWindow" , MakeBoundCallback(&CwndChange, outStream));
  	  std::cout << "server node id : " << extraServer->GetId() << std::endl;
  	  std::cout << "enb node id : " << enbNode->GetId() << std::endl;
  	  std::cout << "ue node id : " << ueNode->GetId() << std::endl;
  	  std::cout << "Pgw node id : " << pgw->GetId() << std::endl;


  	  /*Ue Mobility trace*/
  	  ueNode->GetObject<MobilityModel>()->TraceConnectWithoutContext("CourseChange", MakeCallback(&CourseChange));
  	  std::cout << "how many Netdevices server has... : " << extraServer->GetNDevices() << std::endl ;
  	  std::cout << "how many Netdevices enb has... : " << enbNode->GetNDevices() << std::endl ;
  	  std::cout << "how many Netdevices ue has... : " << ueNode->GetNDevices() << std::endl ;
  	  Ptr<Ipv4L3Protocol> enb_ipv4L3 = enbNode->GetObject<Ipv4L3Protocol>();
  	  std::cout << "enb N-device : " << enbNode->GetNDevices() << std::endl;
  	  std::cout << "enb N-interface : " << enb_ipv4L3->GetNInterfaces() << std::endl;

  	  for (int index2 = 0 ; index2 < enb_ipv4L3->GetNInterfaces(); index2++ )
  	  {
  		  std::cout << "enb address " << index2 <<": " <<enb_ipv4L3->GetAddress(index2,0) << std::endl;
  	  }
  	  std::cout << "enb->GetDevice(2) : "<< enbNode->GetDevice(2)  << std::endl;
  	  std::cout << "enb_ipv4L3->GetNetDevice(1) : "<< enb_ipv4L3->GetNetDevice(1)  << std::endl;
  	  std::cout << "enb_ipv4L3->GetNetDevice(1) : "<< enb_ipv4L3->GetNetDevice(1)->GetIfIndex()  << std::endl;

  	  Ptr<Ipv4L3Protocol> ue_ipv4L3 = ueNode->GetObject<Ipv4L3Protocol>();
  	  std::cout << "ue N-device : " << ueNode->GetNDevices() << std::endl;
  	  std::cout << "ue N-interface : " << ue_ipv4L3->GetNInterfaces() << std::endl;
  	  std::cout << "ue address : " << ue_ipv4L3->GetAddress(0,0) << std::endl;
  	  std::cout << "ue address : " << ue_ipv4L3->GetAddress(1,0) << std::endl;
  	  std::cout << "ue address local : " << ue_ipv4L3->GetAddress(1,0).GetLocal() << std::endl;
  	  sim_result[3].userID = ue_ipv4L3->GetAddress(1,0).GetLocal();
  	  std::cout << "ue address to int32 : " << ue_ipv4L3->GetAddress(1,0).GetLocal().Get() << std::endl;
  	  Ptr<Ipv4L3Protocol> pgw_ipv4L3 = pgw->GetObject<Ipv4L3Protocol>();
  	  std::cout << "pgw N-device : " << pgw->GetNDevices() << std::endl;
  	  std::cout << "pgw N-interface : " << pgw_ipv4L3->GetNInterfaces() << std::endl;
  	  for (int index2 = 0 ; index2 < pgw_ipv4L3->GetNInterfaces() ; index2++ )
  	  {
  		  std::cout << "pgw address " << index2 << ": " << pgw_ipv4L3->GetAddress(index2,0) << std::endl;
  	  }
  	  std::cout << "UE position : " << extraUE->
  	  	  	  	  	  GetObject<ConstantVelocityMobilityModel>()->GetPosition() << std::endl;
  	  std::cout << "UE speed : " << extraUE->
  	  	  	  	  	  GetObject<ConstantVelocityMobilityModel>()->GetVelocity() << std::endl;
  	  std::cout << "---------------------------------------------" << std::endl;




  	  Simulator::Schedule( extraUeAppear ,&AddExtraUe, ue_ipv4L3, serverAddr[3], index);


    }


  std::cout << "test kkg 6 " << std::endl;

  AsciiTraceHelper ascii;
  ueMobility_h.EnableAsciiAll(ascii.CreateFileStream("MHtcp22 mobility trace.mob"));

  if(mmwaveTrace)
  {
	  mmWave_h->EnableTraces();
  }


  std::string fname_wired="";
  fname_wired.append(fname);

  if(enablePcap)
  {
	  for( uint32_t index=0 ; index< numUEs+numExtraUEs ; index++)
	  {
		  p2pInet_h[index].EnablePcapAll(fname_wired);
	  }

  }



  if(enableEA)
  {
	  Config::ConnectWithoutContext("/NodeList/2/$ns3::Ipv4L3Protocol/Rx", MakeCallback(&RxPacketTrace));
	  Config::ConnectWithoutContext("/NodeList/2/$ns3::Ipv4L3Protocol/Tx", MakeCallback(&TxPacketTrace));
	  Config::ConnectWithoutContext("/NodeList/0/$ns3::Ipv4L3Protocol/Rx", MakeCallback(&RxPacketTraceGW));
	  Config::ConnectWithoutContext("/NodeList/0/$ns3::Ipv4L3Protocol/Tx", MakeCallback(&TxPacketTraceGW));
  }
  else
  {
	  Config::ConnectWithoutContext("/NodeList/2/$ns3::Ipv4L3Protocol/Tx", MakeCallback(&RxPacketTraceConv));
  }


  Config::Set("/NodeList/*/DeviceList/*/TxQueue/MaxPackets", UintegerValue(basicQueueSize));
  Config::Set("/NodeList/*/DeviceList/*/TxQueue/MaxBytes", UintegerValue(basicQueueSize*1500));
  Config::Set("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RcvBufSize", UintegerValue(rcvBufSize));


  enb_node = enbNodes.Get (0);



  /***************   test   ****************/
  // to change Netdevice buffer at eNB to make buffer over flow

  PointerValue tmp;
  enb_node->GetDevice(2)->GetAttribute("TxQueue", tmp);



  Ptr<Object> txQueue = tmp.GetObject();

  UintegerValue limit;
  txQueue->GetAttribute("MaxPackets", limit);





  std::cout << "Maxpackets at enb net-device , before : " << limit.Get() << std::endl;

  txQueue->SetAttribute("MaxPackets", UintegerValue(basicQueueSize) );

  txQueue->GetAttribute("MaxPackets", limit);

  std::cout << "Maxpackets at enb net-device , after : " << limit.Get() << std::endl;


  for (uint32_t index=0 ; index < numUEs ; index++)
  {
	  servers.Get(index)->GetDevice(1)->GetAttribute("TxQueue", tmp);
	  txQueue = tmp.GetObject();
	  txQueue->GetAttribute("MaxPackets", limit);
	  std::cout << "Maxpackets at server"<<index<< " net-device , after : " << limit.Get() << std::endl;
  }




  Simulator::Stop(Seconds(simTime));
  Simulator::ScheduleDestroy( &Summary);
  Simulator::Run();
  Simulator::Destroy();

  if(needLog)
  {
  log_file.close();
  err_file.close();
  }
  return 0;

}

