/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
#include "ns3/tcp-option.h"
#include "ns3/tcp-option-ts.h"
//#include "ns3/epc-enb-application.h"

namespace ns3 {

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
			m_proxyTcpSocket = 0;
			m_proxyEnbSocket = 0;
		}


	EpcEnbProxyApplication::EpcEnbProxyApplication (Ptr<Socket> proxyTcpSocket, Ptr<Socket> proxyEnbSocket, Ipv4Address proxyToEnbAddress)
		: m_proxyTcpSocket (proxyTcpSocket),
		m_proxyEnbSocket (proxyEnbSocket),
		m_proxyToEnbAddress (proxyToEnbAddress),
		m_proxyToEnbUdpPort (8434),
		m_holdBuffer (false) // fixed by the standard
	{
		NS_LOG_FUNCTION (this << proxyTcpSocket << proxyEnbSocket << proxyToEnbAddress);
		//m_proxyTcpSocket->SetRecvCallback (MakeCallback (&EpcEnbProxyApplication::RecvFromTcpSocket, this));
		m_proxyEnbSocket->SetRecvCallback (MakeCallback (&EpcEnbProxyApplication::RecvFromEnbSocket, this));
		m_proxyTcpSocket->GetObject<TcpSocketBase>()->SetSndBufSize(24*1024*1024);
		m_totalRx = 0;
		m_lastTotalRx = 0;
		m_currentAvailable = 0;
		m_lastAvailable = 0;
		m_count = 0;
		m_count_dep = 0;
		Simulator::Schedule(Seconds(0.5),&EpcEnbProxyApplication::GetArrivalRate,this);
		Simulator::Schedule(Seconds(0.5),&EpcEnbProxyApplication::GetDepartureRate,this);
		m_delay = 0.01;
		//m_temp = true;
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


			NS_LOG_LOGIC("Packet size before remove header: "<<packet->GetSize());
			Ipv4Header tempIpv4Header;
			TcpHeader tempTcpHeader;
			packet -> RemoveHeader (tempIpv4Header);
			packet -> RemoveHeader (tempTcpHeader);

			m_Ipv4Header = tempIpv4Header;
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
				Address tcpToEnbAddress (InetSocketAddress(m_source,srcPort));
				m_proxyTcpSocket->Connect(tcpToEnbAddress);

				//Send SYN|ACK packet to server, set SYN|ACK packet
				SequenceNumber32 dataSeqNum = SequenceNumber32(0);
				newTcpHeader.SetSequenceNumber(dataSeqNum);
				newTcpHeader.SetFlags(TcpHeader::SYN|TcpHeader::ACK);
				newTcpHeader.SetAckNumber(SequenceNumber32(1));

				Ptr<Packet> ackPacket = Create<Packet> ();
				ackPacket->AddHeader(newTcpHeader);
				ackPacket->AddHeader(newIpv4Header);

				NS_LOG_LOGIC("Packet size: "<<packet->GetSize());
				NS_LOG_LOGIC("Ipv4 Header: "<<newIpv4Header);
				NS_LOG_LOGIC("Tcp Header: "<<newTcpHeader);

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
				if(m_proxyTcpSocket->GetObject<TcpSocketBase>()->m_proxyHoldBuffer)
				{
					NS_LOG_LOGIC("Hold buffer phase!!");
					Ptr<TcpTxBuffer> proxyTxBuffer = m_proxyTcpSocket->GetObject<TcpSocketBase>()->GetTxBuffer();
					proxyTxBuffer->Add(packet);
					//NS_ASSERT(isIn==true);				

					uint32_t awndSize = proxyTxBuffer->Available()-m_delay*m_arrivalRate;

					if(proxyTxBuffer->Available() < m_delay*m_arrivalRate||awndSize<1400)
					{
						awndSize = 1400;
					}

					NS_LOG_LOGIC("Proxy tcp's awnd size is "<< awndSize);

					//Send Early ACK packet to server, set ack number
					uint32_t dataSize = packet->GetSize();
					SequenceNumber32 AckNum = dataSeqNum + dataSize;
					newTcpHeader.SetAckNumber(AckNum);
					newTcpHeader.SetFlags(TcpHeader::ACK);
					newTcpHeader.SetWindowSize(awndSize);

					Ptr<Packet> ackPacket = Create<Packet> ();
					ackPacket->AddHeader(newTcpHeader);
					ackPacket->AddHeader(newIpv4Header);

                                        //std::cout<<"When hold buffer operating.. Seq " << AckNum << " is arrived, Tail sequence: "<< proxyTxBuffer->TailSequence() << std::endl;
 
					m_totalRx += dataSize;

					uint32_t flags = 0;
					m_proxyEnbSocket->SendTo (ackPacket, flags, InetSocketAddress (m_proxyToEnbAddress, m_proxyToEnbUdpPort));
				}
				else
				{
					//Send data packet from proxy tcp to user
				
		//			if(m_temp)
		
					m_proxyTcpSocket->Send(packet);
			
					//Set advertise window
					Ptr<TcpTxBuffer> proxyTxBuffer = m_proxyTcpSocket->GetObject<TcpSocketBase>()->GetTxBuffer();

					uint32_t awndSize = proxyTxBuffer->Available()-m_delay*m_departureRate;

					if(proxyTxBuffer->Available() < m_delay*m_departureRate||awndSize<1400)
					{
						awndSize =1400;
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

					m_totalRx += dataSize;

					uint32_t flags = 0;
					m_proxyEnbSocket->SendTo (ackPacket, flags, InetSocketAddress (m_proxyToEnbAddress, m_proxyToEnbUdpPort));
				}
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
		EpcEnbProxyApplication::GetDepartureRate ()
		{
			NS_LOG_FUNCTION (this);
			m_currentAvailable = m_proxyTcpSocket->GetObject<TcpSocketBase>()->GetTxBuffer()->Available();
			m_departureRate = (m_currentAvailable - m_lastAvailable)/(double)(0.1);
			if(m_currentAvailable < m_lastAvailable)
			{
				m_departureRate = 0;
			}
			m_count_dep++;
			m_lastAvailable = m_currentAvailable;
			//std::cout<<"Departure rate is "<<m_departureRate<<std::endl;
			Simulator::Schedule(MilliSeconds(100),&EpcEnbProxyApplication::GetDepartureRate,this);
		}

	void
		EpcEnbProxyApplication::SendToEnbSocket (Ptr<Packet> packet)
		{
			NS_LOG_FUNCTION (this << packet << packet->GetSize ());
			int sentBytes = m_proxyEnbSocket->Send (packet);
			NS_ASSERT (sentBytes > 0);
		}
/*
	//Process8
	void
		EpcEnbProxyApplication::ForwardingProxy (uint32_t seq, double delay, double interval)
		{
//			m_temp = false;
			NS_LOG_FUNCTION (this);
			//std::cout << Simulator::Now() <<" Handover occured. Forward cached inflight packets."<<std::endl;
			Ptr<TcpSocketBase> tempSocket = m_proxyTcpSocket->GetObject<TcpSocketBase>();
			Ptr<TcpTxBuffer> proxyTxBuffer = tempSocket->GetTxBuffer();

			uint32_t newSeq = seq  + (interval + delay) * m_arrivalRate;

			std::cout<<"Head seq: "<<proxyTxBuffer->HeadSequence()<<std::endl;
			std::cout<<"Additional: "<<(interval + delay) * m_arrivalRate<<std::endl;
			
			if(proxyTxBuffer->HeadSequence().GetValue()>seq)
			{
				newSeq = proxyTxBuffer->HeadSequence().GetValue();
			}
			
			tempSocket->ProxyBufferRetransmit(SequenceNumber32(newSeq),true);
		}
*/
	//Process_Last
	void
		EpcEnbProxyApplication::ForwardingProxy (uint32_t seq, double delay, double interval)
		{
			NS_LOG_FUNCTION (this);
			Ptr<TcpSocketBase> tempSocket = m_proxyTcpSocket->GetObject<TcpSocketBase>();
			Ptr<TcpTxBuffer> proxyTxBuffer = tempSocket -> GetTxBuffer();
		
			uint32_t newSeq = seq + (interval + delay) * m_arrivalRate;		

			std::cout<<"Head seq: "<<proxyTxBuffer->HeadSequence()<<std::endl;
			std::cout<<"Additional: "<<(interval + delay) * m_arrivalRate<<std::endl;
	
			Ptr<TcpSocketState>m_tcb = tempSocket->m_tcb;

			SequenceNumber32 startPoint = m_tcb->m_nextTxSequence - SequenceNumber32(newSeq);
			startPoint = startPoint - SequenceNumber32(startPoint.GetValue()%m_tcb->m_segmentSize) + 1;

			if((startPoint < proxyTxBuffer->HeadSequence()|| startPoint > m_tcb->m_nextTxSequence||startPoint > proxyTxBuffer->TailSequence()))
			{
				startPoint = proxyTxBuffer -> HeadSequence();
			}

			SequenceNumber32 tempSeq;
			tempSeq = startPoint;
			std::cout<<"Start point: " <<startPoint<<std::endl;
			while(tempSeq < m_tcb->m_nextTxSequence)
			{
				Ptr <Packet> proxyPacket = tempSocket -> GetProxyPacket(tempSeq,m_tcb->m_segmentSize,true);
	
				proxyPacket->AddHeader(m_Ipv4Header);
				
				Address addr;
				Address addr2;
				m_enbApp -> RecvFromTunDevice (proxyPacket,addr,addr2,0);
				//std::cout<<"Send: "<<tempSeq<<std::endl;
				//proxyPacket->Print(std::cout);
				tempSeq = tempSeq + m_tcb-> m_segmentSize;	
			}

			tempSocket->ProxyBufferRetransmit(SequenceNumber32(newSeq),true);
		}
		

	//Process8
	void
		EpcEnbProxyApplication::HoldProxyBuffer()
		{
			NS_LOG_FUNCTION (this);
			//std::cout << Simulator::Now() <<" Handover is prepared. Hold proxy buffer until path switching."<<std::endl;

			m_proxyTcpSocket->GetObject<TcpSocketBase>()->m_proxyHoldBuffer = true;
		}

	void 
		EpcEnbProxyApplication::AddEnbApp(Ptr<EpcEnbApplication> epcApp)
		{
			NS_LOG_FUNCTION (this);
			m_enbApp = epcApp;
		}

}  // namespace ns3
