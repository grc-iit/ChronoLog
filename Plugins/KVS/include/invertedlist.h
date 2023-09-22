#ifndef __INVERTED_LIST_H_
#define __INVERTED_LIST_H_

#include "blockmap.h"
#include <mpi.h>
#include <hdf5.h>
#include "h5_async_lib.h"
#include "event.h"
#include "data_server_client.h"
#include "KeyValueStoreIO.h"
#include "util_t.h"
#include <boost/lockfree/queue.hpp>
#include <typeinfo>

namespace tl=thallium;


template<class KeyT,class ValueT, class HashFcn=std::hash<KeyT>,class EqualFcn=std::equal_to<KeyT>>
struct invnode
{
   BlockMap<KeyT,ValueT,HashFcn,EqualFcn> *bm;
   memory_pool<KeyT,ValueT,HashFcn,EqualFcn> *ml; 
};

template <class KeyT>
struct KeyIndex
{
  KeyT key;
  uint64_t index;
};

struct keydata
{
  uint64_t ts;
  char *data;

};

template<class KeyT,class ValueT>
struct event_req
{
 KeyT key;
 std::vector<ValueT> values;
};

template<class KeyT>
bool compareIndex(struct KeyIndex<KeyT> &k1, struct KeyIndex<KeyT> &k2)
{
    if(k1.index < k2.index) return true;
    else return false;
}

template<class KeyT,class ValueT,class hashfcn=std::hash<KeyT>,class equalfcn=std::equal_to<KeyT>>
class hdf5_invlist
{

   private:
	   int numprocs;
	   int myrank;
	   struct invnode<KeyT,ValueT,hashfcn,equalfcn>* invlist;
	   int tag;
	   int totalsize;
	   int maxsize;
	   KeyT emptyKey;
	   std::string filename;
	   std::string attributename;
	   std::string rpc_prefix;
	   std::atomic<bool> file_exists;
	   data_server_client *d;
	   tl::engine *thallium_server;
           tl::engine *thallium_shm_server;
           tl::engine *thallium_client;
           tl::engine *thallium_shm_client;
           std::vector<tl::endpoint> serveraddrs;
           std::vector<std::string> ipaddrs;
           std::vector<std::string> shmaddrs;
           std::string myipaddr;
           std::string myhostname;
	   std::string dir;
           int nservers;
           int serverid;
	   int ntables;
	   KeyValueStoreIO *io_t;
           int nbits;
	   int nbits_p;
	   int nbits_r;
	   std::vector<int> cached_keyindex_mt;
	   std::vector<struct KeyIndex<KeyT>> cached_keyindex;
	   boost::lockfree::queue<struct event_req<KeyT,ValueT>*> *pending_gets;
	   int tables_per_proc;
	   int numtables;
	   std::vector<int> table_ids;
	   int pre_table;
	   int numevents;
	   int io_count;
	   boost::mutex invmutex;
	   int datasize;
	   hid_t datatype;
	   std::string respfile1;
	   std::ofstream ost;
	   std::string respfile2;
	   std::ofstream ost1;
	   int flush_count;
   public:
	   hdf5_invlist(int n,int p,int tsize,int np,KeyT emptykey,std::string &table,std::string &attr,data_server_client *ds,KeyValueStoreIO *io,int c,int data_size) : numprocs(n), myrank(p), io_count(c)
	   {
	     tag = 2000;
	     tag += io_count;
	     totalsize = tsize;
	     ntables = np;
	     int size = nearest_power_two(totalsize);
	     nbits = log2(size);
	     int nps = nearest_power_two(ntables);
	     nbits_p = log2(nps);
	     nbits_r = nbits-nbits_p; 
	     tables_per_proc = ntables/numprocs;
	     int rem = ntables%numprocs;
	     if(myrank < rem) numtables = tables_per_proc+1;
	     else numtables = tables_per_proc;
	     datasize = data_size;
	     if(myrank==0) std::cout <<" totalsize = "<<totalsize<<" number of tables = "<<ntables<<" totalbits = "<<nbits<<" nbits_per_table = "<<nbits_r<<" datasize = "<<datasize<<std::endl;

	     dir = "/home/asasidharan/FrontEnd/build/emu/"; 
	     maxsize = numtables*pow(2,nbits_r);

	     int prefix = 0;
	     for(int i=0;i<myrank;i++)
	     {	
		int nt = 0;
		if(i < rem) nt = tables_per_proc+1;
		else nt = tables_per_proc;
		prefix += nt;
	     }

	     pre_table = prefix;

	     for(int i=0;i<numtables;i++)
	     {
		table_ids.push_back(prefix+i);
	     }

	     flush_count = 0;
	     emptyKey = emptykey;
	     filename = table;
	     attributename = attr;
	     rpc_prefix = filename+attributename;
	     d = ds;
	     io_t = io;
	     file_exists.store(false);
	     respfile1 = filename+attributename+std::to_string(myrank);
	     ost = std::ofstream(respfile1.c_str(),std::ios_base::out); 
	     respfile2 = filename+attributename+std::to_string(myrank)+std::to_string(1);
	     ost1 = std::ofstream(respfile2.c_str(),std::ios_base::out);
	     tl::engine *t_server = d->get_thallium_server();
             tl::engine *t_server_shm = d->get_thallium_shm_server();
             tl::engine *t_client = d->get_thallium_client();
             tl::engine *t_client_shm = d->get_thallium_shm_client();
             std::vector<tl::endpoint> saddrs = d->get_serveraddrs();
             std::vector<std::string> ips = d->get_ipaddrs();
             std::vector<std::string> shm_addrs = d->get_shm_addrs();
	     nservers = numprocs;
	     serverid = myrank;
	     thallium_server = t_server;
             thallium_shm_server = t_server_shm;
             thallium_client = t_client;
             thallium_shm_client = t_client_shm;
             ipaddrs.assign(ips.begin(),ips.end());
             shmaddrs.assign(shm_addrs.begin(),shm_addrs.end());
             myipaddr = ipaddrs[serverid];
             serveraddrs.assign(saddrs.begin(),saddrs.end());
	     invlist = new struct invnode<KeyT,ValueT,hashfcn,equalfcn> ();
	     invlist->ml = new memory_pool<KeyT,ValueT,hashfcn,equalfcn> (100);
	     invlist->bm = new BlockMap<KeyT,ValueT,hashfcn,equalfcn>(size,invlist->ml,emptykey);
	     pending_gets = new boost::lockfree::queue<struct event_req<KeyT,ValueT>*> (128);
	   }

