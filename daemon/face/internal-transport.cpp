/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "internal-transport.hpp"
#include "mw-nfd/mw-nfd-global.hpp"

#include <iostream>
using namespace std;

namespace nfd {
namespace face {

NFD_LOG_MEMBER_INIT(InternalForwarderTransport, InternalForwarderTransport);
NFD_LOG_MEMBER_INIT(InternalClientTransport, InternalClientTransport);

InternalForwarderTransport::InternalForwarderTransport(const FaceUri& localUri, const FaceUri& remoteUri,
                                                       ndn::nfd::FaceScope scope, ndn::nfd::LinkType linkType)
{
  this->setLocalUri(localUri);
  this->setRemoteUri(remoteUri);
  this->setScope(scope);
  this->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  this->setLinkType(linkType);
  this->setMtu(MTU_UNLIMITED);

  NFD_LOG_FACE_DEBUG("Creating transport");
}

void
InternalForwarderTransport::receivePacket(const Block& packet)
{
  getGlobalIoService().post([this, packet] {
    NFD_LOG_FACE_TRACE("MODORI-Received: " << packet.size() << " bytes");

    // added by ETRI(modori) on 20201110
#if 1
    receive(packet);
#else
    int32_t packetType;
    int32_t worker;

    //print_payload(packet.wire(), packet.size());
    std::tie(packetType, worker) = getWorkerId( packet.wire(), packet.size() );

    auto b = make_shared<ndn::Buffer>(packet.wire(), packet.size());
    NDN_MSG msg;
    msg.buffer = b;
    msg.endpoint = 0;
    msg.faceId = 1;

    if( worker < 0 ){
        std::cout << "InternalForwarderTransport's worker is < 0" << std::endl;
        return;
    }

     if(packetType==tlv::Interest){
        nfd::g_dcnMoodyMQ[ 0 ][worker]->try_enqueue(msg);
     } else{
        nfd::g_dcnMoodyMQ[ 1 ][worker]->try_enqueue(msg);
     }
#endif
  });
}

void
InternalForwarderTransport::doSend(const Block& packet, const EndpointId& id)
{
  NFD_LOG_FACE_TRACE("Sending to " << m_peer);
//    std::cout << "Sending... packet.type: " << packet.type() << ", Id: " << id << ", to " << m_peer << std::endl;

  if (m_peer)
    m_peer->receivePacket(packet);
}

void
InternalForwarderTransport::doClose()
{
  NFD_LOG_FACE_TRACE(__func__);

  setState(TransportState::CLOSED);
}

InternalClientTransport::~InternalClientTransport()
{
  if (m_forwarder != nullptr) {
    m_forwarder->setPeer(nullptr);
  }
}

void
InternalClientTransport::connectToForwarder(InternalForwarderTransport* forwarder)
{
  NFD_LOG_DEBUG(__func__ << " " << forwarder);

  if (m_forwarder != nullptr) {
    // disconnect from the old forwarder transport
    m_forwarder->setPeer(nullptr);
    m_fwTransportStateConn.disconnect();
  }

  m_forwarder = forwarder;

  if (m_forwarder != nullptr) {
    // connect to the new forwarder transport
    m_forwarder->setPeer(this);
    m_fwTransportStateConn = m_forwarder->afterStateChange.connect(
      [this] (TransportState oldState, TransportState newState) {
        if (newState == TransportState::CLOSED) {
          connectToForwarder(nullptr);
        }
      });
  }
}

void
InternalClientTransport::receivePacket(const Block& packet)
{
  getGlobalIoService().post([this, packet] {
    NFD_LOG_TRACE("Received: " << packet.size() << " bytes");
    if (m_receiveCallback) {
      m_receiveCallback(packet);
    }
  });
}

void
InternalClientTransport::send(const Block& wire)
{
  NFD_LOG_TRACE("Sending to " << m_forwarder);

  if (m_forwarder)
    m_forwarder->receivePacket(wire);
}

void
InternalClientTransport::send(const Block& header, const Block& payload)
{
  ndn::EncodingBuffer encoder(header.size() + payload.size(), header.size() + payload.size());
  encoder.appendByteArray(header.wire(), header.size());
  encoder.appendByteArray(payload.wire(), payload.size());

  send(encoder.block());
}

} // namespace face
} // namespace nfd
