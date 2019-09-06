/*/*-*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2016, University of Padova, Dep. of Information Engineering, SIGNET lab
 *
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
 *
 * Author: Jaume Nin <jnin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
 *
 * Modified by Michele Polese <michele.polese@gmail.com>
 *     (support for RRC_CONNECTED->RRC_IDLE state transition + support for real S1AP link)
 *
 * Modified by Goodsol Lee <gslee2@netlab.snu.ac.kr>
 *     (Add Proxy function for TCP proxy based handover)
 */


#include "epc-enb-proxy-application.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/uinteger.h"
#include "ns3/internet-module.h"

#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-tx-buffer.h"
#include "epc-gtpu-header.h"
#include "eps-bearer-tag.h"





namespace ns3 {

	static void
		Ssthresh (Ptr<OutputStreamWrapper> stream, uint32_t oldSsthresh, uint32_t newSsthresh)
		{
			*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldSsthresh << "\t" << newSsthresh << std::endl;
		}

	static void
		RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
		{
			*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
		}



	NS_LOG_COMPONENT_DEFINE ("EpcEnbProxyApplication");

	TypeId
		EpcEnbProxyApplication::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::EpcEnbProxyApplication")
				.SetParent<Object> ()
				.SetGroupName("Lte");

			return tid;
		}

	void
		EpcEnbProxyApplication::DoDispose (void)
		{
			NS_LOG_FUNCTION (this);
			for(auto it = m_proxyTcpSocketMap.begin(); it != m_proxyTcpSocketMap.end(); it++)
			{
				it->second = 0;
			}
			m_proxyEnbSocket = 0;
		}


	EpcEnbProxyApplication::EpcEnbProxyApplication (Ptr<Node> proxyNode, Ipv4Address proxyAddress, uint16_t proxyTcpPort, Ptr<Socket> proxyEnbSocket, Ipv4Address proxyToEnbAddress,uint32_t proxyBufferSize)
		: //m_proxyTcpSocket (proxyTcpSocket),
			m_proxyEnbSocket (proxyEnbSocket),
			m_proxyToEnbAddress (proxyToEnbAddress),
			m_proxyToEnbUdpPort (8434),
			m_proxyNode (proxyNode),
			m_proxyAddress (proxyAddress),
			m_proxyTcpPort (proxyTcpPort),
			m_proxyBufferSize (proxyBufferSize)// fixed by the standard
	{
		NS_LOG_FUNCTION (this << proxyNode << proxyAddress << proxyTcpPort<< proxyEnbSocket << proxyToEnbAddress);
		//m_proxyTcpSocket->SetRecvCallback (MakeCallback (&EpcEnbProxyApplication::RecvFromTcpSocket, this));
		m_proxyEnbSocket->SetRecvCallback (MakeCallback (&EpcEnbProxyApplication::RecvFromEnbSocket, this));
		//m_proxyTcpSocket->GetObject<TcpSocketBase>()->SetSndBufSize(15*1024*1024);
		m_totalRx = 0;
		m_lastTotalRx = 0;
		m_currentAvailable = 0;
		m_lastAvailable = 0;
		m_count = 0;
		m_count_dep = 0;
		//Simulator::Schedule(Seconds(0.5),&EpcEnbProxyApplication::GetArrivalRate,this);
		m_delay = 0.03;
	}


	EpcEnbProxyApplication::~EpcEnbProxyApplication (void)
	{
		NS_LOG_FUNCTION (this);
	}
	/*
	   void 
	   EpcEnbProxyApplication::RecvFromTcpSocket (Ptr<Socket> socket)
	   {
	   NS_LOG_FUNCTION (this);  
	   NS_ASSERT (socket == m_proxyTcpSocket);
	   Ptr<Packet> packet = socket->Recv ();

	/// \internal
	/// Workaround for \bugid{231}
	//SocketAddressTag satag;
	//packet->RemovePacketTag (satag);
	Ptr<Packet> pCopy = packet-> Copy();

	TcpHeader tempTcpHeader;
	pCopy -> RemoveHeader(tempTcpHeader);
	}
	 */

	//All receiving packet processes are handled by this function
	void 
		EpcEnbProxyApplication::RecvFromEnbSocket (Ptr<Socket> socket)
		{
			NS_LOG_FUNCTION (this << socket);  
			NS_ASSERT (socket == m_proxyEnbSocket);
			Ptr<Packet> packet = socket -> Recv ();

			//m_jitterEstimate.PrepareTx(packet);

			NS_LOG_LOGIC("Packet size before remove header: "<<packet->GetSize());
			GtpuHeader tempGtpuHeader;
			Ipv4Header tempIpv4Header;
			TcpHeader tempTcpHeader;

			packet -> RemoveHeader (tempGtpuHeader);
			packet -> RemoveHeader (tempIpv4Header);
			packet -> RemoveHeader (tempTcpHeader);

			Ipv4Header newIpv4Header = tempIpv4Header;
			//Set Ipv4 Header
			m_source = tempIpv4Header.GetDestination();
			m_dest = tempIpv4Header.GetSource();

			newIpv4Header.SetDestination(m_dest);
			newIpv4Header.SetSource(m_source);
			newIpv4Header.SetProtocol(6);
			newIpv4Header.SetTtl(64);

			TcpHeader newTcpHeader = tempTcpHeader;
			//Set TCP ack header
			SequenceNumber32 dataSeqNum = tempTcpHeader.GetSequenceNumber();
			uint16_t destPort = tempTcpHeader.GetSourcePort();
			uint16_t srcPort = tempTcpHeader.GetDestinationPort();
			newTcpHeader.SetSourcePort(srcPort);
			newTcpHeader.SetDestinationPort(destPort);

			//#1 classify by tcp header: SYN
			if(tempTcpHeader.GetFlags()==TcpHeader::SYN)
			{
				//Start Proxy TCP communication
				NS_LOG_LOGIC("First packet from Server");

				Ptr<Socket> proxyTcpSocket = Socket::CreateSocket (m_proxyNode, TypeId::LookupByName ("ns3::TcpSocketFactory"));
				int retval = proxyTcpSocket->Bind (InetSocketAddress (m_proxyAddress, m_proxyTcpPort));
				m_proxyTcpPort++;
				NS_ASSERT (retval == 0);
				proxyTcpSocket->GetObject<TcpSocketBase>()->SetSndBufSize(m_proxyBufferSize);

				std::ostringstream fileName;
				fileName<<"proxyRtt"<<srcPort<<".txt";
				AsciiTraceHelper asciiTraceHelper;
				Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName.str ().c_str ());
				proxyTcpSocket->GetObject<TcpSocketBase>()->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange,stream));

				std::ostringstream fileName2;
				fileName2<<"proxySst"<<srcPort<<".txt";
				AsciiTraceHelper asciiTraceHelper2;
				Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper2.CreateFileStream (fileName2.str ().c_str ());
				proxyTcpSocket->GetObject<TcpSocketBase>()->TraceConnectWithoutContext ("SlowStartThreshold", MakeBoundCallback (&Ssthresh,stream2));


				Simulator::Schedule(Simulator::Now() + Seconds(0.5),&EpcEnbProxyApplication::GetDepartureRate, this, proxyTcpSocket);

				Address tcpToEnbAddress (InetSocketAddress(m_source,srcPort));
				proxyTcpSocket->Connect(tcpToEnbAddress);

				m_proxyTcpSocketMap.insert(std::make_pair(srcPort,proxyTcpSocket));

				//Send SYN|ACK packet to server, set SYN|ACK packet
				SequenceNumber32 dataSeqNum = SequenceNumber32(0);
				newTcpHeader.SetSequenceNumber(dataSeqNum);
				newTcpHeader.SetFlags(TcpHeader::SYN|TcpHeader::ACK);
				newTcpHeader.SetAckNumber(SequenceNumber32(1));

				Ptr<Packet> ackPacket = Create<Packet> ();
				ackPacket->AddHeader(newTcpHeader);
				ackPacket->AddHeader(newIpv4Header);
				ackPacket->AddHeader(tempGtpuHeader);

				NS_LOG_LOGIC("Packet size: "<<packet->GetSize());
				NS_LOG_LOGIC("Ipv4 Header: "<<newIpv4Header);
				NS_LOG_LOGIC("Tcp Header: "<<newTcpHeader);
				NS_LOG_LOGIC("Gtpu Header: "<<tempGtpuHeader);

				uint32_t flags = 0;
				m_proxyEnbSocket->SendTo (ackPacket, flags, InetSocketAddress (m_proxyToEnbAddress, m_proxyToEnbUdpPort));
			}
			//#2 ACK after SYN|ACK
			else if(packet->GetSize() == 0)
			{
				//Do nothing.. just wait for data packet
			}
			//#3 receive data packet
			else
			{

				Ptr<Socket> proxyTcpSocket = m_proxyTcpSocketMap.find(srcPort)->second;
				//Send data packet from proxy tcp to user
				proxyTcpSocket->Send(packet);
				//Set advertise window
				Ptr<TcpTxBuffer> proxyTxBuffer = proxyTcpSocket->GetObject<TcpSocketBase>()->GetTxBuffer();

				uint32_t awndSize = proxyTxBuffer->Available()-m_delay*m_departureRate;

				//std::cout<<"Max: "<<proxyTxBuffer->MaxBufferSize() <<" Avail: "<<proxyTxBuffer->Available()<<" Awnd: "<<awndSize<<std::endl;

				if(proxyTxBuffer->Available() < m_delay*m_departureRate||awndSize<1400)
				{
					awndSize =packet->GetSize();
				}

				NS_LOG_LOGIC("Proxy tcp's awnd size is "<< awndSize);
				//std::cout<<"awndSize = "<<awndSize<<std::endl;

				//std::cout<<"Tail Sequence: "<<proxyTxBuffer->TailSequence() << std::endl;

				//Send Early ACK packet to server, set ack number
				uint32_t dataSize = packet->GetSize();
				SequenceNumber32 AckNum = dataSeqNum + dataSize;
				newTcpHeader.SetAckNumber(AckNum);
				newTcpHeader.SetFlags(TcpHeader::ACK);
				newTcpHeader.SetWindowSize(awndSize);

				Ptr<Packet> ackPacket = Create<Packet> ();
				ackPacket->AddHeader(newTcpHeader);
				ackPacket->AddHeader(newIpv4Header);
				ackPacket->AddHeader(tempGtpuHeader);

				m_totalRx += dataSize;

				uint32_t flags = 0;
				m_proxyEnbSocket->SendTo (ackPacket, flags, InetSocketAddress (m_proxyToEnbAddress, m_proxyToEnbUdpPort));
			}
		}



	void
		EpcEnbProxyApplication::GetArrivalRate ()
		{
			NS_LOG_FUNCTION (this);
			m_arrivalRate = (m_totalRx - m_lastTotalRx)/(double)(0.1);
			m_count++;
			m_lastTotalRx = m_totalRx;
			//std::cout<<"Arrival rate is "<<m_arrivalRate<<std::endl;
			Simulator::Schedule(MilliSeconds(100),&EpcEnbProxyApplication::GetArrivalRate,this);
		}

	void
		EpcEnbProxyApplication::GetDepartureRate (Ptr <Socket> proxyTcpSocket)
		{
			NS_LOG_FUNCTION (this);
			m_currentAvailable = proxyTcpSocket->GetObject<TcpSocketBase>()->GetTxBuffer()->Available();
			m_departureRate = (m_lastAvailable - m_currentAvailable)/(double)(0.1);
			if(m_currentAvailable > m_lastAvailable)
			{
				m_departureRate = 0;
			}
			m_count_dep++;
			m_lastAvailable = m_currentAvailable;
			//std::cout<<"Current: "<<m_currentAvailable<<" Departure rate is "<<m_departureRate<<std::endl;
			//m_departureRate = 0;
			Simulator::Schedule(MilliSeconds(100),&EpcEnbProxyApplication::GetDepartureRate,this,proxyTcpSocket);
		}

	void 
		EpcEnbProxyApplication::SendToEnbSocket (Ptr<Packet> packet)
		{
			NS_LOG_FUNCTION (this << packet << packet->GetSize ());
			int sentBytes = m_proxyEnbSocket->Send (packet);
			NS_ASSERT (sentBytes > 0);
		}

}  // namespace ns3
