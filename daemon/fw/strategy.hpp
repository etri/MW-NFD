/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FW_STRATEGY_HPP
#define NFD_DAEMON_FW_STRATEGY_HPP

#include <iostream>

#include "forwarder.hpp"
#include "table/measurements-accessor.hpp"
#include "common/global.hpp"
#include "mw-nfd/mw-nfd-global.hpp"

namespace nfd {
namespace fw {

/** \brief Represents a forwarding strategy
 */
class Strategy : noncopyable
{
public: // registry
  /** \brief Register a strategy type
   *  \tparam S subclass of Strategy
   *  \param strategyName strategy program name, must contain version
   *  \note It is permitted to register the same strategy type under multiple names,
   *        which is useful in tests and for creating aliases.
   */
  template<typename S>
  static void
  registerType(const Name& strategyName = S::getStrategyName())
  {
    BOOST_ASSERT(strategyName.size() > 1);
    BOOST_ASSERT(strategyName.at(-1).isVersion());
    Registry& registry = getRegistry();
    BOOST_ASSERT(registry.count(strategyName) == 0);
    registry[strategyName] = [] (auto&&... args) {
      return make_unique<S>(std::forward<decltype(args)>(args)...);
    };
  }

  /** \return Whether a strategy instance can be created from \p instanceName
   *  \param instanceName strategy instance name, may contain version and parameters
   *  \note This function finds a strategy type using same rules as \p create ,
   *        but does not attempt to construct an instance.
   */
  static bool
  canCreate(const Name& instanceName);

  /** \return A strategy instance created from \p instanceName
   *  \retval nullptr if !canCreate(instanceName)
   *  \throw std::invalid_argument strategy type constructor does not accept
   *                               specified version or parameters
   */
  static unique_ptr<Strategy>
  create(const Name& instanceName, Forwarder& forwarder);

  /** \return Whether \p instanceNameA and \p instanceNameA will initiate same strategy type
   */
  static bool
  areSameType(const Name& instanceNameA, const Name& instanceNameB);

  /** \return Registered versioned strategy names
   */
  static std::set<Name>
  listRegistered();

public: // constructor, destructor, strategy info
  /** \brief Construct a strategy instance.
   *  \param forwarder a reference to the forwarder, used to enable actions and accessors.
   *  \note Strategy subclass constructor must not retain a reference to \p forwarder.
   */
  explicit
  Strategy(Forwarder& forwarder);

  virtual
  ~Strategy();

#ifdef DOXYGEN
  /** \return Strategy program name
   *
   *  The strategy name is defined by the strategy program.
   *  It must end with a version component.
   */
  static const Name&
  getStrategyName();
#endif

  /** \return Strategy instance name
   *
   *  The instance name is assigned during instantiation.
   *  It contains a version component, and may have extra parameter components.
   */
  const Name&
  getInstanceName() const
  {
    return m_name;
  }

  /** \return Whether the afterNewNextHop trigger should be invoked for this strategy.
   */
  bool
  wantNewNextHopTrigger() const
  {
    return m_wantNewNextHopTrigger;
  }

public: // triggers
  /** \brief Trigger after Interest is received
   *
   *  The Interest:
   *  - has not exceeded HopLimit
   *  - does not violate Scope
   *  - is not looped
   *  - cannot be satisfied by ContentStore
   *  - is under a namespace managed by this strategy
   *
   *  The PIT entry is set to expire after InterestLifetime has elapsed at each downstream.
   *
   *  The strategy should decide whether and where to forward this Interest.
   *  - If the strategy decides to forward this Interest,
   *    invoke \c sendInterest for each upstream, either now or shortly after via a scheduler event,
   *    but before PIT entry expires.
   *    Optionally, the strategy can invoke \c setExpiryTimer to adjust how long it would wait for a response.
   *  - If the strategy has already forwarded this Interest previously and decides to continue waiting,
   *    do nothing.
   *    Optionally, the strategy can invoke \c setExpiryTimer to adjust how long it would wait for a response.
   *  - If the strategy concludes that this Interest cannot be satisfied,
   *    invoke \c rejectPendingInterest to erase the PIT entry.
   *
   *  \warning The strategy must not retain shared_ptr<pit::Entry>, otherwise undefined behavior
   *           may occur. However, the strategy is allowed to store weak_ptr<pit::Entry>.
   */
  virtual void
  afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) = 0;

