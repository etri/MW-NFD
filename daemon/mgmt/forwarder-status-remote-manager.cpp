/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "forwarder-status-remote-manager.hpp"
#include "fw/forwarder.hpp"
#include "face/face.hpp"
#include "core/version.hpp"
#include "face/face-system.hpp"

#include "mw-nfd/mw-nfd-global.hpp"


#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-info.hpp>


#include <sstream>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;


#include <iostream>


using namespace ndn;
NFD_LOG_INIT(ForwarderStatusRemoteManager);

namespace nfd {

static const time::milliseconds STATUS_FRESHNESS(5000);

ForwarderStatusRemoteManager::ForwarderStatusRemoteManager(Forwarder& forwarder, Dispatcher& dispatcher,
	FaceSystem& faceSystem
)
  : m_forwarder(forwarder)
  , m_dispatcher(dispatcher)
  , m_faceSystem(faceSystem)
  , m_faceTable(faceSystem.getFaceTable())
  , m_startTimestamp(time::system_clock::now())
{
  m_dispatcher.addStatusDataset("info/status", ndn::mgmt::makeAcceptAllAuthorization(),
                                bind(&ForwarderStatusRemoteManager::listGeneralRemoteStatus, this, _1, _2, _3));
}

ndn::nfd::ForwarderStatus
ForwarderStatusRemoteManager::collectGeneralStatus()
{
  ndn::nfd::ForwarderStatus status;

  status.setNfdVersion(NFD_VERSION_BUILD_STRING);
  status.setStartTimestamp(m_startTimestamp);
  status.setCurrentTimestamp(time::system_clock::now());

#ifndef ETRI_NFD_ORG_ARCH
  size_t nNameTree=0;
  size_t nFib=m_forwarder.getFib().size();
  size_t nPit=0;
  size_t nM=0;
  size_t nCs=0;
  size_t nInInterests=0;
  size_t nOutInterests=0;
  size_t nInData=0;
  size_t nOutData=0;
  size_t nInNacks=0;
  size_t nOutNacks=0;
  size_t nSatisfiedInterests=0;
  size_t nUnsatisfiedInterests=0;

  int32_t workers = getForwardingWorkers();

  uint64_t __attribute__((unused)) inInt[16]={0,};
  uint64_t __attribute__((unused)) outInt[16]={0,};
  uint64_t __attribute__((unused)) inData[16]={0,};
  uint64_t __attribute__((unused)) outData[16]={0,};

  nNameTree+=m_forwarder.getNameTree().size();
  nFib += m_forwarder.getFib().size();
  nPit += m_forwarder.getPit().size();
  nM +=  m_forwarder.getMeasurements().size();
  nCs +=m_forwarder.getCs().size();

  const ForwarderCounters& counters = m_forwarder.getCounters();
  nInInterests+=(counters.nInInterests);
        nOutInterests +=(counters.nOutInterests);
        nInData +=(counters.nInData);
        nOutData += (counters.nOutData);
        nInNacks += (counters.nInNacks);
        nOutNacks += (counters.nOutNacks);
        nSatisfiedInterests += (counters.nSatisfiedInterests);
        nUnsatisfiedInterests +=(counters.nUnsatisfiedInterests);

  for(int32_t i=0;i<workers;i++){

      auto worker = getMwNfd(i);

      nNameTree += worker->getNameTreeTable().size();
      nFib += worker->getFibTable().size();
      nPit += worker->getPitTable().size();
      nM += worker->getMeasurementsTable().size();
      nCs += worker->getCsTable().size();


    const ForwarderCounters& counters = worker->getCountersInfo();
    nInInterests += counters.nInInterests;
    nOutInterests += counters.nOutInterests;
    nInData += counters.nInData;
    nOutData += counters.nOutData;
    nInNacks += counters.nInNacks;
    nOutNacks += counters.nOutNacks;
    nSatisfiedInterests += counters.nSatisfiedInterests;
    nUnsatisfiedInterests += counters.nUnsatisfiedInterests;


  }

  status.setNNameTreeEntries(nNameTree);
  status.setNFibEntries(nFib);
  status.setNPitEntries(nPit);
  status.setNMeasurementsEntries(nM);
  status.setNCsEntries(nCs);

  status.setNInInterests(nInInterests)
        .setNOutInterests(nOutInterests)
        .setNInData(nInData)
        .setNOutData(nOutData)
        .setNInNacks(nInNacks)
        .setNOutNacks(nOutNacks)
        .setNSatisfiedInterests(nSatisfiedInterests)
        .setNUnsatisfiedInterests(nUnsatisfiedInterests);
#else
  status.setNNameTreeEntries(m_forwarder.getNameTree().size());
  status.setNFibEntries(m_forwarder.getFib().size());
  status.setNPitEntries(m_forwarder.getPit().size());
  status.setNMeasurementsEntries(m_forwarder.getMeasurements().size());
  status.setNCsEntries(m_forwarder.getCs().size());

  const ForwarderCounters& counters = m_forwarder.getCounters();
  status.setNInInterests(counters.nInInterests)
        .setNOutInterests(counters.nOutInterests)
        .setNInData(counters.nInData)
        .setNOutData(counters.nOutData)
        .setNInNacks(counters.nInNacks)
        .setNOutNacks(counters.nOutNacks)
        .setNSatisfiedInterests(counters.nSatisfiedInterests)
        .setNUnsatisfiedInterests(counters.nUnsatisfiedInterests);
#endif

  return status;
}

extern std::shared_ptr<ndn::Face> g_internalClientFace;
extern std::shared_ptr<Face> g_internalFace;

void ForwarderStatusRemoteManager::formatStatusJson( ptree& parent, const ndn::nfd::ForwarderStatus& item)
{
	ptree pt;
#if 1
  pt.put ("version", item.getNfdVersion());
  pt.put ("startTime", item.getStartTimestamp());
  pt.put ("currentTime", item.getCurrentTimestamp());
  pt.put ("uptime", time::duration_cast<time::seconds>(item.getCurrentTimestamp()-item.getStartTimestamp()));
  pt.put ("nNameTreeEntries", item.getNNameTreeEntries());
  pt.put ("nFibEntries", item.getNFibEntries());
  pt.put ("nPitEntries", item.getNPitEntries());
  pt.put ("nMeasurementsEntries", item.getNMeasurementsEntries());
  pt.put ("nCsEntries", item.getNCsEntries());
	pt.put("packetCounters.incomingPackets.nInterests", item.getNInInterests());
	pt.put("packetCounters.incomingPackets.nData", item.getNInData());
	pt.put("packetCounters.incomingPackets.nNacks", item.getNInNacks());
	pt.put("packetCounters.outgoingPackets.nInterests", item.getNOutInterests());
	pt.put("packetCounters.outgoingPackets.nData", item.getNOutData());
	pt.put("packetCounters.outgoingPackets.nNacks", item.getNOutNacks());
  pt.put ("nSatisfiedInterests", item.getNSatisfiedInterests());
  pt.put ("nUnsatisfiedInterests", item.getNUnsatisfiedInterests());
#endif

parent.add_child("nfdStatus.generalStatus", pt);
}

void ForwarderStatusRemoteManager::formatChannelsJson( ptree& parent )
{
	ptree pt;
	parent.add_child("nfdStatus.channels", pt);
}
void ForwarderStatusRemoteManager::formatFacesJson( ptree& parent )
{
	ptree pt;
	parent.add_child("nfdStatus.faces", pt);
}
void ForwarderStatusRemoteManager::formatRibJson( ptree& parent )
{
	ptree pt;
	parent.add_child("nfdStatus.rib", pt);
}
void ForwarderStatusRemoteManager::formatFibJson( ptree& parent )
{
	ptree pt;
	parent.add_child("nfdStatus.fib", pt);
}
void ForwarderStatusRemoteManager::formatScJson( ptree& parent )
{
	ptree pt;
	parent.add_child("nfdStatus.strategyChoices", pt);
}
void ForwarderStatusRemoteManager::formatCsJson( ptree& parent )
{
        //ndn::nfd::CsInfo info;

	ptree pt;
        int32_t workers = getForwardingWorkers();
        size_t NEntries = 0;
        size_t NHits = 0;
        size_t NMisses = 0;
        size_t NCapa = 0;

        if(workers==0){
          //      NCapa += m_cs.getLimit();
           //     info.setEnableAdmit(m_cs.shouldAdmit());
            //    info.setEnableServe(m_cs.shouldServe());
        }else{

                for(int32_t i=0;i<workers;i++){
                        auto worker = getMwNfd(i);
                        NCapa += worker->getCsTable().getLimit();
                        NEntries += worker->getCsTable().size();
                        NHits += worker->getCountersInfo().nCsHits;
                        NMisses += worker->getCountersInfo().nCsMisses;

                }
        }

        NEntries += m_forwarder.getCs().size();
        //NHits += m_forwarder.getCountersInfo().nCsHits;
        //NMisses += m_forwarder.getCountersInfo().nCsMisses;

  pt.put ("capacity", NCapa);
  //pt.put ("admitEnabled", m_forwarder.getCs().shouldAdmin());
  //pt.put ("serveEnabled", m_forwarder.getCs().shouldServe());
  pt.put ("nEntries", NEntries);
  pt.put ("nHits", NHits);
  pt.put ("nMisses", NMisses);
parent.add_child("nfdStatus.cs", pt);
}



void
ForwarderStatusRemoteManager::listGeneralRemoteStatus(const Name& topPrefix, const Interest& interest,
                                          ndn::mgmt::StatusDatasetContext& context)
{

	std::cout << "listGeneralRemoteStatus = " << interest << std::endl;
  auto status = this->collectGeneralStatus();


  context.end();
	Name name = interest.getName();
	//name.append("viper");
	auto data = make_shared<Data>(name);
//	Data data(name);
	KeyChain keychain;
	//Block content(55);
//data->setContent(content);
	data->setFreshnessPeriod(1_s);

	keychain.sign(*data, security::SigningInfo(security::SigningInfo::SIGNER_TYPE_SHA256));

// Write json.
  ptree nfd_info;

	
	formatStatusJson(nfd_info, status);
	formatChannelsJson(nfd_info);
	formatFacesJson(nfd_info);
	formatFibJson(nfd_info);
	formatRibJson(nfd_info);
	formatCsJson(nfd_info);
	formatScJson(nfd_info);

  std::ostringstream buf; 
  write_json (buf, nfd_info, false);
  std::string json = buf.str(); 

std::ofstream file;
        file.open("/tmp/json.json");
        file << json;
        file.close();

	std::cout << "JSON: " << std::endl;
	std::cout << json << std::endl;

	g_internalFace->getLinkService()->receivePacket(data->wireEncode(), 1);
	//g_internalClientFace->put(*data);
}

} // namespace nfd
