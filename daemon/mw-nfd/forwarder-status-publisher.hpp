
#ifndef NFD_DAEMON_MGMT_FORWARDER_STATUS_REMOTE_HPP
#define NFD_DAEMON_MGMT_FORWARDER_STATUS_REMOTE_HPP

#include <map>

#include "face/face-system.hpp"
#include "fw/face-table.hpp"

#include <ndn-cxx/face.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using boost::property_tree::ptree;

#include <ndn-cxx/mgmt/nfd/forwarder-status.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-info.hpp>

using std::map;

namespace nfd {

class ForwarderStatusPublisher: noncopyable
{
public:
  ForwarderStatusPublisher();
  void
  publish(const ndn::Name &, const Interest &, ndn::Face &);

    std::string prepareNextData();


private:
  ndn::nfd::ForwarderStatus
  collectGeneralStatus();


	void formatStatusJson( ptree &, const ndn::nfd::ForwarderStatus&);
	void formatCsJson( ptree & );
	void formatScJson( ptree & );
	void formatFacesJson( ptree & );
	void formatChannelsJson( ptree & );
	void formatRibJson( ptree & );
	void formatFibJson( ptree & );
	//using DataContainer = std::map<uint64_t, shared_ptr<ndn::Data>>;
	  //DataContainer m_data;
	  ndn::KeyChain m_keyChain;
      std::string m_nfdStatus;

};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_FORWARDER_STATUS_HPP
