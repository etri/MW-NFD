
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2019-2021,  HII of ETRI,
 *
 * This file is part of MW-NFD (Named Data Networking Multi-Worker Forwarding Daemon).
 * See README.md for complete list of NFD authors and contributors.
 *
 * MW-NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * MW-NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_IW_NFD_HPP
#define NFD_IW_NFD_HPP

#ifndef ETRI_NFD_ORG_ARCH
//#include <spdlog/spdlog.h>
#endif

#include "face/face.hpp"
#include "common/global.hpp"
#include "fw/face-table.hpp"

#include "face/multicast-ethernet-transport.hpp"
#include "face/generic-link-service.hpp"
#include "face/network-predicate.hpp"
#include "face/tcp-channel.hpp"
#include "face/udp-channel.hpp"
#include "face/ethernet-channel.hpp"
#include <ndn-cxx/net/network-monitor-stub.hpp>
#include <ndn-cxx/face.hpp>
#include "face/udp-factory.hpp"
#include "face/protocol-factory.hpp"

#include <string>
#include <iostream>

namespace nfd {

namespace face {
class Face;
} // namespace face

using namespace std;

class InputThread : noncopyable
{
    public:
        explicit
            InputThread();

        ~InputThread();

        void initialize(int32_t, const std::string ifname);

         void terminate(const boost::system::error_code& error, int signalNo)
         {
                 getGlobalIoService().stop();
         }

         void run();
    private:
        boost::asio::signal_set m_terminationSignalSet;
		int m_Id;
};

} // namespace nfd

#endif // NFD_DAEMON_NFD_HPP
