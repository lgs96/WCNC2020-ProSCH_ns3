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
 * Modified by: Michele Polese <michele.polese@gmail.com> 
 *          Support for real S1AP link
 *
 * Modified by: Goodsol Lee <gslee2@netlab.snu.ac.kr>
 * 			Proxy tcp application
 */

#ifndef EPC_ENB_PROXY_APPLICATION_H
#define EPC_ENB_PROXY_APPLICATION_H

#include <ns3/address.h>
#include <ns3/socket.h>
#include <ns3/virtual-net-device.h>
#include <ns3/traced-callback.h>
#include <ns3/callback.h>
#include <ns3/ptr.h>
#include <ns3/object.h>
#include <ns3/lte-common.h>
#include <ns3/application.h>
#include <ns3/eps-bearer.h>
#include <ns3/epc-enb-s1-sap.h>
#include <ns3/epc-s1ap-sap.h>
#include <map>
#include "ns3/internet-module.h"

namespace ns3 {
class EpcEnbS1SapUser;
class EpcEnbS1SapProvider;


/**
 * \ingroup lte
 *
 * This application is installed inside eNBs and provides the bridge functionality for user data plane packets between the radio interface and the S1-U interface.
 */
class EpcEnbProxyApplication : public Application
{
  // inherited from Object
public:
  static TypeId GetTypeId (void);
protected:
  void DoDispose (void);

public:
  
  

  /** 
   * Constructor
   * 
   * \param lteSocket the socket to be used to send/receive packets to/from the LTE radio interface
   * \param s1uSocket the socket to be used to send/receive packets
   * to/from the S1-U interface connected with the SGW 
   * \param enbS1uAddress the IPv4 address of the S1-U interface of this eNB
   * \param sgwS1uAddress the IPv4 address at which this eNB will be able to reach its SGW for S1-U communications
   * \param cellId the identifier of the enb
   */
  EpcEnbProxyApplication (Ptr<Socket> proxyTcpSocket, Ptr<Socket> proxyEnbSocket, Ipv4Address proxyToEnbAddress);

  /**
   * Destructor
   * 
   */
  virtual ~EpcEnbProxyApplication (void);

  void RecvFromEnbSocket (Ptr<Socket> socket);

  //void RecvFromS1uSocket (Ptr<Socket> socket);

  void GetArrivalRate ();
  void GetDepartureRate ();

private:

  void SendToEnbSocket (Ptr<Packet> packet);

  Ptr<Socket> m_proxyTcpSocket;
  Ptr<Socket> m_proxyEnbSocket;

  Ipv4Address m_proxyToEnbAddress;

  uint16_t m_proxyToEnbUdpPort;

  Ipv4Address m_source;
  Ipv4Address m_dest;

  //Process9
  uint64_t m_totalRx;
  uint64_t m_lastTotalRx;
  uint16_t m_count;
  uint16_t m_count_dep;
  uint32_t m_arrivalRate;
  uint32_t m_departureRate;
  uint32_t m_currentAvailable;
  uint32_t m_lastAvailable;
  uint32_t m_delay;
};

} //namespace ns3

#endif /* EPC_ENB_APPLICATION_H */

