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

#include "cs-policy.hpp"
#include "cs.hpp"
#include "common/logger.hpp"

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

NFD_LOG_INIT(CsPolicy);

namespace nfd {
namespace cs {

Policy::Registry&
Policy::getRegistry()
{
  static Registry registry;
  return registry;
}

unique_ptr<Policy>
Policy::create(const std::string& policyName)
{
  Registry& registry = getRegistry();
  auto i = registry.find(policyName);
  return i == registry.end() ? nullptr : i->second();
}

std::set<std::string>
Policy::getPolicyNames()
{
  std::set<std::string> policyNames;
  boost::copy(getRegistry() | boost::adaptors::map_keys,
              std::inserter(policyNames, policyNames.end()));
  return policyNames;
}

Policy::Policy(const std::string& policyName)
  : m_policyName(policyName)
{
}

void
Policy::setLimit(size_t nMaxEntries)
{
#ifdef __linux__
  NFD_LOG_INFO("setLimit " << nMaxEntries << " on CPU " << sched_getcpu());
#elif __APPLE__
  NFD_LOG_INFO("setLimit " << nMaxEntries );
#endif
  m_limit = nMaxEntries;
  this->evictEntries();
#ifdef ETRI_DUAL_CS
  this->evictEntriesExact();
#endif
}

void
Policy::setPmLimit(size_t nMaxEntries)
{
#ifdef __linux__
  NFD_LOG_INFO("setPmLimit " << nMaxEntries << " on CPU " << sched_getcpu());
#elif __APPLE__
  NFD_LOG_INFO("setPmLimit " << nMaxEntries );
#endif
  m_pm_limit = nMaxEntries;
  this->evictEntries();
}

void
Policy::setEmLimit(size_t nMaxEntries)
{
#ifdef __linux__
  NFD_LOG_INFO("setEmLimit " << nMaxEntries << " on CPU " << sched_getcpu());
#elif __APPLE__
  NFD_LOG_INFO("setEmLimit " << nMaxEntries );
#endif
  m_em_limit = nMaxEntries;
#ifdef ETRI_DUAL_CS
  this->evictEntriesExact();
#endif
}

void
Policy::afterInsert(EntryRef i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doAfterInsert(i);
}

void
Policy::afterRefresh(EntryRef i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doAfterRefresh(i);
}

void
Policy::beforeErase(EntryRef i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doBeforeErase(i);
}

void
Policy::beforeUse(EntryRef i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doBeforeUse(i);
}

#ifdef ETRI_DUAL_CS

void
Policy::afterInsertExact(EntryRefExact i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doAfterInsertExact(i);
}

void
Policy::afterRefreshExact(EntryRefExact i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doAfterRefreshExact(i);
}

void
Policy::beforeEraseExact(EntryRefExact i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doBeforeEraseExact(i);
}

void
Policy::beforeUseExact(EntryRefExact i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doBeforeUseExact(i);
}
#endif 

} // namespace cs
} // namespace nfd