  /** \brief Trigger before PIT entry is satisfied
   *
   *  This trigger is invoked when an incoming Data satisfies more than one PIT entry.
   *  The strategy can collect measurements information, but cannot manipulate Data forwarding.
   *  When an incoming Data satisfies only one PIT entry, \c afterReceiveData is invoked instead
   *  and given full control over Data forwarding. If a strategy does not override \c afterReceiveData,
   *  the default implementation invokes \c beforeSatisfyInterest.
   *
   *  Normally, PIT entries would be erased after receiving the first matching Data.
   *  If the strategy wishes to collect responses from additional upstream nodes,
   *  it should invoke \c setExpiryTimer within this function to prolong the PIT entry lifetime.
   *  If a Data arrives from another upstream during the extended PIT entry lifetime, this trigger will be invoked again.
   *  At that time, this function must invoke \c setExpiryTimer again to continue collecting more responses.
   *
   *  In this base class this method does nothing.
   *
   *  \warning The strategy must not retain shared_ptr<pit::Entry>, otherwise undefined behavior
   *           may occur. However, the strategy is allowed to store weak_ptr<pit::Entry>.
   */
  virtual void
  beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                        const FaceEndpoint& ingress, const Data& data);

  /** \brief Trigger after a Data is matched in CS
   *
   *  In the base class this method sends \p data to \p ingress
   */
  virtual void
  afterContentStoreHit(const shared_ptr<pit::Entry>& pitEntry,
                       const FaceEndpoint& ingress, const Data& data);

  /** \brief Trigger after Data is received
   *
   *  This trigger is invoked when an incoming Data satisfies exactly one PIT entry,
   *  and gives the strategy full control over Data forwarding.
   *
   *  When this trigger is invoked:
   *  - The Data has been verified to satisfy the PIT entry.
   *  - The PIT entry expiry timer is set to now
   *
   *  Within this function:
   *  - A strategy should return Data to downstream nodes via \c sendData or \c sendDataToAll.
   *  - A strategy can modify the Data as long as it still satisfies the PIT entry, such as
   *    adding or removing congestion marks.
   *  - A strategy can delay Data forwarding by prolonging the PIT entry lifetime via \c setExpiryTimer,
   *    and forward Data before the PIT entry is erased.
   *  - A strategy can collect measurements about the upstream.
   *  - A strategy can collect responses from additional upstream nodes by prolonging the PIT entry
   *    lifetime via \c setExpiryTimer every time a Data is received. Note that only one Data should
   *    be returned to each downstream node.
   *
   *  In the base class this method invokes \c beforeSatisfyInterest trigger and then returns
   *  the Data to downstream faces via \c sendDataToAll.
   */
  virtual void
  afterReceiveData(const shared_ptr<pit::Entry>& pitEntry,
                   const FaceEndpoint& ingress, const Data& data);

