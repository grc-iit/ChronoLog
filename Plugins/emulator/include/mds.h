#ifndef __MDS_H_
#define __MDS_H_

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
#include "block_map.h"
#include "city.h"
#include "Chronicle.h"
#include "ClientInfo.h"

struct stringhash
{
   uint64_t operator()(const std::string &s) const
   {
        return CityHash64(s.c_str(),s.size());
   }
};

template<typename B>
void add_story(Chronicle **c,B &&s)
{
    (*c)->add_story_to_chronicle(std::forward<B>(s));

}

template<typename B>
bool acquisition_count_zero(Chronicle **c, B&&n)
{
  if((*c)->get_acquisition_count(std::forward<B>(n))==0) return true;
  return false;
}

template<typename B>
void increment_acquisition_chronicle(Chronicle **c,B &&n)
{
   (*c)->increment_acquisition_count(std::forward<B>(n));
}

template<typename B>
void decrement_acquisition_chronicle(Chronicle **c,B &&n)
{
   (*c)->decrement_acquisition_count(std::forward<B>(n));
}

template<typename B>
void increment_acquisition_story(Chronicle **c,B &&b)
{

    (*c)->acquire_story(std::forward<B>(b));

}
template<typename B>
void decrement_acquisition_story(Chronicle **c,B &&b)
{

  (*c)->release_story(std::forward<B>(b));

}

namespace tl=thallium;

class metadata_server
{

  private:
	  tl::engine *thallium_server;
	  BlockMap<std::string,ClientInfo,stringhash> *client_table;
	  BlockMap<std::string,Chronicle*,stringhash> *metadata_table;
	  memory_pool<std::string,ClientInfo,stringhash> *pl1;
	  memory_pool<std::string,Chronicle*,stringhash> *pl2;

	  int numcores;
	  int numprocs;
	  int myrank;
	  std::string serveraddr;

   public:
	metadata_server(int np,int p,std::string &s,int n) : numprocs(np), myrank(p), serveraddr(s), numcores(n)
	{
	    thallium_server = new tl::engine(serveraddr.c_str(),THALLIUM_SERVER_MODE,true,numcores);
	    int tablesize = 1024;
            pl1 = new memory_pool<std::string,ClientInfo,stringhash> (128);
	    pl2 = new memory_pool<std::string,Chronicle*,stringhash> (128);	
	    std::string emptyKey="NULL";
	    client_table = new BlockMap<std::string,ClientInfo,stringhash> (tablesize,pl1,emptyKey);  
	    metadata_table = new BlockMap<std::string,Chronicle*,stringhash> (tablesize,pl2,emptyKey);
	}		
	~metadata_server()
	{
	    
	    thallium_server->finalize();
	    delete thallium_server;
	    delete pl1;
	    delete pl2;
	    delete client_table;
	    delete metadata_table;

	}