	   void bind_functions()
	   {
	       std::function<void(const tl::request &,KeyT &,ValueT&)> putEntryFunc(
               std::bind(&hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::ThalliumLocalPutEntry,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

	       std::function<void(const tl::request &,KeyT&)> getEntryFunc(
	       std::bind(&hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::ThalliumLocalGetEntry,this,std::placeholders::_1,std::placeholders::_2));		       

	       std::function<void(const tl::request &,KeyT &,std::vector<ValueT>&)> addPendingEvent(
	       std::bind(&hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::ThalliumAddPending,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

	       std::string fcnname1 = rpc_prefix+"RemotePutEntry";
               thallium_server->define(fcnname1.c_str(),putEntryFunc);
               thallium_shm_server->define(fcnname1.c_str(),putEntryFunc);

	       std::string fcnname2 = rpc_prefix+"RemoteGetEntry";
	       thallium_server->define(fcnname2.c_str(),getEntryFunc);
	       thallium_shm_server->define(fcnname2.c_str(),getEntryFunc);

	       std::string fcnname3 = rpc_prefix+"RemoteGetReq";
	       thallium_server->define(fcnname3.c_str(),addPendingEvent);
	       thallium_shm_server->define(fcnname3.c_str(),addPendingEvent);
	       
	       MPI_Request *reqs = (MPI_Request *)std::malloc(2*numprocs*sizeof(MPI_Request));
	       int nreq = 0;
	       int send_v = 1;
	       std::vector<int> recv_v(numprocs);
	       std::fill(recv_v.begin(),recv_v.end(),0);
	       
	       for(int i=0;i<numprocs;i++)
	       {
		  MPI_Isend(&send_v,1,MPI_INT,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
		  nreq++;
		  MPI_Irecv(&recv_v[i],1,MPI_INT,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
		  nreq++;
	       }

	       MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

	       std::free(reqs);

	   }

	   ~hdf5_invlist()
	   {
	        if(invlist != nullptr) 
	        {
	          delete invlist->bm;
		  delete invlist->ml;
		  delete invlist;
		  delete pending_gets;
	         }
		if(ost.is_open()) ost.close();
		if(ost1.is_open()) ost1.close();
	   }
	  
	   void add_event_file(std::string &eventstring)
	   {
		if(ost1.is_open())
		{
		   ost1 << eventstring << std::endl;
		}

	   }
	   bool LocalPutEntry(KeyT &k,ValueT& v)
	   {
		bool b = false;
		int ret = invlist->bm->insert(k,v);
		if(ret==INSERTED) b = true;
		return b;
	   }

	   std::vector<ValueT> LocalGetEntry(KeyT &k)
	   {
		bool b;
		std::vector<ValueT> values;
		b = invlist->bm->getvalues(k,values);
		return values;
	   }

	   void ThalliumLocalPutEntry(const tl::request &req,KeyT &k,ValueT &v)
           {
                req.respond(LocalPutEntry(k,v));
           }

	   void ThalliumLocalGetEntry(const tl::request &req,KeyT &k)
	   {
		req.respond(LocalGetEntry(k));
	   }

	   bool PutEntry(KeyT &k,ValueT &v,int destid)
	   {
              if(ipaddrs[destid].compare(myipaddr)==0)
              {
                    tl::endpoint ep = thallium_shm_client->lookup(shmaddrs[destid]);
		    std::string fcnname = rpc_prefix+"RemotePutEntry";
                    tl::remote_procedure rp = thallium_shm_client->define(fcnname.c_str());
                    return rp.on(ep)(k,v);
              }
              else
              {
		    std::string fcnname = rpc_prefix+"RemotePutEntry";
                    tl::remote_procedure rp = thallium_client->define(fcnname.c_str());
                    return rp.on(serveraddrs[destid])(k,v);
              }
	   }

	   std::vector<ValueT> GetEntry(KeyT &k,int destid)
	   {
		if(ipaddrs[destid].compare(myipaddr)==0)
		{
		    tl::endpoint ep = thallium_shm_client->lookup(shmaddrs[destid]);
		    std::string fcnname = rpc_prefix+"RemoteGetEntry";
		    tl::remote_procedure rp = thallium_shm_client->define(fcnname.c_str());
		    return rp.on(ep)(k);
		}
		else
		{
		   std::string fcnname = rpc_prefix+"RemoteGetEntry";
		   tl::remote_procedure rp = thallium_client->define(fcnname.c_str());
		   return rp.on(serveraddrs[destid])(k);
		}
	   }
	   bool AddPending(KeyT &key,std::vector<ValueT> &values,int destid)
	   {
		if(ipaddrs[destid].compare(myipaddr)==0)
		{
		   tl::endpoint ep = thallium_shm_client->lookup(shmaddrs[destid]);
		   std::string fcnname = rpc_prefix+"RemoteGetReq";
		   tl::remote_procedure rp = thallium_shm_client->define(fcnname.c_str());
		   return rp.on(ep)(key,values);
		}
		else
		{
		   std::string fcnname = rpc_prefix+"RemoteGetReq";
		   tl::remote_procedure rp = thallium_client->define(fcnname.c_str());
		   return rp.on(serveraddrs[destid])(key,values);
		}
	   }
	   bool CheckLocalFileExists()
	   {
		   return file_exists.load();
	   }

	   void LocalFileExists()
	   {
		file_exists.store(true);
	   }

	   bool add_pending(KeyT &key,std::vector<ValueT> &values)
	   {
		struct event_req<KeyT,ValueT> *q = new struct event_req<KeyT,ValueT>();
		q->key = key;
		q->values.assign(values.begin(),values.end());
		return pending_gets->push(q);
	   }
	   void ThalliumAddPending(const tl::request &req,KeyT &key,std::vector<ValueT> &values)
	   {
		req.respond(add_pending(key,values));
	   }

	   std::vector<struct keydata> get_events();
	   void create_async_io_request(KeyT &,std::vector<ValueT>&);
	   void create_sync_io_request();
	   bool put_entry(KeyT&,ValueT&);
	   int get_entry(KeyT&,std::vector<ValueT>&);
	   void fill_invlist_from_file(std::string&,int);
	   void flush_table_file(int,bool);
	   int partition_no(KeyT &k);	
           void cache_latest_table();	   
	   void add_entries_to_tables(std::string&,std::vector<struct keydata>*,uint64_t,int); 
	   void get_entries_from_tables(std::vector<struct KeyIndex<KeyT>> &,int&,int&,uint64_t);
	   std::vector<struct KeyIndex<KeyT>> merge_keyoffsets(std::vector<struct KeyIndex<KeyT>>&,std::vector<struct KeyIndex<KeyT>>&,std::vector<int>&);

	   void create_index_file();
	   void update_index_file();
};

#include "../srcs/invertedlist.cpp"

#endif