  /** \brief Trigger after Nack is received
   *
   *  This trigger is invoked when an incoming Nack is received in response to
   *  an forwarded Interest.
   *  The Nack has been confirmed to be a response to the last Interest forwarded
   *  to that upstream, i.e. the PIT out-record exists and has a matching Nonce.
   *  The NackHeader has been recorded in the PIT out-record.
   *
   *  If the PIT entry is not yet satisfied, its expiry timer remains unchanged.
   *  Otherwise, the PIT entry normally would expire immediately after this function returns.
   *
   *  If the strategy wishes to collect responses from additional upstream nodes,
   *  it should invoke \c setExpiryTimer within this function to retain the PIT entry.
   *  If a Nack arrives from another upstream during the extended PIT entry lifetime, this trigger will be invoked again.
   *  At that time, this function must invoke \c setExpiryTimer again to continue collecting more responses.
   *
   *  In the base class this method does nothing.
   *
   *  \warning The strategy must not retain shared_ptr<pit::Entry>, otherwise undefined behavior
   *           may occur. However, the strategy is allowed to store weak_ptr<pit::Entry>.
   */
  virtual void
  afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack,
                   const shared_ptr<pit::Entry>& pitEntry);

  /** \brief Trigger after Interest dropped (e.g., for exceeding allowed retransmissions)
   *
   *  In the base class this method does nothing.
   */
  virtual void
  onDroppedInterest(const Face& egress, const Interest& interest);

  /** \brief Trigger after new nexthop is added
   *
   *  The strategy should decide whether to send the buffered Interests to the new nexthop.
   *  In the base class, this method does nothing.
   */
  virtual void
  afterNewNextHop(const fib::NextHop& nextHop, const shared_ptr<pit::Entry>& pitEntry);

protected: // actions
  /** \brief Send an Interest packet.
   *  \param pitEntry the PIT entry
   *  \param egress face through which to send out the Interest
   *  \param interest the Interest packet
   *  \return A pointer to the out-record created or nullptr if the Interest was dropped
   */
  VIRTUAL_WITH_TESTS pit::OutRecord*
  sendInterest(const shared_ptr<pit::Entry>& pitEntry, Face& egress,
               const Interest& interest);

  /** \brief Send a Data packet.
   *  \param pitEntry the PIT entry
   *  \param data the Data packet
   *  \param egress face through which to send out the Data
   *  \return Whether the Data was sent (true) or dropped (false)
   */
  VIRTUAL_WITH_TESTS bool
  sendData(const shared_ptr<pit::Entry>& pitEntry, const Data& data, Face& egress);

  /** \brief Send a Data packet to all matched and qualified faces.
   *
   *  A matched face is qualified if it is ad-hoc or it is NOT \p inFace.
   *
   *  \param pitEntry the PIT entry
   *  \param inFace face on which the Data arrived
   *  \param data the Data packet
   */
  VIRTUAL_WITH_TESTS void
  sendDataToAll(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace, const Data& data);

  /** \brief Schedule the PIT entry for immediate deletion.
   *
   *  This helper function sets the PIT entry expiry time to zero.
   *  The strategy should invoke this function when it concludes that the Interest cannot
   *  be forwarded and it does not want to wait for responses from existing upstream nodes.
   */
  VIRTUAL_WITH_TESTS void
  rejectPendingInterest(const shared_ptr<pit::Entry>& pitEntry)
  {
#ifndef ETRI_NFD_ORG_ARCH
      // added by ETRI(modori) on 20210105
      //if(pitEntry->getName().compare(0, 1, "localhop"))
       //getGlobalLogger().info("rejectPendingInterest: {} on CPU {}" , pitEntry->getName().toUri(), sched_getcpu());
#endif
    this->setExpiryTimer(pitEntry, 0_ms);
  }

  /** \brief Send a Nack packet.
   *
   *  The egress face must have a PIT in-record, otherwise this method has no effect.
   *
   *  \param pitEntry the PIT entry
   *  \param egress face through which to send out the Nack
   *  \param header the Nack header
   *  \return Whether the Nack was sent (true) or dropped (false)
   */
  VIRTUAL_WITH_TESTS bool
  sendNack(const shared_ptr<pit::Entry>& pitEntry, Face& egress,
           const lp::NackHeader& header)
  {
    return m_forwarder.onOutgoingNack(pitEntry, egress, header);
  }

  /** \brief Send Nack to every face that has an in-record, except those in \p exceptFaces
   *  \param pitEntry the PIT entry
   *  \param header the Nack header
   *  \param exceptFaces list of faces that should be excluded from sending Nacks
   *  \note This is not an action, but a helper that invokes the sendNack() action.
   */
  void
  sendNacks(const shared_ptr<pit::Entry>& pitEntry, const lp::NackHeader& header,
            std::initializer_list<const Face*> exceptFaces = {});

  /** \brief Schedule the PIT entry to be erased after \p duration.
   */
  void
  setExpiryTimer(const shared_ptr<pit::Entry>& pitEntry, time::milliseconds duration)
  {
    m_forwarder.setExpiryTimer(pitEntry, duration);
  }

