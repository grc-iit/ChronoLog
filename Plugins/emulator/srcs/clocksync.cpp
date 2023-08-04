#include "ClockSync.h"
#include <cmath>
#include <thread>

template<typename Clocksource>
void ClockSynchronization<Clocksource>::SynchronizeClocks()
{
     
     int nreq = 0;  	
     
     MPI_Request *reqs = (MPI_Request *)std::malloc(2*numprocs*sizeof(MPI_Request));

     myoffset = 0;maxError = 0;

     MPI_Barrier(MPI_COMM_WORLD);

     if(myrank==0)
     {
       std::vector<int> requests;
       requests.resize(numprocs);
       std::fill(requests.begin(),requests.end(),0);

       std::vector<uint64_t> t;
       t.resize(3*numprocs);
       std::fill(t.begin(),t.end(),0);      

       for(int i=1;i<numprocs;i++)
       {
        	  
	MPI_Status s;
	MPI_Recv(&requests[i],1,MPI_INT,i,123,MPI_COMM_WORLD,&s);
	nreq++;
        t[i*3+0] = Timestamp();
       }

       //MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

       for(int i=1;i<numprocs;i++)
	      t[i*3+1] = Timestamp();

       nreq=0;
       for(int i=1;i<numprocs;i++)
       {
	t[i*3+2] = Timestamp();
	MPI_Send(&t[i*3],3,MPI_UINT64_T,i,123,MPI_COMM_WORLD);
	nreq++;
       }
       
       //MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);
   }
   else 
   {
      uint64_t Treq = Timestamp();
      
      int nreq = 0;
      int req = 1;

      MPI_Send(&req,1,MPI_INT,0,123,MPI_COMM_WORLD);
      nreq++;


      //MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

      std::vector<uint64_t> resp;
      resp.resize(3);
      std::fill(resp.begin(),resp.end(),0);
      MPI_Status et;

      nreq = 0;
      MPI_Recv(resp.data(),3,MPI_UINT64_T,0,123,MPI_COMM_WORLD,&et);
      nreq++;
      
      //MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

      uint64_t Tresp = Timestamp();
      int64_t offset1 = resp[0]-Treq;
      int64_t offset2 = resp[2]-Tresp;
      double diff = (double)(offset1+offset2)/(double)2;
      int64_t m_offset = std::ceil(diff);
      if(m_offset < 0) 
      {
	      m_offset = -1*m_offset;
	      is_less = true;
      }
      myoffset = m_offset;

   }

   std::free(reqs);
   //std::cout <<" rank = "<<myrank<<" offset = "<<myoffset<<" timestamp = "<<Timestamp()<<std::endl;

}

