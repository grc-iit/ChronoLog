#ifndef __KeyValueStore_H_
#define __KeyValueStore_H_

#include "KeyValueStoreMetadata.h"
#include "KeyValueStoreAccessorRepository.h"
#include "KeyValueStoreMDS.h"
#include "KeyValueStoreIO.h"
#include "data_server_client.h"
#include "util_t.h"
#include <hdf5.h>
#include <thread>
#include <mutex>
#include "Interface_Queues.h"

struct kstream_args
{
  std::string tname;
  std::string attr_name;
  int maxsize;
  int tid;
};

class KeyValueStore
{
    private:
	    int numprocs;
	    int myrank;
	    KeyValueStoreMDS *mds;
	    data_server_client *ds;
	    KeyValueStoreIO *io_layer;
	    Interface_Queues *if_q;
	    KeyValueStoreAccessorRepository *tables;
	    int io_count;
	    int tag;
	    std::unordered_map<std::string,int> kvindex;
	    std::vector<struct kstream_args> k_args;
	    std::vector<std::thread> kstreams;
	    std::atomic<int> *stream_flags;
	    std::atomic<int> nstreams;

    public:
	    KeyValueStore(int np,int r) : numprocs(np), myrank(r)
	   {
		H5open();
   	        //H5VLis_connector_registered_by_name("async");
		io_count=0;
		int base_port = 2000;
		tag = 20000;
		nstreams.store(0);
		ds = new data_server_client(numprocs,myrank,base_port);
		mds = new KeyValueStoreMDS(numprocs,myrank);
		tl::engine *t_server = ds->get_thallium_server();
           	tl::engine *t_server_shm = ds->get_thallium_shm_server();
           	tl::engine *t_client = ds->get_thallium_client();
           	tl::engine *t_client_shm = ds->get_thallium_shm_client();
           	std::vector<tl::endpoint> server_addrs = ds->get_serveraddrs();
           	std::vector<std::string> ipaddrs = ds->get_ipaddrs();
           	std::vector<std::string> shmaddrs = ds->get_shm_addrs();
           	mds->server_client_addrs(t_server,t_client,t_server_shm,t_client_shm,ipaddrs,shmaddrs,server_addrs);
		mds->bind_functions();
		io_layer = new KeyValueStoreIO(numprocs,myrank);
		io_layer->server_client_addrs(t_server,t_client,t_server_shm,t_client_shm,ipaddrs,shmaddrs,server_addrs);
		io_layer->bind_functions();
		if_q = new Interface_Queues(numprocs,myrank);
		if_q->server_client_addrs(t_server,t_client,t_server_shm,t_client_shm,ipaddrs,shmaddrs,server_addrs);
		std::string remfilename = "emulatoraddrs";
		if_q->get_remote_addrs(remfilename);
		if_q->bind_functions();
		MPI_Barrier(MPI_COMM_WORLD);
		bool b = if_q->PutKVSAddresses(myrank);
		tables = new KeyValueStoreAccessorRepository(numprocs,myrank,io_layer,if_q,ds);
		k_args.resize(MAXSTREAMS);
	        kstreams.resize(MAXSTREAMS);
		stream_flags = (std::atomic<int>*)std::malloc(MAXSTREAMS*sizeof(std::atomic<int>));
	   }
	   void createKeyValueStoreEntry(std::string &,KeyValueStoreMetadata &);
	   bool findKeyValueStoreEntry(std::string &,KeyValueStoreMetadata &);
	   void removeKeyValueStoreEntry(std::string &s);
	   void addKeyValueStoreInvList(std::string &s,std::string &attr_name,int);
	   bool findKeyValueStoreInvList(std::string &s,std::string &attr_name);
	   void removeKeyValueStoreInvList(std::string &s,std::string &attr_name);

	   /*template<typename T,typename N>
	   void PutGetPerf(struct kstream_args<N>*k)
	   {
		KeyValueStoreAccessor* ka = tables->get_accessor(k->tname);
		int pos = ka->get_inverted_list_index(k->attr_name);
		int nthreads = 4;
		int totalkeys = k->keys.size();
		int mywork = totalkeys/nthreads;
		int rem = totalkeys%nthreads;

		int start=0,end=0;
		for(int i=0;i<k->tid;i++)
		{
			if(i < rem) start += mywork+1;
			else start += mywork;
		}
		
		if(k->tid < rem) end = start+mywork+1;
		else end = start+mywork;

		for(int i=start;i<end;i++)
                {
                        N key = k->keys[i];
                        uint64_t ts_k = k->ts[i];
                        ka->insert_entry<T,N>(pos,key,ts_k);
                }

                 for(int i=start;i<end;i++)
                 {
                   std::vector<uint64_t> values = ka->get_entry<T,N>(pos,k->keys[i]);
                 }
	   }*/

