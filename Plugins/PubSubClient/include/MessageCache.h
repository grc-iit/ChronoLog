#ifndef __MSG_CACHE_
#define __MSG_CACHE_

#include <mpi.h>
#include <unordered_map>
#include <vector>
#include <string>
#include "KeyValueStore.h"


class message_cache
{
     private : 

	     int numprocs;
	     int myrank;
	     std::string name;
	     int num_messages;
	     int msg_size;
	     std::vector<std::string> messages;
	     std::string rpc_prefix;
	     data_server_client *ds;    
    public :
	    message_cache(int n,int p,std::string s,data_server_client *d,int msize,int cache_size) : numprocs(n), myrank(p)
	    {
	        name = s;
		rpc_prefix = name;
		ds = d;
		msg_size = msize;
		num_messages = cache_size;




	    }

	    ~message_cache()
	    {


	    }


};


#endif

