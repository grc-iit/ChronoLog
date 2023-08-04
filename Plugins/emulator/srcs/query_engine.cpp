#include "query_engine.h"

void query_engine::send_query(std::string &s)
{
       int count = query_number.fetch_add(1);
       struct query_req r;
       r.name = s;
       r.minkey = 0;
       r.maxkey = UINT64_MAX;
       r.collective = false;
       r.id = count;
       r.sorted = true;
       r.output_file = false;
       r.single_point = false;
       r.sender = 0;
       if(myrank==0)
       {
          Q->PutRequestAll(r);
	  std::cout <<" query id = "<<r.id<<std::endl;
        }

}

void query_engine::query_point(std::string &s,uint64_t ts)
{
   int count = query_number.fetch_add(1); 
   struct query_req r;
   r.name = s;
   r.minkey = ts;
   r.maxkey = ts;
   r.collective = false;
   r.id = count;
   r.sender = myrank;
   r.from_nvme = true;
   r.sorted = false;
   r.output_file = false;
   r.single_point = true;
  

   int index = rwp->get_stream_index(s);

   int pid = rwp->get_event_proc(index,ts);

   std::cout <<" pid = "<<pid<<std::endl;

   if(rwp->get_nvme_proc(index,ts)!=-1)
   {
	Q->PutRequest(r,pid);
   }
   else
   {
      r.from_nvme = true;
      Q->PutRequest(r,pid);
   }
}

void query_engine::sort_response(std::string &s,int id,std::vector<struct event> *buf,uint64_t &maxkey)
{

}

bool query_engine::end_file_read(bool end_read,int id)
{

}
void query_engine::get_range(std::vector<struct event> *buf1,std::vector<struct event> *buf2,std::vector<struct event> *buf3,uint64_t minkeys[3],uint64_t maxkeys[3],int id)
{

}

void query_engine::service_query(struct thread_arg_q* t) 
{
	int end_service = 0;

	bool sorted = false;

	while(true)
	{
	   struct query_req *r = nullptr;
	   r=Q->GetRequest();
	   if(r != nullptr)
          {

	      std::string sname(r->name);
	      if(sname.compare("endsession")==0) 
	      {
		     delete r;
		     break;
	      }
	      uint64_t min_key1,max_key1;

	      bool b = false;
	    
	      int pid = -1;

	      struct event e;
	      if(!r->from_nvme)
	      rwp->find_event(r->name,r->minkey,e);
	      else
	      rwp->find_nvme_event(r->name,r->minkey,e);
	      pid = r->sender;
	     
	      struct query_resp s;
	     
	      s.id = r->id;
   	      s.minkey = r->minkey;
   	      s.maxkey = r->maxkey;
   	      s.sender = myrank;
   	      //s.response = e.pack_event();
   	      s.complete = true;
  		
	      std::cout <<" ts = "<<e.ts<<std::endl; 
	      //Q->PutResponse(s,pid); 
	     
	      /*std::vector<struct event> *buf1 = new std::vector<struct event> ();
	      std::vector<struct event> *buf2 = new std::vector<struct event> ();
	      std::vector<struct event> *buf3 = new std::vector<struct event> ();
	      std::vector<struct event> *resp_vec = nullptr;


	      std::string filename = "file";
	      //filename += names[i]+".h5";
                filename += sname+".h5";

	      uint64_t minkey_f,maxkey_f;

	      uint64_t minkey_r,maxkey_r;

	      uint64_t minkey_e = 0; 
	      uint64_t maxkey_e = UINT64_MAX;
	
		  
	      struct io_request *nq = new struct io_request();
	      nq->name.assign(sname);
	      nq->completed.store(0);
	      nq->buf1 = buf1;
	      nq->buf2 = buf2;
	      nq->buf3 = buf3;
	      rwp->sync_queue_push(nq);
	      while(!nq->completed.load()==1);

	      delete nq;

	      uint64_t minkeys[3],maxkeys[3];
	      
	      get_range(buf1,buf2,buf3,minkeys,maxkeys,100);

	      if(myrank==0)
              {
         	std::cout <<" id = "<<r->id<<" minkey 3 = "<<minkeys[2]<<" maxkey 3 = "<<maxkeys[2]<<std::endl;
         	std::cout <<" id = "<<r->id<<" minkey 2 = "<<minkeys[1]<<" maxkey 2 = "<<maxkeys[1]<<std::endl;
         	std::cout <<" id = "<<r->id<<" minkey 1 = "<<minkeys[0]<<" maxkey 1 = "<<maxkeys[0]<<std::endl;
     	      }*/


	      /*if(sorted)
	      {

		  uint64_t maxkey = std::max(maxkeys[1],maxkeys[2]);
		  sort_response(names[i],i,buf1,maxkey);
	      }*/
	        
	      /*resp_vec = new std::vector<struct event> ();

	      uint64_t minkey = UINT64_MAX;
	      uint64_t maxkey = 0;
	      if(buf3 != nullptr) 
	      {
		  resp_vec->assign(buf3->begin(),buf3->end()); buf3->clear();
	      }

	      if(buf2 != nullptr)
	      {
		  for(int i=0;i<buf2->size();i++)
	      	  {
		   if((*buf2)[i].ts > maxkeys[2]) 
	           {
		     resp_vec->push_back((*buf2)[i]);
		   }
		 }
		 buf2->clear();			
	      }

	      if(buf1 != nullptr)
	      {
		for(int i=0;i<buf1->size();i++)
		{
		   if((*buf1)[i].ts > maxkeys[2] && (*buf1)[i].ts > maxkeys[1])
		   {
			resp_vec->push_back((*buf1)[i]);
		   }
		}
		buf1->clear();
	       }*/
      	      /*struct query_resp *p = new struct query_resp();

	      p->response_vector = nullptr;
	      p->response_vector = resp_vec;

	      O->push(p);	 */   

	      //delete buf1; 
	      //delete buf2;
	      //delete buf3;
	      //delete resp_vec;
	      delete r;
           }
	}

}

void query_engine::sort_file(std::string &s)
{

    /*  std::string attrname = "attr"+std::to_string(0);
      std::string attr_type = "integer";
      std::string outputfile = hs->sort_on_secondary_key<int>(s,attrname,0,0,UINT64_MAX,attr_type);

      hs->merge_tree<int>(s,0);*/
}
