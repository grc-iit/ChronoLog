#ifndef __PSCLIENT_H_
#define __PSCLIENT_H_

#include "KeyValueStore.h"
#include "MessageCache.h"
#include <mpi.h>
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
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cassert>
#include <thread>

class pubsubclient
{
   private :
	   int numprocs;
	   int myrank;
	   std::vector<std::vector<int>> subscribers;
	   std::vector<std::vector<int>> publishers;
	   std::unordered_map<std::string,int> table_roles;
  	   KeyValueStore *ks; 
	   data_server_client *ds;
	   std::unordered_map<std::string,int> client_role;
	   std::unordered_map<std::string,int> mcnum;
	   std::vector<message_cache *> mcs;
	   std::mutex m;
   public :

	   pubsubclient(int n,int p) : numprocs(n),myrank(p)
	   {  
		ks = new KeyValueStore(numprocs,myrank);
		ds = ks->get_rpc_client();
	   }


	   void create_pub_sub_service(std::string&,std::vector<int> &,std::vector<int> &);
	   void add_pubs(std::string &,std::vector<int>&);
	   void add_subs(std::string &,std::vector<int>&);
	   void remove_pubs(std::string &,std::vector<int>&);
	   void remove_subs(std::string &,std::vector<int>&);
	   void add_message_cache(std::string &,int,int);

	   ~pubsubclient()
	   {
		ks->close_sessions();
		delete ks;
		for(int i=0;i<mcs.size();i++)
		  delete mcs[i];
	   }
};


#endif