protected: // accessors
  /** \brief Performs a FIB lookup, considering Link object if present.
   */
  const fib::Entry&
  lookupFib(const pit::Entry& pitEntry) const;

  MeasurementsAccessor&
  getMeasurements()
  {
    return m_measurements;
  }

  Face*
  getFace(FaceId id) const
  {
    return getFaceTable().get(id);
  }

  const FaceTable&
  getFaceTable() const
  {
    return m_forwarder.m_faceTable;
  }

protected: // instance name
  struct ParsedInstanceName
  {
    Name strategyName; ///< strategy name without parameters
    optional<uint64_t> version; ///< whether strategyName contains a version component
    PartialName parameters; ///< parameter components
  };

  /** \brief Parse a strategy instance name
   *  \param input strategy instance name, may contain version and parameters
   *  \throw std::invalid_argument input format is unacceptable
   */
  static ParsedInstanceName
  parseInstanceName(const Name& input);

  /** \brief Construct a strategy instance name
   *  \param input strategy instance name, may contain version and parameters
   *  \param strategyName strategy name with version but without parameters;
   *                      typically this should be \p getStrategyName()
   *
   *  If \p input contains a version component, return \p input unchanged.
   *  Otherwise, return \p input plus the version component taken from \p strategyName.
   *  This allows a strategy instance to be constructed with an unversioned name,
   *  but its final instance name should contain the version.
   */
  static Name
  makeInstanceName(const Name& input, const Name& strategyName);

  /** \brief Set strategy instance name
   *  \note This must be called by strategy subclass constructor.
   */
  void
  setInstanceName(const Name& name)
  {
    m_name = name;
  }

PUBLIC_WITH_TESTS_ELSE_PROTECTED: // setter
  /** \brief Set whether the afterNewNextHop trigger should be invoked for this strategy
   */
  void
  enableNewNextHopTrigger(bool enabled)
  {
    m_wantNewNextHopTrigger = enabled;
  }

private: // registry
  typedef std::function<unique_ptr<Strategy>(Forwarder& forwarder, const Name& strategyName)> CreateFunc;
  typedef std::map<Name, CreateFunc> Registry; // indexed by strategy name

  static Registry&
  getRegistry();

  static Registry::const_iterator
  find(const Name& instanceName);

protected: // accessors
  signal::Signal<FaceTable, Face>& afterAddFace;
  signal::Signal<FaceTable, Face>& beforeRemoveFace;

private: // instance fields
  Name m_name;

  /** \brief Reference to the forwarder
   *
   *  Triggers can access forwarder indirectly via actions.
   */
  Forwarder& m_forwarder;

  MeasurementsAccessor m_measurements;

  bool m_wantNewNextHopTrigger = false;
};

} // namespace fw
} // namespace nfd

/** \brief Registers a strategy
 *
 *  This macro should appear once in .cpp of each strategy.
 */
#define NFD_REGISTER_STRATEGY(S)                       \
static class NfdAuto ## S ## StrategyRegistrationClass \
{                                                      \
public:                                                \
  NfdAuto ## S ## StrategyRegistrationClass()          \
  {                                                    \
    ::nfd::fw::Strategy::registerType<S>();            \
  }                                                    \
} g_nfdAuto ## S ## StrategyRegistrationVariable

#endif // NFD_DAEMON_FW_STRATEGY_HPP
