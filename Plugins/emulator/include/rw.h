#ifndef __RW_H_
#define __RW_H_

#include <abt.h>
#include <mpi.h>
#include "ClockSync.h"
#include <hdf5.h>
#include <boost/container_hash/hash.hpp>
#include "write_buffer.h"
#include "distributed_sort.h"
#include "data_server_client.h"
#include "event_metadata.h"
#include "nvme_buffer.h"
#include <boost/lockfree/queue.hpp>
//#include "h5_async_lib.h"
#include <thread>

using namespace boost;

struct thread_arg_w
{
  int tid;
  int num_events;
  bool endsession;
  std::string name;
};


struct io_request
{
   std::string name;
   bool from_nvme;
   bool read_op;
   std::vector<struct event> *buf1;
   std::vector<struct event> *buf2;
   std::vector<struct event> *buf3;
   std::atomic<int> completed;
   bool for_query;
   hsize_t total_records;
};


class read_write_process
{

private:
      ClockSynchronization<ClocksourceCPPStyle> *CM;
      int myrank;
      int numprocs;
      int numcores;
      boost::hash<uint64_t> hasher;
      uint64_t seed = 1;
      databuffers *dm;
      nvme_buffers *nm;
      boost::mutex m1;
      boost::mutex m2;
      std::set<std::string> file_names;
      std::unordered_map<std::string,std::pair<int,event_metadata>> write_names;
      std::unordered_map<std::string,std::pair<int,event_metadata>> read_names;
      std::vector<std::pair<std::atomic<uint64_t>,std::atomic<uint64_t>>> *file_interval;
      std::vector<std::pair<std::atomic<uint64_t>,std::atomic<uint64_t>>> *write_interval;
      std::vector<std::pair<std::atomic<uint64_t>,std::atomic<uint64_t>>> *read_interval;
      std::vector<struct atomic_buffer*> myevents;
      std::vector<struct atomic_buffer*> readevents;
      dsort *ds;
      data_server_client *dsc;
      std::vector<struct thread_arg_w> t_args;
      std::vector<std::thread> workers;    
      boost::lockfree::queue<struct io_request*> *io_queue_async;
      boost::lockfree::queue<struct io_request*> *io_queue_sync;
      std::atomic<int> end_of_session;
      std::atomic<int> end_of_io_session;
      std::atomic<int> num_streams;
      int num_io_threads;
      std::vector<struct thread_arg_w> t_args_io;
      std::vector<std::thread> io_threads;
      std::atomic<int> sync_clock;
      std::vector<int> num_dropped;
      std::vector<int> batch_size;
      tl::engine *thallium_server;
      tl::engine *thallium_shm_server;
      tl::engine *thallium_client;
      tl::engine *thallium_shm_client;
      std::vector<tl::endpoint> serveraddrs;
      std::vector<std::string> ipaddrs;
      std::vector<std::string> shmaddrs;
      std::string myipaddr;
      int iters_per_batch;
public:
	read_write_process(int r,int np,ClockSynchronization<ClocksourceCPPStyle> *C,int n,data_server_client *rc) : myrank(r), numprocs(np), numcores(n), dsc(rc)
	{
           H5open();
	   //H5VLis_connector_registered_by_name("async");
           std::string unit = "microsecond";
	   CM = C;
	   sync_clock.store(0);
	   iters_per_batch = 4;
	   thallium_server = dsc->get_thallium_server();
	   thallium_shm_server = dsc->get_thallium_shm_server();
	   thallium_client = dsc->get_thallium_client();
	   thallium_shm_client = dsc->get_thallium_shm_client();
	   serveraddrs = dsc->get_serveraddrs();
	   ipaddrs = dsc->get_ipaddrs();
	   shmaddrs = dsc->get_shm_addrs();
           myipaddr = ipaddrs[myrank];
	   dm = new databuffers(numprocs,myrank,numcores,CM);
	   dm->server_client_addrs(thallium_server,thallium_client,thallium_shm_server,thallium_shm_client,ipaddrs,shmaddrs,serveraddrs);
	   CM->server_client_addrs(thallium_server,thallium_client,thallium_shm_server,thallium_shm_client,ipaddrs,shmaddrs,serveraddrs);
	   CM->bind_functions();
	   ds = new dsort(numprocs,myrank);
	   nm = new nvme_buffers(numprocs,myrank);
	   io_queue_async = new boost::lockfree::queue<struct io_request*> (128);
	   io_queue_sync = new boost::lockfree::queue<struct io_request*> (128);
	   end_of_session.store(0);
	   end_of_io_session.store(0);
	   num_streams.store(0);
	   num_io_threads = 1;
	   file_interval = new std::vector<std::pair<std::atomic<uint64_t>,std::atomic<uint64_t>>> (MAXSTREAMS);
	   write_interval =  new std::vector<std::pair<std::atomic<uint64_t>,std::atomic<uint64_t>>> (MAXSTREAMS);
	   read_interval = new std::vector<std::pair<std::atomic<uint64_t>,std::atomic<uint64_t>>> (MAXSTREAMS);

	   for(int i=0;i<MAXSTREAMS;i++)
	   {
		(*file_interval)[i].first.store(UINT64_MAX);
		(*file_interval)[i].second.store(0);
		(*write_interval)[i].first.store(UINT64_MAX);
		(*write_interval)[i].second.store(0);
		(*read_interval)[i].first.store(UINT64_MAX);
		(*read_interval)[i].second.store(0);

	   }
	   std::function<void(struct thread_arg_w *)> IOFunc(
           std::bind(&read_write_process::io_polling,this, std::placeholders::_1));

	   std::function<void(struct thread_arg_w*)> IOFuncSeq(
	   std::bind(&read_write_process::io_polling_seq,this,std::placeholders::_1));

	   t_args_io.resize(num_io_threads);
	   io_threads.resize(num_io_threads);
	   t_args_io[0].tid = 0;
	   std::thread iothread0{IOFunc,&t_args_io[0]};
	   io_threads[0] = std::move(iothread0);

	}
	~read_write_process()
	{
	   delete dm;
	   delete ds;
	   for(int i=0;i<readevents.size();i++)
	   {
		delete readevents[i]->buffer;
		delete readevents[i];
	   }
	   delete nm;
	   delete io_queue_async;
	   delete io_queue_sync;
	   delete file_interval;
	   delete write_interval;
	   delete read_interval;
	   H5close();

	}

