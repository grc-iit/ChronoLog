#ifndef __CLOCK_SYNC_
#define __CLOCK_SYNC_

#include <chrono>
#include <time.h>
#include <unistd.h>
#include <climits>
#include <mpi.h>
#include <vector>
#include <iostream>
#include <cassert>
#include <thallium.hpp>
#include <thallium/serialization/proc_input_archive.hpp>
#include <thallium/serialization/proc_output_archive.hpp>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/stl/array.hpp>
#include <thallium/serialization/stl/complex.hpp>
#include <thallium/serialization/stl/deque.hpp>
#include <thallium/serialization/stl/forward_list.hpp>
#include <thallium/serialization/stl/list.hpp>
#include <thallium/serialization/stl/map.hpp>
#include <thallium/serialization/stl/multimap.hpp>
#include <thallium/serialization/stl/multiset.hpp>
#include <thallium/serialization/stl/pair.hpp>
#include <thallium/serialization/stl/set.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/tuple.hpp>
#include <thallium/serialization/stl/unordered_map.hpp>
#include <thallium/serialization/stl/unordered_multimap.hpp>
#include <thallium/serialization/stl/unordered_multiset.hpp>
#include <thallium/serialization/stl/unordered_set.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>


namespace tl=thallium;

class ClocksourceCStyle
{
public:
    ClocksourceCStyle()
    {}
    ~ClocksourceCStyle()
    {}    
    uint64_t getTimestamp()
    {
        struct timespec t{};
        clock_gettime(CLOCK_TAI, &t);
        return t.tv_sec;
    }
};
 
class ClocksourceCPPStyle
{
public:
    ClocksourceCPPStyle()
    {}
    ~ClocksourceCPPStyle()
    {}    
    uint64_t getTimestamp()
    {
        auto t = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	return t;
    }
};


template<class Clocksource>
class ClockSynchronization
{
 
   private:
     Clocksource *clock;
     uint64_t maxError;
     int myrank;
     int numprocs;
     uint64_t myoffset;
     uint64_t unit;
     uint64_t epsilon;
     uint64_t delay;
     bool is_less;
     tl::engine *thallium_server;
     tl::engine *thallium_shm_server;
     tl::engine *thallium_client;
     tl::engine *thallium_shm_client;
     std::vector<tl::endpoint> serveraddrs;
     std::vector<std::string> ipaddrs;
     std::vector<std::string> shmaddrs;
     std::string myipaddr;
     std::string myhostname;
     uint32_t nservers;
     uint32_t serverid;

   public:
     ClockSynchronization(int n1,int n2,std::string &q) : myrank(n1), numprocs(n2)
     {
	 if(q.compare("second")==0) unit = 1000000000;
	 else if(q.compare("microsecond")==0) unit = 1000;
         else if(q.compare("millisecond")==0) unit = 1000000;
	 else unit = 1;
	 myoffset = 0;
	 maxError = 0;
	 epsilon = 200000000;  //400 microseconds (scheduling, measurement errors)
	 delay = 500000; // 200 microseconds network delay for reasonably large messages
	 delay = delay/unit;
	 epsilon = epsilon/unit;
	 is_less = false;
	 nservers = numprocs;
	 serverid = myrank;
     }
     ~ClockSynchronization()
     {}

      void server_client_addrs(tl::engine *t_server,tl::engine *t_client,tl::engine *t_server_shm, tl::engine *t_client_shm,std::vector<std::string> &ips,std::vector<std::string> &shm_addrs,std::vector<tl::endpoint> &saddrs)
      {
               thallium_server = t_server;
               thallium_shm_server = t_server_shm;
               thallium_client = t_client;
               thallium_shm_client = t_client_shm;
               ipaddrs.assign(ips.begin(),ips.end());
               shmaddrs.assign(shm_addrs.begin(),shm_addrs.end());
               myipaddr = ipaddrs[serverid];
               serveraddrs.assign(saddrs.begin(),saddrs.end());

     }
    
      void bind_functions()
      {
	 std::function<void(const tl::request &,int &)> TimeStampFunc(
         std::bind(&ClockSynchronization<Clocksource>::ThalliumLocalTimeStamp,this,std::placeholders::_1, std::placeholders::_2));
         thallium_server->define("RemoteTimeStamp",TimeStampFunc);
         thallium_shm_server->define("RemoteTimeStamp",TimeStampFunc);
      }

     uint64_t Timestamp()
     {
	 if(!is_less)
	 return clock->getTimestamp()/unit+myoffset;
	 else
	  return clock->getTimestamp()/unit-myoffset;
     }

     bool NearTime(uint64_t ts)
     {
	uint64_t myts = Timestamp();
	uint64_t diff = UINT64_MAX;
	if(myts < ts) diff = ts-myts;
	else diff = myts-ts;
	if(diff <= 2*maxError+delay+epsilon) return true;
	else 
	{
		//std::cout <<" diff = "<<diff<<" err = "<<2*maxError+delay+epsilon<<std::endl;
		return false;
	}
     }
     void ThalliumLocalTimeStamp(const tl::request &req, int &pid)
     {
           req.respond(Timestamp());
     }

     uint64_t RequestTimeStamp(int server_id)
     {
           uint64_t b = UINT64_MAX;
           if(ipaddrs[server_id].compare(myipaddr)==0)
           {
                  tl::endpoint ep = thallium_shm_client->lookup(shmaddrs[server_id]);
                  tl::remote_procedure rp = thallium_shm_client->define("RemoteTimeStamp");
                  b = rp.on(ep)(myrank);
           }
           else
           {
                   tl::remote_procedure rp = thallium_client->define("RemoteTimeStamp");
                   b = rp.on(serveraddrs[server_id])(myrank);
           }
           return b;
     }

     void SynchronizeClocks();
     void ComputeErrorInterval();
     void UpdateOffsetMaxError();
     void localsync();

};

#include "../srcs/clocksync.cpp"

#endif
