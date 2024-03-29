#ifndef _KEEPER_ID_CARD_H
#define _KEEPER_ID_CARD_H

#include <arpa/inet.h>

#include <iostream>

// this class wrapps ChronoKeeper Process identification 
// that will be used by all the ChronoLog Processes 
// to both identofy the Keepr process and create RPC client channels 
// to send the data to the Keeper Recording service

namespace chronolog
{

// Keeper Process can be uniquely identified by the combination of
// the host IP address + client_port

typedef uint32_t        in_addr_t;
typedef uint16_t        in_port_t;
typedef std::pair <in_addr_t, in_port_t> service_endpoint;

// KeeperGroup is the logical grouping of KeeperProcesses
typedef uint64_t    KeeperGroupId;


class KeeperIdCard
{

   uint64_t keeper_group_id;
   uint32_t ip_addr; //IP address as uint32_t in host byte order
   uint16_t port;    //port number as uint16_t in host byte order
   uint16_t tl_provider_id; // id of thallium service provider

public:


  KeeperIdCard( uint64_t group_id = 0, uint32_t addr = 0, uint16_t a_port=0, uint16_t provider_id=0)
	: keeper_group_id(group_id), ip_addr(addr), port(a_port),tl_provider_id(provider_id) 
	{} 

  KeeperIdCard( KeeperIdCard const& other)
  	: keeper_group_id(other.getGroupId()), ip_addr(other.getIPaddr()), port(other.getPort()),tl_provider_id(other.getProviderId())
  	{}

  ~KeeperIdCard()=default;

  uint64_t getGroupId() const { return keeper_group_id; }
  uint32_t getIPaddr() const {return ip_addr; }
  uint16_t getPort() const { return port;}
  uint16_t getProviderId () const { return tl_provider_id; }


  // serialization function used by thallium RPC providers 
  // to serialize/deserialize KeeperIdCard 
  template <typename SerArchiveT>
  void serialize( SerArchiveT & serT)
  {
      serT & keeper_group_id;
      serT & ip_addr;
      serT & port;	
      serT & tl_provider_id;
  }

std::string & getIPasDottedString ( std::string & a_string ) const
{
  
  char buffer[INET_ADDRSTRLEN];
  // convert ip from host to network byte order uint32_t
  uint32_t ip_net_order = htonl(ip_addr); 
  // convert network byte order uint32_t to a dotted string
  if (NULL != inet_ntop(AF_INET, &ip_net_order, buffer, INET_ADDRSTRLEN))
   {a_string += std::string(buffer); }
  return a_string;
}

};

} //namespace chronolog

std::ostream & operator<< (std::ostream & out , chronolog::KeeperIdCard const & keeper_id_card)
{
   std::string a_string;
   out << "KeeperIdCard{"<<keeper_id_card.getGroupId()
	   <<":"<<keeper_id_card.getIPasDottedString(a_string)<<":"<<keeper_id_card.getPort()
	   <<":"<<keeper_id_card.getProviderId()<<"}";
   return out;
}


#endif
