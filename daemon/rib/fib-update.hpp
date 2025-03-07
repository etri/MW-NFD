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

#ifndef NFD_DAEMON_RIB_FIB_UPDATE_HPP
#define NFD_DAEMON_RIB_FIB_UPDATE_HPP

#include "core/common.hpp"

namespace nfd {
namespace rib {

/** \class FibUpdate
 *  \brief represents a FIB update
 */
class FibUpdate
{
public:
  FibUpdate()
    : faceId(0)
    , cost(0)
            //added MODORI 20210626
    , flags(0)
  {
  }

  bool
  operator==(const FibUpdate& other) const
  {
    return (this->name == other.name &&
            this->faceId == other.faceId &&
            this->cost == other.cost &&
            //added MODORI
            this->flags == other.flags &&
            this->action == other.action);
  }

  static FibUpdate
#ifdef ETRI_DCN_ROUTING
  createAddUpdate(const Name& name, const uint64_t faceId, const uint64_t cost, const uint64_t flags);
#else
  createAddUpdate(const Name& name, const uint64_t faceId, const uint64_t cost);
#endif

  static FibUpdate
  createRemoveUpdate(const Name& name, const uint64_t faceId);

  enum Action {
    ADD_NEXTHOP    = 0,
    REMOVE_NEXTHOP = 1
  };

public:
  Name name;
  uint64_t faceId;
  uint64_t cost;
  Action action;
  uint64_t flags;
};

inline std::ostream&
operator<<(std::ostream& os, const FibUpdate& update)
{
  os << "FibUpdate("
     << " Name: " << update.name << ", "
     << "faceId: " << update.faceId << ", ";

  if (update.action == FibUpdate::ADD_NEXTHOP) {
    os << "cost: " << update.cost << ", "
       << "action: ADD_NEXTHOP";
  }
  else {
    os << "action: REMOVE_NEXTHOP";
  }

  os << ")";

  return os;
}

} // namespace rib
} // namespace nfd

#endif // NFD_DAEMON_RIB_FIB_UPDATE_HPP
