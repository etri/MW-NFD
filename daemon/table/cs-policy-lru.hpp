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

#ifndef NFD_DAEMON_TABLE_CS_POLICY_LRU_HPP
#define NFD_DAEMON_TABLE_CS_POLICY_LRU_HPP

#include "cs-policy.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

namespace nfd {
namespace cs {
namespace lru {

#ifdef ETRI_DUAL_CS
struct EntryRefHasherExact
{
	size_t
	operator()(const Policy::EntryRefExact& t) const {
	    return nfd::name_tree::computeHash(t->first);
	}
};

struct EntryRefComparatorExact {
	bool
	operator()(const Policy::EntryRefExact& t1, const Policy::EntryRefExact& t2) const {
		if(t1->first == t2->first)
			return true;
		return false;
	}
};
using QueueExact = boost::multi_index_container< 
                Policy::EntryRefExact,
                boost::multi_index::indexed_by<
                  boost::multi_index::sequenced<>,
                  boost::multi_index::hashed_unique<boost::multi_index::identity<Policy::EntryRefExact>, EntryRefHasherExact, EntryRefComparatorExact>
                >
              >;
#endif

using Queue = boost::multi_index_container<
                Policy::EntryRef,
                boost::multi_index::indexed_by<
                  boost::multi_index::sequenced<>,
                  boost::multi_index::ordered_unique<boost::multi_index::identity<Policy::EntryRef>>
                >
              >;
/** \brief Least-Recently-Used (LRU) replacement policy
 */
class LruPolicy : public Policy
{
public:
  LruPolicy();

public:
  static const std::string POLICY_NAME;

private:
  void
  doAfterInsert(EntryRef i) override;

  void
  doAfterRefresh(EntryRef i) override;

  void
  doBeforeErase(EntryRef i) override;

  void
  doBeforeUse(EntryRef i) override;

  void
  evictEntries() override;

#ifdef ETRI_DUAL_CS
  void
  doAfterInsertExact(EntryRefExact i) override;

  void
  doAfterRefreshExact(EntryRefExact i) override;

  void
  doBeforeEraseExact(EntryRefExact i) override;

  void
  doBeforeUseExact(EntryRefExact i) override;

  void
  evictEntriesExact() override;
#endif

private:
  /** \brief moves an entry to the end of queue
   */
  void
  insertToQueue(EntryRef i, bool isNewEntry);

#ifdef ETRI_DUAL_CS
  void
  insertToQueueExact(EntryRefExact i, bool isNewEntry);

  QueueExact m_queueExact;
#endif

private:
  Queue m_queue;
};

} // namespace lru

using lru::LruPolicy;

} // namespace cs
} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_POLICY_LRU_HPP