	void bind_functions()
	{
           std::function<void(const tl::request &,int &,std::string&)> CreateEventBuffer(
           std::bind(&read_write_process::ThalliumCreateBuffer,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

	   std::function<void(const tl::request &,std::string&,std::string&)> AddEventBuffer(
	   std::bind(&read_write_process::ThalliumAddEvent,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	   std::function<void(const tl::request &,std::string &,uint64_t&)> GetEventProc(
	   std::bind(&read_write_process::ThalliumGetEventProc,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	
	   std::function<void(const tl::request &,std::string &,uint64_t&)> GetEvent(
	   std::bind(&read_write_process::ThalliumGetEvent,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

	   std::function<void(const tl::request &,std::string &,uint64_t&)> GetNVMEProc(
	   std::bind(&read_write_process::ThalliumGetNVMEProc,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	   
	   std::function<void(const tl::request &,std::string &,uint64_t &)>GetNVMEEvent(
	   std::bind(&read_write_process::ThalliumGetNVMEEvent,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	   std::function<void(const tl::request &,std::string &,uint64_t&)> FindEvent(
           std::bind(&read_write_process::ThalliumFindEvent,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

	   std::function<void(const tl::request &,std::string &)> CheckFile(
	   std::bind(&read_write_process::ThalliumCheckFile,this,std::placeholders::_1,std::placeholders::_2));

	   thallium_server->define("EmulatorCheckFile",CheckFile);
	   thallium_shm_server->define("EmulatorCheckFile",CheckFile);	   
	   thallium_server->define("EmulatorFindEvent",FindEvent);
	   thallium_shm_server->define("EmulatorFindEvent",FindEvent);
	   thallium_server->define("EmulatorGetNVMEEvent",GetNVMEEvent);
	   thallium_shm_server->define("EmulatorGetNVMEEvent",GetNVMEEvent);
	   thallium_server->define("EmulatorGetNVMEProc",GetNVMEProc);
	   thallium_shm_server->define("EmulatorGetNVMEProc",GetNVMEProc);
	   thallium_server->define("EmulatorGetEvent",GetEvent);
	   thallium_shm_server->define("EmulatorGetEvent",GetEvent);
	   thallium_server->define("EmulatorGetEventProc",GetEventProc);
	   thallium_shm_server->define("EmulatorGetEventProc",GetEventProc);
           thallium_server->define("EmulatorCreateBuffer",CreateEventBuffer);
           thallium_shm_server->define("EmulatorCreateBuffer",CreateEventBuffer);
	   thallium_server->define("EmulatorAddEvent",AddEventBuffer);
	   thallium_shm_server->define("EmulatorAddEvent",AddEventBuffer);
	}
	void end_session_flag()
	{
	  end_of_session.store(1);

	}
	void end_sessions()
	{
		end_of_io_session.store(1);
		for(int i=0;i<num_io_threads;i++) io_threads[i].join();

	}
	void sync_queue_push(struct io_request *r)
	{
		io_queue_sync->push(r);
	}
	void spawn_write_streams(std::vector<std::string> &,std::vector<int> &,int);
	dsort *get_sorter()
	{
		return ds;
	}
	void create_write_buffer(std::string &s,event_metadata &em,int maxsize)
	{
            m1.lock(); 
	    if(write_names.find(s)==write_names.end())
	    {
	      struct atomic_buffer *ev = nullptr;
	      ev = dm->create_write_buffer(maxsize);
	      myevents.push_back(ev);
	      int n = myevents.size()-1;
	      //numrecvevents[n].store(0);
	      std::pair<std::string,std::pair<int,event_metadata>> p2;
	      p2.first.assign(s);
	      p2.second.first = myevents.size()-1;
	      p2.second.second = em;
	      write_names.insert(p2);
	      ds->create_sort_buffer();
	      try
	      {
	         nm->create_nvme_buffer(s,em);
	      }
	      catch(const std::exception &except)
	      {
		std::cout <<except.what()<<std::endl;
		exit(-1);
	      }
	      num_dropped.push_back(0);
	      batch_size.push_back(maxsize);
	    }
	    m1.unlock();
	}	
	void create_read_buffer(std::string &s,event_metadata &em)
	{
	    m2.lock();
	    auto r = read_names.find(s);;
	    if(r==read_names.end())
	    {
	        struct atomic_buffer *ev = new struct atomic_buffer ();
    	        ev->buffer = new std::vector<struct event> ();		
		readevents.push_back(ev);
		std::pair<int,event_metadata> p1(readevents.size()-1,em);
		std::pair<std::string,std::pair<int,event_metadata>> p2(s,p1);
		read_names.insert(p2);
	    }	
	    m2.unlock();
	}
	void get_events_from_map(std::string &s)
	{
	   int index = -1;
	   m1.lock();
           auto r = write_names.find(s);
	   if(r != write_names.end())
	   {
	      index = (r->second).first;
	   }
	   m1.unlock();
	   if(index != -1)
	   myevents[index] = dm->get_atomic_buffer(index);
	}

	atomic_buffer* get_write_buffer(std::string &s)
	{
	   m1.lock();
	   int index = -1;
	   auto r = write_names.find(s);
	   if(r != write_names.end()) index = (r->second).first;
	   m1.unlock();

	   if(index==-1) return nullptr;
	   else return dm->get_atomic_buffer(index);
	}

	atomic_buffer* get_read_buffer(std::string &s)
	{
	   m2.lock();
	   int index = -1;
	   auto r = read_names.find(s);
	   if(r != read_names.end()) index = (r->second).first;
	   m2.unlock();
	
	   if(index==-1) return nullptr;
	   else return readevents[index];
	}

	void sort_events(std::string &);

	int get_event_proc(int index,uint64_t &ts)
	{
	    int pid = dm->GetValue(ts,index);
	    return pid;
	}
	int get_event_proc(std::string &s,uint64_t &ts)
	{
	    int index = -1;
	    m1.lock();
	    auto r = write_names.find(s);
	    if(r != write_names.end())
	       index = (r->second).first;
	    m1.unlock();
	    if(index == -1) return index;
	    int pid = dm->GetValue(ts,index);
	    return pid;
	}
	int get_nvme_proc(std::string &s,uint64_t ts)
	{
	   int index_r = nm->get_proc(s,ts);
	   return index_r;
	}
	bool find_nvme_event(std::string &s,uint64_t ts, struct event *e)
	{
	    return nm->find_event(s,ts,e); 
	}
	std::string find_nvme_event(std::string &s,uint64_t &ts)
	{
	   int index = -1;
	   event_metadata em;
	   m1.lock();
	   auto r = write_names.find(s);
	   if(r != write_names.end())
	   {
	      index = (r->second).first;
	      em = (r->second).second;
	   }
	   m1.unlock();
	   std::string eventstring;

	   if(index == -1) return  eventstring;

	   struct event e;
	   char data[em.get_datasize()];
	   e.data = data;

	   bool b = find_nvme_event(s,ts,&e);

	   eventstring = pack_event(&e,em.get_datasize());
	   return eventstring;
	}
	bool find_event(int index,uint64_t ts,struct event *e,int length)
	{

	   boost::shared_lock<boost::shared_mutex> lk(myevents[index]->m);

	   e->ts = UINT64_MAX;

           for(int i=0;i<myevents[index]->buffer_size.load();i++)
	   {	   
		if((*myevents[index]->valid)[i].load()==1 && (*myevents[index]->buffer)[i].ts==ts)
		{	
		   e->ts = (*myevents[index]->buffer)[i].ts;
	           std::memcpy(e->data,&(*myevents[index]->buffer)[i].data,length);
		   return true;	   
		   break;
		}
	   }
	   return false;
	}

	std::string find_event(std::string &s,uint64_t &ts)
	{
	    int index = -1;
	    event_metadata em;
	    m1.lock();
	    auto r = write_names.find(s);
	    if(r != write_names.end())
	    {
		index = (r->second).first;
		em = (r->second).second;
	    }
	    m1.unlock();

	    std::string eventstring;
	    if(index == -1) return eventstring;

	    struct event e;
	    char data[em.get_datasize()]; 
	    e.ts = UINT64_MAX;
	    e.data = data;
	    bool b = find_event(index,ts,&e,em.get_datasize());
	    if(b)
	    {
		eventstring = pack_event(&e,em.get_datasize());	
	    }
	    return eventstring;
	}

	event_metadata get_metadata(std::string &s)
	{
		event_metadata em;
	        m1.lock(); 
		auto r = write_names.find(s);
		if(r != write_names.end())
		{
		  em = (r->second).second;
		}
		m1.unlock();
		return em;
	}
	int get_stream_index(std::string &s)
	{
	   int index = -1;
	   m1.lock();
	   auto r = write_names.find(s);
	   if(r != write_names.end())
	     index = (r->second).first;	   
	   m1.unlock();
	   return index;
	}

	void get_file_minmax(std::string &s,uint64_t &min_v,uint64_t &max_v)
	{
	   min_v = UINT64_MAX; max_v = 0;
	
	   int index = -1;
	   m1.lock();
	   auto r = std::find(file_names.begin(),file_names.end(),s);
	   if(r != file_names.end()) index = std::distance(file_names.begin(),r);
	   m1.unlock();

	   if(index != -1)
	   {
	     min_v = (*file_interval)[index].first.load();
	     max_v = (*file_interval)[index].second.load();
	   }

	}
	bool file_exists(std::string &s)
	{
	   bool found = false;
	   m1.lock();
	   auto r = std::find(file_names.begin(),file_names.end(),s);
	   if(r != file_names.end()) found = true;
	   m1.unlock();
	   return found;
	}

        bool get_range_in_read_buffers(std::string &s,uint64_t &min_v,uint64_t &max_v)
	{
	    min_v = UINT64_MAX; max_v = 0;
	    bool err = false;
	    int index = -1;
	    m2.lock();
	    auto r = read_names.find(s);
	    if(r != read_names.end()) index = r->second.first;
	    m2.unlock();

	    if(index != -1)
	    {
		uint64_t minv = (*read_interval)[index].first.load();
		uint64_t maxv = (*read_interval)[index].second.load();

		min_v = (min_v < minv) ? min_v : minv;
		max_v = (max_v > maxv) ? max_v : maxv;
		err = true;
	    }
	    return err;
	}

	bool get_range_in_write_buffers(std::string &s,uint64_t &min_v,uint64_t &max_v)
	{
	    min_v = UINT64_MAX; max_v = 0;
	    bool err = false;
	    int index = -1;
	    m1.lock();
	    auto r = write_names.find(s);
	    if(r != write_names.end()) index = r->second.first;
	    m1.unlock();

	    if(index != -1)
	    {
		uint64_t minv = (*write_interval)[index].first.load(); uint64_t maxv = (*write_interval)[index].second.load();

		min_v = (min_v < minv) ? min_v : minv;
		max_v = (max_v > maxv) ? max_v : maxv;
		err = true;
	    }
	    return err;
	}

	int num_write_events(std::string &s)
	{
		int index = -1;
	        m1.lock();
		auto r = write_names.find(s);
		if(r != write_names.end())
		{
		  index = (r->second).first;
		}
		m1.unlock();
		if(index != -1)
		{
		  return myevents[index]->buffer_size.load();
		}
		return 0;
	}
	int dropped_events()
	{
	    return dm->num_dropped_events();
	}
	void ThalliumCheckFile(const tl::request &req,std::string &s)
	{
	   req.respond(file_exists(s));
	}
	void ThalliumCreateBuffer(const tl::request &req, int &num_events,std::string &s)
        {
                req.respond(create_buffer(num_events,s));
        }
        void ThalliumAddEvent(const tl::request &req, std::string &s,std::string &data)
	{
	        req.respond(add_event(s,data));
	}
	void ThalliumGetEventProc(const tl::request &req,std::string &s,uint64_t &ts)
	{
		req.respond(get_event_proc(s,ts));
	}
        void ThalliumGetEvent(const tl::request &req,std::string &s,uint64_t &ts)
	{
		req.respond(find_event(s,ts));
	}
	void ThalliumGetNVMEProc(const tl::request &req,std::string &s,uint64_t &ts)
	{
		req.respond(get_nvme_proc(s,ts));
	}
	void ThalliumGetNVMEEvent(const tl::request &req,std::string &s,uint64_t &ts)
	{
		req.respond(find_nvme_event(s,ts));
	}

	void ThalliumFindEvent(const tl::request &req,std::string &s,uint64_t &ts)
        {
           req.respond(FindEvent(s,ts));
        }


	std::string FindEvent(std::string&,uint64_t&);
	std::string GetEvent(std::string &,uint64_t&,int);
	std::string GetNVMEEvent(std::string &,uint64_t&,int);
        bool get_events_in_range_from_read_buffers(std::string &s,std::pair<uint64_t,uint64_t> &range,std::vector<struct event> &oup);
	void create_events(int num_events,std::string &s,double);
	void clear_write_events(int,uint64_t&,uint64_t&);
	void clear_read_events(std::string &s);
	void get_range(std::string &s);
	void pwrite_extend_files(std::vector<std::string>&,std::vector<hsize_t>&,std::vector<hsize_t>&,std::vector<std::pair<std::vector<struct event>*,std::vector<char>*>>&,std::vector<uint64_t>&,std::vector<uint64_t>&,bool,std::vector<int>&,std::vector<std::vector<std::vector<int>>>&);
	void pwrite(std::vector<std::string>&,std::vector<hsize_t>&,std::vector<hsize_t>&,std::vector<std::pair<std::vector<struct event>*,std::vector<char>*>>&,std::vector<uint64_t>&,std::vector<uint64_t>&,bool,std::vector<int>&,std::vector<std::vector<std::vector<int>>>&);
	void pwrite_files(std::vector<std::string> &,std::vector<hsize_t> &,std::vector<hsize_t>&,std::vector<std::pair<std::vector<struct event>*,std::vector<char>*>>&,std::vector<uint64_t>&,std::vector<uint64_t>&,bool,std::vector<int>&,std::vector<std::vector<std::vector<int>>>&);
	bool preaddata(const char*,std::string &s,uint64_t,uint64_t,uint64_t&,uint64_t&,uint64_t&,uint64_t&,std::vector<struct event>*);
	void preadappend(const char*,const char*,std::string&);
	bool preadfileattr(const char*);
	std::pair<std::vector<struct event>*,std::vector<char>*>
	create_data_spaces(std::string &,hsize_t&,hsize_t&,uint64_t&,uint64_t&,bool,int&,std::vector<std::vector<int>>&);
	void io_polling(struct thread_arg_w*);
	void io_polling_seq(struct thread_arg_w*);
	void data_stream(struct thread_arg_w*);
	void sync_clocks();
	bool create_buffer(int &,std::string &);
	std::vector<uint64_t> add_event(std::string&,std::string&);
        int endsessioncount();	
};

#endif
