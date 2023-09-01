#include "process.h"
#include <cassert>
#include <thallium.hpp>
#include <chrono>
#include <sched.h>
#include <thread>
#include "rw_request.h"
#include "event_metadata.h"


namespace tl=thallium;
using namespace boost::interprocess;
using namespace std::chrono_literals;

int main(int argc,char **argv)
{

  int p;

  MPI_Init_thread(&argc,&argv,MPI_THREAD_MULTIPLE,&p);

  assert(p == MPI_THREAD_MULTIPLE);

  int size,rank;
  MPI_Comm_size(MPI_COMM_WORLD,&size);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);

  cpu_set_t cpu;
  pthread_t self = pthread_self();

  pthread_getaffinity_np(self, sizeof(cpu_set_t), &cpu);

  int num_cores = 0;
  for(int i=0;i<CPU_SETSIZE;i++)
  {
	if(CPU_ISSET(i,&cpu)) num_cores++;
  }

  //std::cout <<" rank = "<<rank<<" num_cores = "<<num_cores<<std::endl;

  auto t1 = std::chrono::high_resolution_clock::now();

  emu_process *np = new emu_process(size,rank,num_cores);

  np->bind_functions();

  np->synchronize();

  auto t2 = std::chrono::high_resolution_clock::now();

  double t = std::chrono::duration<double>(t2-t1).count();

  double total_time = 0;
  MPI_Allreduce(&t,&total_time,1,MPI_DOUBLE,MPI_MAX,MPI_COMM_WORLD);

  if(rank==0) std::cout <<" numprocs = "<<size<<" sync time = "<<total_time<<std::endl;

  /*metadata_client *CC = np->getclientobj();


  if(rank != 0)
  {

  std::string client_id = "client";
  client_id += std::to_string(rank);
  CC->Connect(client_id);

  std::string chronicle_name = "record";
  CC->CreateChronicle(client_id,chronicle_name);

  CC->AcquireChronicle(client_id,chronicle_name);

  CC->ReleaseChronicle(client_id,chronicle_name);

  CC->DestroyChronicle(client_id,chronicle_name);

  }*/

  int numstories = 1;
  std::vector<std::string> story_names;
  std::vector<int> total_events;

  int numattrs1 = 100;
  int numattrs2 = 200;
  event_metadata em1,em2;
  em1.set_numattrs(numattrs1);
  em2.set_numattrs(numattrs2);

  for(int i=0;i<numattrs1;i++)
  {
    std::string a = "attr"+std::to_string(i);
    int vsize = sizeof(char);
    bool is_signed = false;
    bool is_big_endian = true; 
    em1.add_attr(a,vsize,is_signed,is_big_endian);
  }

  for(int i=0;i<numattrs2;i++)
  {
     std::string a = "attr"+std::to_string(i);
     int vsize = sizeof(char);
     bool is_signed = false;
     bool is_big_endian = true;
     em2.add_attr(a,vsize,is_signed,is_big_endian);
  }

  for(int i=0;i<numstories;i++)
  {
	std::string name = "table"+std::to_string(i+1);
	story_names.push_back(name);
	total_events.push_back(8192*size);
	if(i%2==0)
	np->prepare_service(name,em1,8192*size);
	else np->prepare_service(name,em2,8192*size);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  int num_writer_threads = 4;

  int nbatches = 1;

  t1 = std::chrono::high_resolution_clock::now();

  MPI_Barrier(MPI_COMM_WORLD);

  np->data_streams_s(story_names,total_events,nbatches);

  //np->generate_queries(story_names);

  //np->end_sessions_t();
  //
  /*std::string s = "endsessions";
  np->end_sessions(s);
*/
  while(np->process_end()==0);

  np->end_sessions_t();  

  t2 = std::chrono::high_resolution_clock::now();
  t = std::chrono::duration<double> (t2-t1).count();

  double send_v = t;
  std::vector<double> recv_v(size);

  MPI_Request *reqs = new MPI_Request[2*size];

  int nreq = 0;
  int tag = 300000;

  for(int i=0;i<size;i++)
  {
	MPI_Isend(&send_v,1,MPI_DOUBLE,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
	nreq++;
	MPI_Irecv(&recv_v[i],1,MPI_DOUBLE,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
	nreq++;
  }

  MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

  delete reqs;

  if(rank==0)
  {
	std::cout <<" session flag "<<std::endl;
  }

  //np->end_sessions_t();

  delete np;
  MPI_Finalize();

}