template<typename Clocksource>
void ClockSynchronization<Clocksource>::ComputeErrorInterval()
{


	  MPI_Status st;
          MPI_Request *reqs = (MPI_Request *)std::malloc(2*numprocs*sizeof(MPI_Request));
 	  int nreq = 0;

	   /*int sreq=1;
           std::vector<int> recvreq(numprocs);
           std::fill(recvreq.begin(),recvreq.end(),0);


            nreq = 0;
            for(int i=0;i<numprocs;i++)
            {
		MPI_Send(&sreq,1,MPI_INT,i,123,MPI_COMM_WORLD,&reqs[nreq]);
		nreq++;
		MPI_Recv(&recvreq[i],1,MPI_INT,i,123,MPI_COMM_WORLD,&st);
		nreq++;
            }*/
	  MPI_Barrier(MPI_COMM_WORLD);
	
	    //MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

	   if(myrank==0)
	   {
	    std::vector<uint64_t> tstamps,terr;
	    tstamps.resize(numprocs);
	    terr.resize(numprocs);
	    std::fill(tstamps.begin(),tstamps.end(),0);
	    std::fill(terr.begin(),terr.end(),0);

	    uint64_t ts;

	    for(int i=1;i<numprocs;i++)
	    {
		MPI_Status s;
		MPI_Recv(&tstamps[i],1,MPI_UINT64_T,i,123,MPI_COMM_WORLD,&s);
		nreq++;
		tstamps[i] += delay;
		ts = Timestamp();
		if(ts > tstamps[i]) terr[i] = ts-tstamps[i];
		else terr[i] = tstamps[i]-ts;
	    }

	    for(int i=0;i<numprocs;i++)
	    {
		if(maxError < terr[i]) maxError = terr[i];
	    }

	    nreq = 0;
	    for(int i=1;i<numprocs;i++)
	    {
		MPI_Send(&maxError,1,MPI_UINT64_T,i,123,MPI_COMM_WORLD);
		nreq++;
	    }
	    //MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

	}
	else
	{
	    uint64_t ts = Timestamp();

	    MPI_Send(&ts,1,MPI_UINT64_T,0,123,MPI_COMM_WORLD);

	    MPI_Status et;

	    nreq =0;
	    MPI_Recv(&maxError,1,MPI_UINT64_T,0,123,MPI_COMM_WORLD,&et);
	    nreq++;
	    //MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);
	}

	std::vector<uint64_t> times1,times2;
	times1.resize(numprocs); times2.resize(numprocs);
	std::fill(times1.begin(),times1.end(),0);
	std::fill(times2.begin(),times2.end(),0);
	times1[myrank] = Timestamp();

	nreq = 0;


	MPI_Allreduce(times1.data(),times2.data(),numprocs,MPI_UINT64_T,MPI_SUM,MPI_COMM_WORLD);

	if(myrank==0) std::cout <<" maxError = "<<maxError<<std::endl;

	for(int i=0;i<numprocs;i++)
	{
	   uint64_t e;
	   if(times2[i]>times2[myrank]) e = times2[i]-times2[myrank];
	   else e = times2[myrank]-times2[i];
	   //if(myrank==0) std::cout <<" e = "<<e<<" epsilon = "<<epsilon<<std::endl;
	   assert(e >= 0 && e <= 2*maxError+delay+epsilon);
	}
	std::free(reqs);
}
template<typename Clocksource>
void ClockSynchronization<Clocksource>::UpdateOffsetMaxError()
{
     int rank1 = 0;
     uint64_t ts0 = clock->getTimestamp()/unit;
     uint64_t ts1 = RequestTimeStamp(rank1); 
     uint64_t ts2 = clock->getTimestamp()/unit;
     uint64_t ts3 = RequestTimeStamp(rank1);

 
     int64_t diff1 = ts1-ts0;
     int64_t diff2 = ts3-ts2;
     double diff = (double)(diff1+diff2)/(double)2;
     int64_t offset = std::ceil(diff);

     if(myrank != 0)
     {
       if(offset < 0) 
       {
	  myoffset = -1*offset; is_less = true;
       }
       else myoffset = offset;
     }

     //std::cout <<" myoffset = "<<myoffset<<std::endl;

     int numranks = std::ceil(0.25*(double)numprocs);

     std::vector<int> ranks;

     int i=0;
     while(i < numranks)
     {
	int pid = (myrank+1+i)%numprocs;
	ranks.push_back(pid);
	i++;
     }

     
     std::vector<uint64_t> timestamps;
     std::vector<uint64_t> error;

     for(i=0;i<ranks.size();i++)
     {
	uint64_t ts = RequestTimeStamp(ranks[i]);
	ts += delay;
	timestamps.push_back(ts);
	uint64_t mts = Timestamp();
	uint64_t e;
	if(mts > ts) e = mts-ts;
        else e = ts-mts;
	error.push_back(e);	
     }
     uint64_t Error = 0;
     for(i=0;i<error.size();i++)
	     if(error[i] > Error) Error = error[i];
     maxError = Error;
     //std::cout <<" maxError = "<<maxError<<std::endl;
}

template<typename Clocksource>
void ClockSynchronization<Clocksource>::localsync()
{

     std::function<void()> UpdateOffset(
     std::bind(&ClockSynchronization<Clocksource>::UpdateOffsetMaxError,this));


     std::thread t_d{UpdateOffset};

     t_d.join();

}