	   template<typename T,typename N>
	   void cacheflushInvList(struct kstream_args*k)
	   {
    		std::string s = k->tname;
    		std::string attr_name = k->attr_name;
    		KeyValueStoreAccessor* ka = tables->get_accessor(s);

    		int pos = ka->get_inverted_list_index(attr_name);

    		std::string type = ka->get_attribute_type(attr_name);

    		int tag = k->tid;

    		int send_v = 1;
    		std::vector<int> recv_v(numprocs);
    		std::fill(recv_v.begin(),recv_v.end(),0);

    		int nreq = 0;
   	 	MPI_Request *reqs = new MPI_Request[2*numprocs];

		bool end_loop = false;
		int rate = 200;
		int request_count=0;

		while(true)
   		{
      		  send_v = stream_flags[k->tid].load();
      		  std::fill(recv_v.begin(),recv_v.end(),0);
      		  nreq = 0;
      		  for(int i=0;i<numprocs;i++)
      		  {
        	     MPI_Isend(&send_v,1,MPI_INT,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
        	     nreq++;
        	     MPI_Irecv(&recv_v[i],1,MPI_INT,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
        	     nreq++;
      		  }

      		  MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

      		  int sum = 0;
      		  for(int i=0;i<numprocs;i++) sum += recv_v[i];

      		  if(sum==numprocs) 
		  {
		     end_loop = true;
		     rate = 100;
		  }

      		  auto t1 = std::chrono::high_resolution_clock::now();
                  while(true)
                  {
        		auto t2 = std::chrono::high_resolution_clock::now();

        		double t = std::chrono::duration<double>(t2-t1).count();
        		if(t > rate) break;
      		  }
		
		  if(end_loop) 
		  ka->flush_invertedlist<T>(attr_name,true);
		  else 
		  {
		     bool c = false;
		     if(request_count/5==0) c = true;
		     ka->flush_invertedlist<T>(attr_name,c);
		  }
		  if(end_loop) break;
		  request_count++;
   	       }

   	       delete reqs;
          }



	   template<typename T,typename N>
           void RunKeyValueStoreFunctions(KeyValueStoreAccessor* ka,struct kstream_args *k)
	   {
		ka->cache_invertedtable<T>(k->attr_name);

   		int pos = ka->get_inverted_list_index(k->attr_name);

		/*std::function<void(struct kstream_args<N> *)>
                PutGetWorkload(std::bind(&KeyValueStore::PutGetPerf<T,N>,this, std::placeholders::_1));

		int nthreads = 4;
		std::vector<struct kstream_args<N>> k_args_w(nthreads);

		for(int i=0;i<nthreads;i++)
		{
		    k_args_w[i].tname = k->tname;
                    k_args_w[i].attr_name = k->attr_name;
                    k_args_w[i].tid = i;
                    k_args_w[i].keys.assign(k->keys.begin(),k->keys.end());
                    k_args_w[i].ts.assign(k->ts.begin(),k->ts.end());

		}

		std::vector<std::thread> workers(nthreads);

		for(int i=0;i<nthreads;i++)
		{
		   std::thread t{PutGetWorkload,&k_args_w[i]};
		   workers[i] = std::move(t);
		}

		for(int i=0;i<nthreads;i++) workers[i].join();*/

		auto t1 = std::chrono::high_resolution_clock::now();

		/*
    		for(int i=0;i<k->keys.size();i++)
    		{
		        if(k->op[i]==0)
			{
        		  N key = k->keys[i];
        		  uint64_t ts_k = k->ts[i];
        		  ka->insert_entry<T,N>(pos,key,ts_k);
			}
    		}*/

		auto t2 = std::chrono::high_resolution_clock::now();

		double pt = std::chrono::duration<double>(t2-t1).count();

		t1 = std::chrono::high_resolution_clock::now();

		 /*for(int i=0;i<k->keys.size();i++)
    		 {
		   if(k->op[i]==0 && i%100==0)
		   {
      		      std::vector<uint64_t> values = ka->get_entry<T,N>(pos,k->keys[i]);
		   }
    		 }*/

		 t2 = std::chrono::high_resolution_clock::now();

		 double gt = std::chrono::duration<double>(t2-t1).count();

		 t1 = std::chrono::high_resolution_clock::now();

		 //ka->flush_invertedlist<T>(k->attr_name);

		 t2 = std::chrono::high_resolution_clock::now();

		 double ft = std::chrono::duration<double>(t2-t1).count();
		 
		 MPI_Request *reqs = (MPI_Request *)std::malloc(2*numprocs*sizeof(MPI_Request));
		 int nreq = 0;

		 std::vector<double> send_v(3);
		 send_v[0] = pt; send_v[1] = gt; send_v[2] = ft;
		 std::vector<double> recvv(3*numprocs);
		 std::fill(recvv.begin(),recvv.end(),0);

		 int tag_m = tag+k->tid;

		 for(int i=0;i<numprocs;i++)
		 {
		    MPI_Isend(send_v.data(),3,MPI_DOUBLE,i,tag_m,MPI_COMM_WORLD,&reqs[nreq]);
		    nreq++;
		    MPI_Irecv(&recvv[3*i],3,MPI_DOUBLE,i,tag_m,MPI_COMM_WORLD,&reqs[nreq]);
		    nreq++;
		 }

		 MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

		 double max_time_1 = 0, max_time_2 = 0, max_time_3 = 0;

		 for(int i=0;i<numprocs;i++)
		 {	
		    if(recvv[3*i] > max_time_1) max_time_1 = recvv[3*i];
		    if(recvv[3*i+1] > max_time_2) max_time_2 = recvv[3*i+1];
		    if(recvv[3*i+2] > max_time_3) max_time_3 = recvv[3*i+2];
		 }

		 if(myrank==0) std::cout <<" ptime = "<<max_time_1<<" gtime = "<<max_time_2<<" ftime = "<<max_time_3<<std::endl;

		 std::free(reqs);
	   }

	   template<typename T,typename N>
           void prepare_inverted_list(struct kstream_args *k)
	   {
		std::string s = k->tname;
                std::string attr_name = k->attr_name;
                KeyValueStoreAccessor* ka = tables->get_accessor(s);
		int maxsize = k->maxsize;

                if(ka==nullptr)
                {
                   KeyValueStoreMetadata m;
                   if(!findKeyValueStoreEntry(s,m)) return;
                   if(!tables->add_accessor(s,m)) return;
                   ka = tables->get_accessor(s);
                }

              int pos = ka->get_inverted_list_index(attr_name);

               if(pos==-1)
               {
                  tables->create_invertedlist(s,attr_name,io_count,maxsize);
                  io_count++;
                  pos = ka->get_inverted_list_index(attr_name);
               }

               std::string type = ka->get_attribute_type(attr_name);
	       KeyValueStoreMetadata m = ka->get_metadata();
	       std::vector<std::string> metastring;
	       m.packmetadata(metastring);
		
	       bool b = if_q->CreateEmulatorStream(s,metastring,myrank);
	   }

	   template<typename T,typename N>
	   void create_keyvalues(int s_id,std::vector<N> &keys,std::vector<std::string> &values,std::vector<int> &ops,int rate)
	   {
		std::string s = k_args[s_id].tname;
		std::string attr_name = k_args[s_id].attr_name;
		KeyValueStoreAccessor *ka = tables->get_accessor(s);
		bool b = false;
		int pos = ka->get_inverted_list_index(attr_name);
		std::string st = k_args[s_id].tname;
		KeyValueStoreMetadata m = ka->get_metadata();	
		int datasize = m.value_size();

		for(int i=0;i<keys.size();i++)
		{
		   if(ops[i]==0)
		   {
		     std::string data;
		     data.resize(sizeof(N)+values[i].length());
		     char *key = (char *)(&keys[i]);
	     	     for(int j=0;j<sizeof(N);j++)
			data[i] = key[j];
		     for(int j=0;j<values[i].length();j++)
			data[sizeof(N)+j] = values[i][j];	     
			
		     if(ka->Put<T,N,std::string>(pos,st,keys[i],data))
		     {


		     }
		     usleep(rate);
			
		   }

		}

	   }

	   template<typename T,typename N,typename M>
	   void create_keyvalues(int s_id,std::vector<N> &keys,std::vector<M> &values,std::vector<int> &ops,int rate)
	   {
	      std::string s = k_args[s_id].tname;
	      std::string attr_name = k_args[s_id].attr_name;
	      KeyValueStoreAccessor *ka = tables->get_accessor(s);
	      bool b = false;
	      int pos = ka->get_inverted_list_index(attr_name);
	      std::string st = k_args[s_id].tname;
	      KeyValueStoreMetadata m = ka->get_metadata();
	      int datasize = m.value_size();
	      
	      for(int i=0;i<keys.size();i++)
	      {
		if(ops[i]==0)
		{
	          std::string data;
	          data.resize(datasize);
		  char *key = (char*)(&(keys[i]));
		  char *value = (char*)(&(values[i]));
		  for(int j=0;j<sizeof(N);j++)
		    data[j] = key[j];
	          for(int j=0;j<sizeof(M);j++)
		    data[sizeof(N)+j] = value[j];	 
		  if(ka->Put<T,N,std::string>(pos,st,keys[i],data))
		  {


		  }
		
		  usleep(rate);
		}


	      }

	   }
	   template<typename T,typename N>
	   void create_keyvalues(int s_id,int nops,int rate)
	   {
		std::string s = k_args[s_id].tname;
   		std::string attr_name = k_args[s_id].attr_name;
   		KeyValueStoreAccessor* ka = tables->get_accessor(s);

		bool b = false;
		int pos = ka->get_inverted_list_index(attr_name);
		std::string st = k_args[s_id].tname;
		std::string data;
		KeyValueStoreMetadata m = ka->get_metadata();
		int datasize = m.value_size();
		data.resize(datasize);
		
		bool exit = false;
		int op = 0;
		N prevkey=0;
		for(int i=0;i<nops;i++)
		{	
		    N key = random()%RAND_MAX; 
		    op = random()%2;
		    if(op==0)
		    { 
		      if(!ka->Put<T,N,std::string>(pos,st,key,data))
		      {
		      }
		      prevkey = key;
		    }
		    else if(prevkey != 0) 
		    {
		      key = prevkey;
		      b = ka->Get<T,N> (pos,st,key);
		    }

		    usleep(rate); 
		}
	   }

           void get_testworkload(std::string &,std::vector<int>&,std::vector<uint64_t>&,int);
           void get_ycsb_timeseries_workload(std::string&,std::vector<uint64_t>&,std::vector<float>&,std::vector<int>&);
           void get_dataworld_workload(std::string&,std::vector<uint64_t>&,std::vector<uint64_t>&,std::vector<int>&);
           void get_ycsb_test(std::string&,std::vector<uint64_t>&,std::vector<std::string>&);

	   template<typename T,typename N>
	   int spawn_kvstream(std::string &s,std::string &a,int maxsize)
	   {

		int prev = nstreams.fetch_add(1);
		k_args[prev].tname = s;
		k_args[prev].attr_name = a;
		k_args[prev].tid = prev;
		k_args[prev].maxsize = maxsize;
		std::string streamname = s+a;
		std::pair<std::string,int> p(streamname,prev);
		kvindex.insert(p);
		stream_flags[prev].store(0);

		prepare_inverted_list<T,N>(&k_args[prev]);

		std::function<void(struct kstream_args *)> 
		KVStream(std::bind(&KeyValueStore::cacheflushInvList<T,N>,this, std::placeholders::_1));
	
	 	std::thread t{KVStream,&k_args[prev]};	
		kstreams[prev] = std::move(t);

		return prev;
	   }

	   int start_session(std::string &name,std::string &attrname,KeyValueStoreMetadata &m,int maxsize)
	   {
		
		 createKeyValueStoreEntry(name,m);
		 addKeyValueStoreInvList(name,attrname,maxsize);
		 std::string type = m.get_type(attrname);
		 int session_index = -1;
		 if(type.compare("int")==0)
		 {
		   session_index = spawn_kvstream<integer_invlist,int>(name,attrname,maxsize);
		 }
		 else if(type.compare("unsignedlong")==0)
		 {
		   session_index = spawn_kvstream<unsigned_long_invlist,unsigned long>(name,attrname,maxsize);
		 }
		 else if(type.compare("float")==0)
		 {
		   session_index = spawn_kvstream<float_invlist,float>(name,attrname,maxsize);
		 }
		 else if(type.compare("double")==0)
		 {
		    session_index = spawn_kvstream<double_invlist,double>(name,attrname,maxsize);
		 }
		 return session_index;

	   }

	   void close_sessions()
	   {
		std::string s = "endsession";
		bool b = if_q->EndEmulatorSession(s,myrank);

		for(int i=0;i<nstreams.load();i++) stream_flags[i].store(1);
		for(int i=0;i<nstreams.load();i++) kstreams[i].join();

		io_layer->end_io();

		s = "shutdown";
		b = if_q->ShutDownEmulator(s,myrank);

	   }
	   ~KeyValueStore()
	   {

		delete tables;
		delete io_layer;
		delete mds;
		delete ds;
		delete if_q;
		std::free(stream_flags);
		H5close();

	   }




};



#endif
