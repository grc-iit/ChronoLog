#include "PSClient.h"


int main(int argc,char **argv)
{
   int prov;

   MPI_Init_thread(&argc,&argv,MPI_THREAD_MULTIPLE,&prov);
   assert (prov == MPI_THREAD_MULTIPLE);

   int size,rank;
   MPI_Comm_size(MPI_COMM_WORLD,&size);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);

   pubsubclient *p = new pubsubclient(size,rank);











  delete p;


  MPI_Finalize();

}
