#ifndef __KeyValueStoreIO_H_
#define __KeyValueStoreIO_H_

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
#include <boost/lockfree/queue.hpp>
#include "event.h"
#include <thread>
#include <mpi.h>

struct request
{
  std::string name;
  std::string attr_name;
  int id;
  int keytype;
  int intkey;
  uint64_t unsignedlongkey;
  float floatkey;
  double doublekey;
  int sender;
  bool flush;
  bool persist;
};

struct sync_request
{
   void *funcptr;
   std::string name;
   std::string attr_name; 
   int keytype;
   int offset;
   bool flush;
   bool fill;
   bool persist;
};

struct response
{
  std::string name;
  std::string attr_name;
  int id;
  int response_id;
  std::string event;
  int sender;
  bool complete;
};

struct thread_arg 
{
  int tid;

};

namespace tl=thallium;

template<typename A>
void serialize(A &ar,struct request &e)
{
	ar & e.name;
	ar & e.attr_name;
        ar & e.id;
        ar & e.keytype;
	ar & e.intkey;
	ar & e.unsignedlongkey;
	ar & e.floatkey;
	ar & e.doublekey;
	ar & e.sender;
	ar & e.flush;
}	

template<typename A>
void serialize(A &ar,struct response &e)
{
   ar & e.name;
   ar & e.attr_name;
   ar & e.id;
   ar & e.response_id;
   ar & e.event;
   ar & e.sender;
   ar & e.complete;
}


class KeyValueStoreIO
{

   private: 
	   int nservers;
	   int serverid;
	   boost::lockfree::queue<struct request*> *req_queue;
	   boost::lockfree::queue<struct response*> *resp_queue;
	   boost::lockfree::queue<struct sync_request*> *sync_queue;
	   tl::engine *thallium_server;
           tl::engine *thallium_shm_server;
           tl::engine *thallium_client;
           tl::engine *thallium_shm_client;
           std::vector<tl::endpoint> serveraddrs;
           std::vector<std::string> ipaddrs;
           std::vector<std::string> shmaddrs;
           std::string myipaddr;
           std::string myhostname;
	   int num_io_threads;
	   std::vector<std::pair<int,void*>> service_queries;
	   std::vector<std::thread> io_threads;
	   std::vector<struct thread_arg> t_args;
	   std::atomic<int> request_count;
	   std::atomic<uint32_t> synchronization_word;
	   boost::mutex mutex_t;
	   boost::condition_variable cv;
	   int semaphore;

   public:

	    KeyValueStoreIO(int np,int p) : nservers(np), serverid(p)
	    {
	        num_io_threads = 1;
		semaphore = 0;
		request_count.store(0);
		synchronization_word.store(0);
		req_queue = new boost::lockfree::queue<struct request*> (128);
		resp_queue = new boost::lockfree::queue<struct response*> (128);
		sync_queue = new boost::lockfree::queue<struct sync_request*> (128);

		 t_args.resize(num_io_threads);
	 	 for(int i=0;i<num_io_threads;i++) t_args[i].tid = i;	 
	
		 io_threads.resize(num_io_threads);

		 std::function<void(struct thread_arg *)> IOFunc(
                 std::bind(&KeyValueStoreIO::io_function,this, std::placeholders::_1));
			
		 std::thread t{IOFunc,&t_args[0]};
		 io_threads[0] = std::move(t);

	    }

	     void end_io()
	     {
		struct sync_request *r = new sync_request();
		r->name = "ZZZZ";
		r->attr_name = "ZZZZ";
		r->keytype = -1;
		r->funcptr = nullptr;

		sync_queue->push(r);

		for(int i=0;i<num_io_threads;i++) io_threads[i].join();

		
	     }
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

	    uint32_t read_sync_word()
	    {
		return synchronization_word.load();
	    }

	    void add_query_service(int type,void *fptr)
	    {
		std::pair<int,void*> p;
		p.first = type;
		p.second = fptr;
		service_queries.push_back(p);
	    }

	    void write_sync_word(uint32_t n)
	    {
		synchronization_word.store(n);
	    }

	    bool announce_sync(int p)
	    {
		uint32_t mask = 1;
		mask = mask << p;
		bool b = false;
		uint32_t mask_s = 1;
		mask_s = mask_s << 31;

		uint32_t prev = synchronization_word.load();
	        uint32_t next;

		do
		{
		    b = false;
		    prev = synchronization_word.load();
		    uint32_t m_p = prev & mask_s;
		    uint32_t m_q = prev & mask;
		    m_q = m_q >> p;
		    m_p = m_p >> 31;
		    if(m_p == 1 || m_q == 1) break;
		    next = prev | mask;
		}while(!(b = synchronization_word.compare_exchange_strong(prev,next)));

		return b;
	    }

