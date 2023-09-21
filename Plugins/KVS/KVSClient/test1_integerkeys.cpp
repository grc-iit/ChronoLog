#include <iostream>
#include "KeyValueStore.h"

struct mthread_arg
{
  KeyValueStore *k;
  int index;
  int nreq;
  int rate;
};

void wthread(struct mthread_arg *m)
{

   m->k->create_keyvalues<integer_invlist,int>(m->index,m->nreq,m->rate);

}

int main(int argc,char **argv)
{

  int prov;

   MPI_Init_thread(&argc,&argv,MPI_THREAD_MULTIPLE,&prov);

   int size,rank;

   MPI_Comm_size(MPI_COMM_WORLD,&size);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);

   KeyValueStore *k = new KeyValueStore(size,rank);

   std::string sname = "table2";
   int n = 3;
   std::vector<std::string> types;
   std::vector<std::string> names;
   std::vector<int> lens;
   types.push_back("int");
   names.push_back("value1");
   lens.push_back(sizeof(int));
   types.push_back("int");
   names.push_back("value2");
   lens.push_back(sizeof(int));
   types.push_back("char");
   names.push_back("value3");
   lens.push_back(200);
   int len = sizeof(int)*2+200;
   KeyValueStoreMetadata m(sname,n,types,names,lens,len);

   std::string sname1 = "table3";
   n = 3;
   KeyValueStoreMetadata m1(sname1,n,types,names,lens,len);

   int s1 = k->start_session(sname,names[0],m,32768);
   int s2 = k->start_session(sname1,names[1],m1,32768);


   int numthreads = 2;

   std::vector<struct mthread_arg> args(numthreads);
   std::vector<std::thread> workers(numthreads);

   for(int i=0;i<numthreads;i++)
   {
	args[i].k = k;
	args[i].index = i;
	args[i].nreq = 4096;
	args[i].rate = 200000;

	std::thread t{wthread,&args[i]};
	workers[i] = std::move(t);
   }

   for(int i=0;i<numthreads;i++) workers[i].join();

   k->close_sessions();

   delete k;
   MPI_Finalize();

}