	void bind_functions()
	{
	    std::function<void(const tl::request &, std::string &)> connectFunc(
            std::bind(&metadata_server::ThalliumLocalConnect,
            this, std::placeholders::_1, std::placeholders::_2));

	    std::function<void(const tl::request &, std::string &)> disconnectFunc(
	    std::bind(&metadata_server::ThalliumLocalDisconnect,
	    this, std::placeholders::_1,std::placeholders::_2));

	    std::function<void(const tl::request &, std::string &,std::string &)> createchronicleFunc(
	    std::bind(&metadata_server::ThalliumLocalCreateChronicle,
	    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	    std::function<void(const tl::request &, std::string &, std::string &)> destroychronicleFunc(
	    std::bind(&metadata_server::ThalliumLocalDestroyChronicle,
	    this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

	    std::function<void(const tl::request &,std::string &,std::string &)> acquirechronicleFunc(
	    std::bind(&metadata_server::ThalliumLocalAcquireChronicle,
	    this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

	    std::function<void(const tl::request &,std::string &,std::string &)> releasechronicleFunc(
	    std::bind(&metadata_server::ThalliumLocalReleaseChronicle,
	    this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

	    std::function<void(const tl::request &,std::string &,std::string &,std::string &)> createstoryFunc(
	    std::bind(&metadata_server::ThalliumLocalCreateStory,
	    this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

	    std::function<void(const tl::request &,std::string &,std::string &,std::string &)> acquirestoryFunc(
	    std::bind(&metadata_server::ThalliumLocalAcquireStory,
	    this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

	    std::function<void(const tl::request &,std::string &,std::string &,std::string &)> releasestoryFunc(
	    std::bind(&metadata_server::ThalliumLocalReleaseStory,
	    this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

	    thallium_server->define("connect",connectFunc);
	    thallium_server->define("disconnect",disconnectFunc);
	    thallium_server->define("createchronicle",createchronicleFunc);
	    thallium_server->define("destroychronicle",destroychronicleFunc);
	    thallium_server->define("acquirechronicle",acquirechronicleFunc);
	    thallium_server->define("releasechronicle",releasechronicleFunc);
	    thallium_server->define("createstory",createstoryFunc);
	    thallium_server->define("acquirestory",acquirestoryFunc);
	    thallium_server->define("releasestory",releasestoryFunc);
	}

	bool LocalConnect(std::string &s)
	{
	    ClientInfo ci;
	    ci.setname(s);
	    uint32_t v = client_table->insert(s,ci);
	    if(v == INSERTED) return true;
	    else return false;
	}

	bool LocalDisconnect(std::string &s)
	{
	   bool b = client_table->erase(s);
	}

	bool LocalCreateChronicle(std::string &client_name, std::string &chronicle_name)
	{
	    Chronicle *c = new Chronicle();
	    c->setname(chronicle_name);

	    uint32_t v = metadata_table->insert(chronicle_name,c);
	    if(v == INSERTED) 
	    {
		  return true;
	    }
	    else return false;
	}

	bool LocalAcquireChronicle(std::string &client_name,std::string &chronicle_name)
	{
	     int a = 1;
	     bool b = metadata_table->update_field(chronicle_name,increment_acquisition_chronicle,a); 
	     return b;
	}

	bool LocalReleaseChronicle(std::string &client_name,std::string &chronicle_name)
	{
	    bool b = false; //metadata_table->update_field(chronicle_name,decrement_acquisition_chronicle);
	    return b;
	}
	bool LocalDestroyChronicle(std::string &client_name,std::string &chronicle_name)
	{
	   Chronicle *c;
	   bool b = metadata_table->get(chronicle_name,&c);
	   if(b)
	   {
		b = false; //metadata_table->erase_if(chronicle_name,acquisition_count_zero);
		if(b) 
		{
		   delete c;
		}
	   }	
	   return b;
	}

	bool LocalCreateStory(std::string &client_name,std::string &chronicle_name,std::string &story_name)
	{
	     bool b = metadata_table->update_field(chronicle_name,add_story,story_name);
	}
	bool LocalAcquireStory(std::string &client_name,std::string &chronicle_name,std::string &story_name)
	{
	     bool b = metadata_table->update_field(chronicle_name,increment_acquisition_story,story_name);
	}
	bool LocalReleaseStory(std::string &client_name,std::string &chronicle_name,std::string &story_name)
	{
	     bool b = metadata_table->update_field(chronicle_name,decrement_acquisition_story,story_name);
	}
	 void ThalliumLocalConnect(const tl::request &req, std::string &client_name)
  	{
        	req.respond(LocalConnect(client_name));
  	}

	void ThalliumLocalDisconnect(const tl::request &req, std::string &client_name)
	{
		req.respond(LocalDisconnect(client_name));
	}
	void ThalliumLocalCreateChronicle(const tl::request &req,std::string &client_name,std::string &chronicle_name)
	{
		req.respond(LocalCreateChronicle(client_name,chronicle_name));
	}
	void ThalliumLocalDestroyChronicle(const tl::request &req, std::string &client_name,std::string &chronicle_name)
	{
		req.respond(LocalDestroyChronicle(client_name,chronicle_name));
	}
	void ThalliumLocalAcquireChronicle(const tl::request &req,std::string &client_name,std::string &chronicle_name)
	{
		req.respond(LocalAcquireChronicle(client_name,chronicle_name));
	}
	void ThalliumLocalReleaseChronicle(const tl::request &req,std::string &client_name,std::string &chronicle_name)
	{
		req.respond(LocalReleaseChronicle(client_name,chronicle_name));
	}
	void ThalliumLocalCreateStory(const tl::request &req,std::string &client_name,std::string &chronicle_name,std::string &story_name)
	{
		req.respond(LocalCreateStory(client_name,chronicle_name,story_name));
	}
	void ThalliumLocalAcquireStory(const tl::request &req,std::string &client_name,std::string &chronicle_name,std::string &story_name)
	{
		req.respond(LocalAcquireStory(client_name,chronicle_name,story_name));
	}
	void ThalliumLocalReleaseStory(const tl::request &req,std::string &client_name,std::string &chronicle_name,std::string &story_name)
	{
		req.respond(LocalReleaseStory(client_name,chronicle_name,story_name));
	}
};

#endif