	    bool reset_sync(int p)
	    {
		uint32_t mask = UINT32_MAX;
		uint32_t mask1 = 1;
		mask1 = mask1 << p;
		mask = mask1 ^ mask;

		bool b = false;

		uint32_t prev = synchronization_word.load();
		uint32_t next;

		do
		{
		   b = false;
		   prev = synchronization_word.load();
		   next = prev & mask;
		}while(!(b = synchronization_word.compare_exchange_strong(prev,next)));
		
		return b;
	    }

	     void bind_functions()
	     {
	       std::function<void(const tl::request &,struct request &)> putRequestFunc(
               std::bind(&KeyValueStoreIO::ThalliumLocalPutRequest,this,std::placeholders::_1,std::placeholders::_2));

	       std::function<void(const tl::request &,struct response &)> putResponseFunc(
               std::bind(&KeyValueStoreIO::ThalliumLocalPutResponse,this,std::placeholders::_1,std::placeholders::_2));
               
	       thallium_server->define("RemotePutIORequest",putRequestFunc);
               thallium_shm_server->define("RemotePutIORequest",putRequestFunc);

               thallium_server->define("RemotePutIOResponse",putResponseFunc);
               thallium_shm_server->define("RemotePutIOResponse",putResponseFunc);

	     }

	     bool LocalPutSyncRequest(struct sync_request *r)
	     {
                sync_queue->push(r);
                return true;
	     }
	     bool LocalPutRequest(struct request &r)
	     {
		struct request *s = new struct request();
		s->name = r.name;
		s->id = request_count.fetch_add(1);
		s->keytype = r.keytype;
  		s->intkey = r.intkey;
  		s->floatkey = r.floatkey;
  		s->doublekey = r.doublekey;
  		s->sender = r.sender;
  		s->flush = r.flush;
		
		req_queue->push(s);
		return true;
	     }

	     bool LocalPutResponse(struct response &r)
	     {
		struct response *s = new struct response();
		s->name = r.name;
  		s->id = r.id;
  		s->response_id = r.response_id;
  		s->event = r.event;
  		s->sender = r.sender;
  		s->complete = r.complete;
		
		resp_queue->push(s);
		return true;
	     }

	     void ThalliumLocalPutRequest(const tl::request &req,struct request &r)
	     {
		req.respond(LocalPutRequest(r));
	     }

	     void ThalliumLocalPutResponse(const tl::request &req,struct response &r)
	     {
		req.respond(LocalPutResponse(r));
	     }

	     struct request *GetRequest()
	     {
		
		struct request *r=nullptr;
		bool b = req_queue->pop(r);
		return r;
	     }
	     struct response *GetResponse()
	     {
		struct response *r=nullptr;
		bool b = resp_queue->pop(r);
		return r;
	     }

	     struct sync_request *GetSyncRequest()
	     {
		struct sync_request *r=nullptr;
		bool b = sync_queue->pop(r);
		return r;
	     }

	     bool RequestQueueEmpty()
	     {
		return req_queue->empty();
	     }
		
	     bool ResponseQueueEmpty()
	     {
		return resp_queue->empty();
	     }

	     bool SyncRequestQueueEmpty()
	     {
		return sync_queue->empty();
	     }

	     bool PutRequest(struct request &r,int destid)
	     {
		if(ipaddrs[destid].compare(myipaddr)==0)
                {
                    tl::endpoint ep = thallium_shm_client->lookup(shmaddrs[destid]);
                    tl::remote_procedure rp = thallium_shm_client->define("RemotePutIORequest");
                    return rp.on(ep)(r);
                }
                else
                {
                    tl::remote_procedure rp = thallium_client->define("RemotePutIORequest");
                    return rp.on(serveraddrs[destid])(r);
                }
	     }
	     bool PutResponse(struct response &r,int destid)
	     {

		if(ipaddrs[destid].compare(myipaddr)==0)
                {
                    tl::endpoint ep = thallium_shm_client->lookup(shmaddrs[destid]);
                    tl::remote_procedure rp = thallium_shm_client->define("RemotePutIOResponse");
                    return rp.on(ep)(r);
                }
                else
                {
                    tl::remote_procedure rp = thallium_client->define("RemotePutIOResponse");
                    return rp.on(serveraddrs[destid])(r);
                }
	     }

	     void get_common_requests(std::vector<struct sync_request*>&,std::vector<struct sync_request*>&);
	     void io_function(struct thread_arg *);

	    ~KeyValueStoreIO()
	    {
		delete req_queue;
		delete resp_queue;
		delete sync_queue;
	    }









};


#endif
