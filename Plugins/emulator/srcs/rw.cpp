#include "rw.h"

#define DATASETNAME1 "Data1"

typedef int DATATYPE;

void read_write_process::sync_clocks()
{
   int nstreams = num_dropped.size();

   int total_dropped = 0;
   for(int i=0;i<nstreams;i++) total_dropped += num_dropped[i];

   int max_recorded = 0;
   for(int i=0;i<iters_per_batch;i++)
   {
	for(int j=0;j<nstreams;j++)
		max_recorded += batch_size[j]; 
   } 

   if(myrank==0)
   {
	std::cout <<" rank = "<<myrank<<" total_dropped = "<<total_dropped<<" max_recorded = "<<max_recorded<<std::endl;
	std::cout <<" threshold = "<<max_recorded/4<<std::endl;
   }
   //if(myrank==0)//if((double)total_dropped > (double)(max_recorded/4)) 
   {
	CM->UpdateOffsetMaxError();
   }

   for(int i=0;i<nstreams;i++) num_dropped[i] = 0;

}

bool read_write_process::create_buffer(int &num_events,std::string &s)
{
    int datasize = 0;
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
    datasize = em.get_datasize();
    assert(index != -1 && datasize > 0 && num_events > 0);

    atomic_buffer *ab = dm->get_atomic_buffer(index);
    ab->buffer_size.store(0);
    try
    {
	ab->buffer->resize(num_events);
	ab->datamem->resize(num_events*datasize);
	for(int i=0;i<ab->valid->size();i++)
		(*ab->valid)[i].store(0);
    }
    catch(const std::exception &except)
    {
	std::cout<<except.what()<<std::endl;
	exit(-1);
    }
    return true;
}

std::vector<uint64_t> read_write_process::add_event(std::string &s,std::string &data)
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

    int datasize = em.get_datasize();
   
    assert (data.length() == datasize);

    atomic_buffer *ab = dm->get_atomic_buffer(index);

    uint64_t ts = UINT64_MAX;

    std::vector<uint64_t> res;

    int b = 0;

    boost::shared_lock<boost::shared_mutex> lk(ab->m);
    {
      ts = CM->Timestamp();
      b = dm->add_event(index,ts,data,em);
      if(b!=1) ts = UINT64_MAX;
    }
    res.push_back(ts);
    res.push_back(b);

    return res;
}

void read_write_process::create_events(int num_events,std::string &s,double arrival_rate)
{
    int datasize = 0;
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

    datasize = em.get_datasize();
  
    assert(index != -1 && datasize >  0 && num_events > 0);

    atomic_buffer *ab = dm->get_atomic_buffer(index);

    ab->buffer_size.store(0);
    try
    {
      ab->buffer->resize(num_events);
      ab->datamem->resize(num_events*datasize);
      for(int i=0;i<ab->valid->size();i++)
	      (*ab->valid)[i].store(0);
    }
    catch(const std::exception &except)
    {
	std::cout <<except.what()<<std::endl;
	exit(-1);
    }

    int num_dropped = 0;

    boost::shared_lock<boost::shared_mutex> lk(ab->m); 

    for(int i=0;i<num_events;i++)
    {
	event e;
	uint64_t ts = CM->Timestamp();

	e.ts = ts;
	      
	bool b = dm->add_event(e,index,em);
	if(!b) num_dropped++;
	usleep(20000);
    }

}


void read_write_process::sort_events(std::string &s)
{
      m1.lock();
      auto r = write_names.find(s);
      int index = -1;
      int datasize = -1;
      event_metadata em;
      if(r != write_names.end())
      {
        index = (r->second).first;
        em = (r->second).second;
      }
      m1.unlock();
      datasize = em.get_datasize();
      int nm_index = nm->buffer_index(s);

      if(index == -1 || nm_index == -1)
      {
	throw std::runtime_error("write stream or nvme file does not exist");
      }

      while(nm->get_buffer(nm_index,nm_index,1)==false);
      
      boost::unique_lock<boost::shared_mutex> lk1(myevents[index]->m);
      { 
        ds->get_unsorted_data(myevents[index]->buffer,myevents[index]->datamem,index);
        uint64_t min_v,max_v;
        int numevents = myevents[index]->buffer_size.load();
	int maxevents = myevents[index]->buffer->size();
        myevents[index]->buffer_size.store(0);
        if(ds->sort_data(index,index,numevents,min_v,max_v,em))
        myevents[index]->buffer_size.store(myevents[index]->buffer->size());

        uint64_t minv = std::min(min_v,(*write_interval)[index].first.load());
        (*write_interval)[index].first.store(minv);
        (*write_interval)[index].second.store(max_v);
      
        nm->copy_to_nvme(s,myevents[index]->buffer,myevents[index]->buffer_size.load());
      
        clear_write_events(index,min_v,max_v);
	myevents[index]->buffer->resize(maxevents);
	myevents[index]->datamem->resize(maxevents*datasize);

      }
      
      nm->release_buffer(nm_index);
}

void read_write_process::clear_write_events(int index,uint64_t& min_k,uint64_t& max_k)
{
   if(index != -1)
   {
	dm->clear_write_buffer_no_lock(index);
	uint64_t min_n = max_k+1;
	uint64_t max_n = UINT64_MAX;
	dm->set_valid_range(index,min_n,max_n);
	num_dropped[index] = 0;
   }

}

void read_write_process::clear_read_events(std::string &s)
{
   int index = -1;
   m2.lock();
   auto r = read_names.find(s);
   if(r != read_names.end()) index = (r->second).first;
   m2.unlock();

   if(index==-1) return;

   boost::upgrade_lock<boost::shared_mutex> lk(readevents[index]->m);

   readevents[index]->buffer->clear();
   (*read_interval)[index].first.store(UINT64_MAX);
   (*read_interval)[index].second.store(0);
}

 bool read_write_process::get_events_in_range_from_read_buffers(std::string &s,std::pair<uint64_t,uint64_t> &range,std::vector<struct event> &oup)
{
     uint64_t min = range.first; uint64_t max = range.second;
     bool err = false;
     int index = -1;
     m2.lock();
     auto r = read_names.find(s);
     if(r != read_names.end()) index = r->second.first;
     m2.unlock();

     uint64_t min_s, max_s;
             
     if(index != -1)
     {
          min_s = (*read_interval)[index].first.load();
          max_s = (*read_interval)[index].second.load();
          if(!((max < min_s) && (min > max_s)))
          {
              min_s = std::max(min_s,min);
              max_s = std::min(max_s,max);

           }
            
	  boost::shared_lock<boost::shared_mutex> lk(readevents[index]->m);
          for(int i=0;i<readevents[index]->buffer->size();i++)
          {
                 uint64_t ts = (*readevents[index]->buffer)[i].ts;
                 if(ts >= min_s && ts <= max_s) oup.push_back((*readevents[index]->buffer)[i]);
          }
          err = true;
        }

        return err;
}

void read_write_process::spawn_write_streams(std::vector<std::string> &snames,std::vector<int> &total_events,int nbatches)
{

	int num_threads = snames.size();
        t_args.resize(num_threads);
        workers.resize(num_threads);

        for(int i=0;i<snames.size();i++)
        {
               t_args[i].tid = i;
	       t_args[i].endsession = false;
               int numevents = total_events[i];
               int events_per_proc = numevents/numprocs;
               int rem = numevents%numprocs;
               if(myrank < rem) t_args[i].num_events = events_per_proc+1;
               else t_args[i].num_events = events_per_proc;
               t_args[i].name = snames[i];
         }

	 std::function<void(struct thread_arg_w *)> DataFunc(
         std::bind(&read_write_process::data_stream,this,std::placeholders::_1));

         while(true)
         {
               for(int j=0;j<num_threads;j++)
               {
                      std::thread t{DataFunc,&t_args[j]};
                      workers[j] = std::move(t);
               }

               for(int j=0;j<num_threads;j++) workers[j].join();

               for(int j=0;j<num_threads;j++)
               {
                    struct io_request *r = new struct io_request();
                    r->name = t_args[j].name;
                    r->from_nvme = true;
                    io_queue_async->push(r);
                }

                num_streams.store(num_threads);
                while(num_streams.load()!=0);
	        bool endbatch = true;
		for(int j=0;j<num_threads;j++)
		  if(t_args[j].endsession != true) endbatch = false;
		if(endbatch) break;

           }

}

void read_write_process::pwrite_extend_files(std::vector<std::string>&sts,std::vector<hsize_t>&total_records,std::vector<hsize_t>&offsets,std::vector<std::pair<std::vector<struct event>*,std::vector<char>*>>&data_arrays,std::vector<uint64_t>&minkeys,std::vector<uint64_t>&maxkeys,bool clear_nvme,std::vector<int>&bcounts,std::vector<std::vector<std::vector<int>>>&blockcounts)
{
    hid_t       fid;
    hid_t       acc_tpl;
    hid_t       xfer_plist;
    hid_t       sid;
    hid_t       file_dataspace;
    hid_t       mem_dataspace;
    hid_t       dataset1;
    hid_t       datatype;
    hsize_t dims[1];
    const char* attr_name[1];
    hsize_t adims[1];
    hsize_t maxsize;

    hid_t async_fapl = H5Pcreate(H5P_FILE_ACCESS);
    hid_t async_dxpl = H5Pcreate(H5P_DATASET_XFER);

    H5Pset_fapl_mpio(async_fapl, MPI_COMM_WORLD, MPI_INFO_NULL);
    H5Pset_dxpl_mpio(async_dxpl, H5FD_MPIO_COLLECTIVE);

    size_t num;
    hbool_t op_failed = false;

    adims[0] = 0;

    hsize_t attr_size[1];
    attr_size[0] = MAXBLOCKS*4+4;
    hid_t attr_space[1];
    attr_name[0] = "Datasizes";
    attr_space[0] = H5Screate_simple(1, attr_size, NULL);

    std::vector<hid_t> event_ids;
    std::vector<hid_t> filespaces;
    std::vector<hid_t> memspaces;
    std::vector<hid_t> gids;
    std::vector<hid_t> fids;
    std::vector<hid_t> dset_ids;
    std::vector<hid_t> type_ids;
    std::vector<event_metadata> metadata;
    std::vector<int> valid_id;

    for(int i=0;i<sts.size();i++)
    {
   
    //hid_t es_id = H5EScreate();

    std::string filename = "file"+sts[i]+".h5";
    //fid = H5Fopen_async(filename.c_str(), H5F_ACC_RDWR, async_fapl,es_id);
    fid = H5Fopen(filename.c_str(), H5F_ACC_RDWR, async_fapl);

    event_metadata em;
    int index = -1;
    m1.lock();
    auto r = write_names.find(sts[i]);
    if(r != write_names.end())
    {
       em = (r->second).second;
       index = (r->second).first;
    }
    m1.unlock();

    if(index==-1) 
    {
	throw std::runtime_error("data stream buffer does not exist");
	return;
    }

    int datasize = em.get_datasize();
    int keyvaluesize = sizeof(uint64_t)+datasize;
    metadata.push_back(em);

    if(data_arrays[i].second == nullptr || total_records[i]==0) continue;

    adims[0] = datasize;
    hid_t s1 = H5Tarray_create(H5T_NATIVE_CHAR,1,adims);
    hid_t s2 = H5Tcreate(H5T_COMPOUND,keyvaluesize);
    H5Tinsert(s2,"key",HOFFSET(struct event,ts),H5T_NATIVE_UINT64);
    H5Tinsert(s2,"data",HOFFSET(struct event,data),s1);
    type_ids.push_back(s2);
    type_ids.push_back(s1);

    //hid_t gapl = H5Pcreate(H5P_GROUP_ACCESS);
    std::string grp_name = "async_g"+sts[i];
    //hid_t grp_id = H5Gopen_async(fid, grp_name.c_str(),gapl, es_id); 
    //hid_t grp_id = H5Gopen(fid, grp_name.c_str(),gapl); 
    
    //dataset1 = H5Dopen_async(fid, DATASETNAME1, H5P_DEFAULT,es_id);
    dataset1 = H5Dopen(fid, DATASETNAME1, H5P_DEFAULT);

    //hid_t attr_id = H5Aopen_async(dataset1,attr_name[0],H5P_DEFAULT,es_id);
    hid_t attr_id = H5Aopen(dataset1,attr_name[0],H5P_DEFAULT);
    std::vector<uint64_t> attrs;
    attrs.resize(attr_size[0]);

    int ret = H5Aread(attr_id,H5T_NATIVE_UINT64,attrs.data());

    dims[0] = (hsize_t)(total_records[i]+attrs[0]);
    maxsize = H5S_UNLIMITED;
    H5Dset_extent(dataset1, dims);
    file_dataspace = H5Dget_space(dataset1);
  
    hsize_t offset_w = attrs[0];

    hsize_t offset_p=0;

    for(int j=0;j<bcounts[i];j++)
    {

        hsize_t block_size = blockcounts[i][j][myrank];

        hid_t memdataspace = H5Screate_simple(1,&block_size,&maxsize);
  
        hsize_t offset_t = offset_w;	
	for(int k=0;k<myrank;k++)
		offset_t += blockcounts[i][j][k];
    
	hsize_t blocktotal = 0;
	for(int k=0;k<numprocs;k++)
		blocktotal += blockcounts[i][j][k];

	char *data_p = data_arrays[i].second->data()+offset_p*keyvaluesize;

        hsize_t one = 1;
        //offsets[i] += attrs[0];
        ret = H5Sselect_hyperslab(file_dataspace,H5S_SELECT_SET,&offset_t,NULL,&one,&block_size);
    
        //ret = H5Dwrite_async(dataset1,s2, memdataspace, file_dataspace,async_dxpl,data_p,es_id);
        ret = H5Dwrite(dataset1,s2, memdataspace, file_dataspace,async_dxpl,data_p);

	offset_p += block_size;
	offset_w += blocktotal;
	memspaces.push_back(memdataspace);
    }

    attrs[0] += total_records[i];
    int pos = attrs[3];
    pos = 4+pos*4;
    attrs[3] += 1;
    attrs[pos] = minkeys[i];
    pos++;
    attrs[pos] = maxkeys[i];
    pos++;
    attrs[pos] = attrs[3];
    pos++;
    attrs[pos] = total_records[i];

    //ret = H5Awrite_async(attr_id,H5T_NATIVE_UINT64,attrs.data(),es_id);
    ret = H5Awrite(attr_id,H5T_NATIVE_UINT64,attrs.data());

    //ret = H5Aclose_async(attr_id,es_id);
    ret = H5Aclose(attr_id);
    //event_ids.push_back(es_id);
    //H5Dclose_async(dataset1,es_id);
    H5Dclose(dataset1);
    //H5Gclose_async(grp_id,es_id);
    //H5Pclose(gapl);
    //H5Fclose_async(fid,es_id);
    H5Fclose(fid);
    filespaces.push_back(file_dataspace);
    valid_id.push_back(i);
    }

    int prefix = 0;
    for(int i=0;i<valid_id.size();i++)
    {
        //H5ESwait(event_ids[i],H5ES_WAIT_FOREVER,&num,&op_failed);
	//H5ESclose(event_ids[i]);
        H5Sclose(filespaces[i]);
	H5Tclose(type_ids[2*i]);
	H5Tclose(type_ids[2*i+1]);
	int id = valid_id[i];
	for(int j=0;j<bcounts[id];j++)
        H5Sclose(memspaces[prefix+j]);
	std::string filename = "file"+sts[id]+".h5";
	int ps = -1;
	m1.lock();
	auto r = std::find(file_names.begin(),file_names.end(),filename);
	if(r != file_names.end())
	ps = std::distance(file_names.begin(),r);
        m1.unlock();
	if(ps !=-1)
	{
	  (*file_interval)[ps].second.store(maxkeys[id]);
	}

	if(clear_nvme) 
	{
	   int nm_index = nm->buffer_index(sts[id]);
	   if(nm_index==-1)
	   {
		throw std::runtime_error("nvme file does not exist");
	   }
	   else
	   {
	     int tag_p = 100;
	     int keyvaluesize = sizeof(uint64_t)+metadata[i].get_datasize();
	     while(nm->get_buffer(nm_index,tag_p,2)==false);
	     nm->erase_from_nvme(sts[id],data_arrays[id].second->size()/keyvaluesize,bcounts[id]);
	     nm->release_buffer(nm_index);
	   }
	}
	if(data_arrays[id].second != nullptr) delete data_arrays[id].second;
	prefix += bcounts[id];
    }

    H5Sclose(attr_space[0]);
    H5Pclose(async_fapl);
    H5Pclose(async_dxpl);

}

void read_write_process::preadappend(const char *srcfile,const char *destfile,std::string &deststring)
{
   std::string filestring(srcfile);

   bool end = false;
   m1.lock();
   auto r1 = std::find(file_names.begin(),file_names.end(),filestring);
   
   if(r1==file_names.end()) end = true;
   
   m1.unlock();

   if(end) return;
 
   hsize_t adims[1];
   adims[0] = VALUESIZE;
   hid_t s1 = H5Tarray_create(H5T_NATIVE_CHAR,1,adims);
   hid_t s2 = H5Tcreate(H5T_COMPOUND,sizeof(struct event));
   H5Tinsert(s2,"key",HOFFSET(struct event,ts),H5T_NATIVE_UINT64);
   H5Tinsert(s2,"value",HOFFSET(struct event,data),s1);
 
   hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
   H5Pset_fapl_mpio(fapl, MPI_COMM_WORLD, MPI_INFO_NULL);
   hid_t xfer_plist = H5Pcreate(H5P_DATASET_XFER);
   hid_t ret = H5Pset_dxpl_mpio(xfer_plist, H5FD_MPIO_COLLECTIVE);
   
   hid_t fid =  H5Fopen(srcfile, H5F_ACC_RDONLY, fapl);

   ret = H5Pclose(fapl);

   hsize_t attr_space[1];
   attr_space[0] = 100*4+4;


   const char* attr_name[1];

   attr_name[0] = "Datasizes";

   hid_t dataset1 = H5Dopen2(fid, DATASETNAME1, H5P_DEFAULT);
   hid_t file_dataspace = H5Dget_space(dataset1);

   hid_t attr_id = H5Aopen(dataset1,attr_name[0],H5P_DEFAULT);
   std::vector<uint64_t> attrs;
   attrs.resize(attr_space[0]);

   int cur_block = 0;

   ret = H5Aread(attr_id,H5T_NATIVE_UINT64,attrs.data());

   int total_k = attrs[0];
   int k_size = attrs[1];
   int data_size = attrs[2];
   int numblocks = attrs[3];


   std::vector<std::string> sname;
   sname.push_back(deststring);

   for(int i=0;i<numblocks;i++)
   {
     hsize_t offset = 0;
     int block_id = i;
     int pos = 4;

     uint64_t minkey = attrs[pos+block_id*4+0];
     uint64_t maxkey = attrs[pos+block_id*4+1];

     offset = 0;
     for(int j=0;j<block_id;j++)
       offset += attrs[pos+j*4+3];

     hsize_t block_size = attrs[pos+block_id*4+3];

     int size_per_proc = block_size/numprocs;
     int rem = block_size%numprocs;

     hsize_t offset_w = 0;

     for(int j=0;j<myrank;j++)
     {
            int size_p;
            if(j < rem) size_p = size_per_proc+1;
            else size_p = size_per_proc;
            offset += size_p;
	    offset_w += size_p;
     }

     std::vector<struct event> *data_array = new std::vector<struct event> ();

     hsize_t blocksize = 0;
     if(myrank < rem) blocksize = size_per_proc+1;
     else blocksize = size_per_proc;

     data_array->resize(blocksize);

     ret = H5Sselect_hyperslab(file_dataspace, H5S_SELECT_SET,&offset,NULL,&blocksize,NULL);
     hid_t mem_dataspace = H5Screate_simple(1,&blocksize, NULL);
     ret = H5Dread(dataset1,s2, mem_dataspace, file_dataspace, xfer_plist,data_array->data());
     std::vector<hsize_t> total_records;
     total_records.push_back(block_size);
     std::vector<hsize_t> offsets;
     offsets.push_back(offset_w);
     std::vector<std::vector<struct event>*> darrays;
     darrays.push_back(data_array);
     std::vector<uint64_t> minkeys,maxkeys;
     minkeys.push_back(minkey); maxkeys.push_back(maxkey);
     //pwrite(sname,total_records,offsets,darrays,minkeys,maxkeys,false);

     H5Sclose(mem_dataspace);

   }


   H5Pclose(xfer_plist);
   H5Sclose(file_dataspace);
   ret = H5Aclose(attr_id);
   ret = H5Dclose(dataset1);
   H5Fclose(fid);

}

bool read_write_process::preadfileattr(const char *filename)
{
    hid_t       fid;
    hid_t       acc_tpl;
    hid_t       xfer_plist;
    hid_t       file_dataspace;
    hid_t       mem_dataspace;

    hsize_t attr_space[1];
    attr_space[0] = MAXBLOCKS*4+4;
    const char *attr_name[1];

    std::string filestring(filename);

    bool end = false;
    int index = -1;

    m1.lock();

    auto r1 = std::find(file_names.begin(),file_names.end(),filestring);

    if(r1==file_names.end()) 
    {
	end = true;
	index = std::distance(file_names.begin(),r1);
    }
    m1.unlock();

    if(end) return false;

    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    //H5Pset_fapl_mpio(fapl, MPI_COMM_WORLD, MPI_INFO_NULL);
    fid = H5Fopen(filename, H5F_ACC_RDONLY, fapl);

    hid_t ret = H5Pclose(fapl);

    attr_name[0] = "Datasizes";

    hid_t dataset1 = H5Dopen2(fid, DATASETNAME1, H5P_DEFAULT);
    file_dataspace = H5Dget_space(dataset1);
    hsize_t nchunks = 0;
    H5Dget_num_chunks(dataset1,file_dataspace, &nchunks);

    hid_t attr_id = H5Aopen(dataset1,attr_name[0],H5P_DEFAULT);
    std::vector<uint64_t> attrs;
    attrs.resize(attr_space[0]);

    ret = H5Aread(attr_id,H5T_NATIVE_UINT64,attrs.data());

    int numblocks = attrs[3];

    if(numblocks > 0)
    {
      (*file_interval)[index].first.store(attrs[4+0]);
      (*file_interval)[index].second.store(attrs[4+(numblocks-1)*4+1]);
    }

    H5Sclose(file_dataspace);
    ret = H5Aclose(attr_id);
    ret = H5Dclose(dataset1);
    H5Fclose(fid);
    return true;

}
bool read_write_process::preaddata(const char *filename,std::string &name,uint64_t minkey,uint64_t maxkey,uint64_t &minkey_f,uint64_t &maxkey_f,uint64_t &minkey_r,uint64_t &maxkey_r,std::vector<struct event>* data_buffer)
{

    hid_t       fid;                                              
    hid_t       acc_tpl;                                         
    hid_t       xfer_plist;                                       
    hid_t       file_dataspace;                                   
    hid_t       mem_dataspace;                                    
    hid_t       dataset1, dataset2, dataset5, dataset6, dataset7;

    hsize_t attr_space[1];
    attr_space[0] = MAXBLOCKS*4+4;
    const char *attr_name[1];
    size_t   num_points;    
    int      i, j, k;

    bool end = false;
    m1.lock();
    std::string filenamestring(filename);
    auto f = std::find(file_names.begin(),file_names.end(),filenamestring);
    if(f == file_names.end()) end = true;
    m1.unlock();

    if(end) return false;

    xfer_plist = H5Pcreate(H5P_DATASET_XFER);
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(fapl, MPI_COMM_WORLD, MPI_INFO_NULL);
    H5Pset_dxpl_mpio(xfer_plist, H5FD_MPIO_COLLECTIVE);
 
    fid = H5Fopen(filename, H5F_ACC_RDONLY, fapl);

    hid_t ret = H5Pclose(fapl);


    attr_name[0] = "Datasizes";

    dataset1 = H5Dopen2(fid, DATASETNAME1, H5P_DEFAULT);

    hid_t attr_id = H5Aopen(dataset1,attr_name[0],H5P_DEFAULT);
    std::vector<uint64_t> attrs;
    attrs.resize(attr_space[0]);

    ret = H5Aread(attr_id,H5T_NATIVE_UINT64,attrs.data());

    hsize_t adims[1];
    adims[0] = VALUESIZE;
    hid_t s1 = H5Tarray_create(H5T_NATIVE_CHAR,1,adims);
    hid_t s2 = H5Tcreate(H5T_COMPOUND,sizeof(struct event));
    H5Tinsert(s2,"key",HOFFSET(struct event,ts),H5T_NATIVE_UINT64);
    H5Tinsert(s2,"value",HOFFSET(struct event,data),s1);
    
    int total_k = attrs[0];
    int k_size = attrs[1];
    int data_size = attrs[2];
    int numblocks = attrs[3];

    std::vector<int> blockids;

    minkey_r = UINT64_MAX;
    maxkey_r = 0;
    minkey_f = UINT64_MAX;
    maxkey_f = 0;
    int pos = 4;

    for(int i=0;i<numblocks;i++)
    {
	uint64_t bmin = attrs[pos+i*4+0];
	uint64_t bmax = attrs[pos+i*4+1];
        if(minkey_r > bmin) minkey_r = bmin;
	if(maxkey_r < bmax) maxkey_r = bmax;

	if(minkey >= bmin && minkey <= bmax ||
	   maxkey >= bmin && maxkey <= bmax ||
	   minkey < bmin && bmax < maxkey)
	{
	    if(minkey_f > bmin) minkey_f = bmin;
	    if(maxkey_f < bmax) maxkey_f = bmax;
	    //blockids.push_back(i);
	}
    }

    if(numblocks > 0) blockids.push_back(numblocks-1);

    int index;
    int datasize;

    if(blockids.size()==0) return false;

    hsize_t total_records = 0;

    for(int i=0;i<blockids.size();i++)
    {

	hsize_t block_size = attrs[pos+blockids[i]*4+3];
	total_records += block_size;
    }

    hsize_t records_per_proc = total_records/numprocs;
    hsize_t rem = total_records%numprocs;


    hsize_t offset = 0;

    for(int i=0;i<blockids[0];i++)
	   offset += attrs[pos+i*4+3];

    for(int i=0;i<myrank;i++)
    {
	int size_p;
	if(i < rem) size_p = records_per_proc+1;
	else size_p = records_per_proc;
	offset += size_p;
    }

    hsize_t blocksize = 0;
    if(myrank < rem) blocksize = records_per_proc+1;
    else blocksize = records_per_proc;

    data_buffer->resize(blocksize);
    
    file_dataspace = H5Dget_space(dataset1);
    ret = H5Sselect_hyperslab(file_dataspace, H5S_SELECT_SET,&offset,NULL,&blocksize,NULL);
    mem_dataspace = H5Screate_simple(1,&blocksize, NULL);
    //ret = H5Pset_dxpl_mpio(xfer_plist, H5FD_MPIO_INDEPENDENT);
    ret = H5Dread(dataset1,s2, mem_dataspace, file_dataspace, xfer_plist,data_buffer->data());

    H5Sclose(mem_dataspace); 
    H5Sclose(file_dataspace);
    H5Pclose(xfer_plist);

    H5Aclose(attr_id);

    H5Dclose(dataset1);

    H5Tclose(s2);
    H5Tclose(s1);
    H5Fclose(fid);  
    
    return true;
}

std::pair<std::vector<struct event>*,std::vector<char>*> read_write_process::create_data_spaces(std::string &s,hsize_t &poffset,hsize_t &trecords,uint64_t &minkey,uint64_t &maxkey,bool from_nvme,int &nblocks,std::vector<std::vector<int>> &blockcounts)
{

   std::vector<int> num_events_recorded_l,num_events_recorded;
   num_events_recorded_l.resize(numprocs);
   num_events_recorded.resize(numprocs);
   std::fill(num_events_recorded_l.begin(),num_events_recorded_l.end(),0);
   std::fill(num_events_recorded.begin(),num_events_recorded.end(),0);

   auto r = write_names.find(s);

   if(r==write_names.end()) 
   {
	throw std::runtime_error("stream does not exist"); 
	std::pair<std::vector<struct event>*,std::vector<char>*> p;
	p.first = nullptr;
	p.second = nullptr;
	return p;
   }

   event_metadata em = (r->second).second;
   int datasize = em.get_datasize();

   int keyvaluesize = sizeof(uint64_t)+datasize*sizeof(char);
   std::vector<char> *datamem = new std::vector<char> ();

   if(from_nvme)
   {
     int index;
     int tag_p = 100;
     int nm_index = nm->buffer_index(s);
     if(nm_index == -1)
     {
	delete datamem;
	throw std::runtime_error("nvme file does not exist");
	std::pair<std::vector<struct event>*,std::vector<char>*> p;
	p.first = nullptr; p.second = nullptr;
	return p;
     }
     while(nm->get_buffer(nm_index,tag_p,3)==false);
     nm->fetch_buffer(datamem,s,index,tag_p,nblocks,blockcounts);
     //nm->erase_from_nvme(s,data_array->size(),nblocks);
     nm->release_buffer(nm_index);
   }
   else
   {
   
   }

   num_events_recorded_l[myrank] = datamem->size()/keyvaluesize;

   MPI_Allreduce(num_events_recorded_l.data(),num_events_recorded.data(),numprocs,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
   
   hsize_t total_records = 0;
   for(int j=0;j<num_events_recorded.size();j++) total_records += (hsize_t)num_events_recorded[j];

   uint64_t num_records = total_records;
   uint64_t total_size = num_records*datasize+num_records*sizeof(uint64_t);

   int record_size = datasize+sizeof(uint64_t);

   hsize_t offset = 0;
   for(int i=0;i<myrank;i++)
        offset += (hsize_t)num_events_recorded[i];

   poffset = offset;
   trecords = total_records;

   uint64_t min_key=UINT64_MAX, max_key=0;


   if(num_events_recorded[myrank]>0)
   {
     min_key = *(uint64_t*)(&((*datamem)[0]));
     int p = (num_events_recorded[myrank]-1)*keyvaluesize;
     max_key = *(uint64_t*)(&((*datamem)[p]));
   }

   MPI_Allreduce(&min_key,&minkey,1,MPI_UINT64_T,MPI_MIN,MPI_COMM_WORLD);

   MPI_Allreduce(&max_key,&maxkey,1,MPI_UINT64_T,MPI_MAX,MPI_COMM_WORLD);

   if(myrank==0) std::cout <<" total_records = "<<total_records<<" minkey = "<<minkey<<" maxkey = "<<maxkey<<std::endl;

   std::pair<std::vector<struct event>*,std::vector<char>*> p;
   p.first = nullptr;
   p.second = datamem;
   return p;
}

void read_write_process::pwrite_files(std::vector<std::string> &sts,std::vector<hsize_t>&total_records,std::vector<hsize_t> &offsets,std::vector<std::pair<std::vector<struct event>*,std::vector<char>*>> &data_arrays,std::vector<uint64_t>& minkeys,std::vector<uint64_t>&maxkeys,bool clear_nvme,std::vector<int>&bcounts,std::vector<std::vector<std::vector<int>>> &blockcounts)
{

  hid_t async_fapl = H5Pcreate(H5P_FILE_ACCESS);
  hid_t async_dxpl = H5Pcreate(H5P_DATASET_XFER);

  H5Pset_fapl_mpio(async_fapl, MPI_COMM_WORLD, MPI_INFO_NULL);
  H5Pset_dxpl_mpio(async_dxpl, H5FD_MPIO_COLLECTIVE);

  size_t num;
  hbool_t op_failed = false;

  const char *attr_name[1];
  hsize_t adims[1];
  adims[0] = 0;
  hsize_t attr_size[1];
  attr_size[0] = MAXBLOCKS*4+4;
  hid_t attr_space[1];
  attr_name[0] = "Datasizes";
  attr_space[0] = H5Screate_simple(1, attr_size, NULL);

  std::vector<hid_t> event_ids;
  std::vector<hid_t> dset_ids;
  std::vector<hid_t> gids;
  std::vector<hid_t> fids;
  std::vector<hid_t> filespaces;
  std::vector<hid_t> memspaces;
  std::vector<hid_t> lists;
  std::vector<hid_t> type_ids;
  std::vector<event_metadata> metadata;
  std::vector<int> valid_id;

  for(int i=0;i<sts.size();i++)
  {

        std::string filename = "file"+sts[i]+".h5";
        hsize_t chunkdims[1];
        chunkdims[0] = total_records[i];
        hsize_t maxdims[1];
        maxdims[0] = (hsize_t)H5S_UNLIMITED;

	int index = -1;
	event_metadata em;
	m1.lock();
	auto r = write_names.find(sts[i]);
	if(r != write_names.end())
	{
	   index = (r->second).first;
	   em = (r->second).second;
	}
	m1.unlock();


	if(index == -1)
	{
	   throw std::runtime_error("data stream buffer does not exist");
	   return;
	}

	if(data_arrays[i].second==nullptr||total_records[i]==0)
	{
	    continue;
	}
	int datasize = em.get_datasize();
	chunkdims[0] = em.get_chunksize();
	metadata.push_back(em);

	hid_t dataset_pl = H5Pcreate(H5P_DATASET_CREATE);

        int keyvaluesize = sizeof(uint64_t)+datasize;	
	adims[0] = (hsize_t)datasize;
	hid_t s1 = H5Tarray_create(H5T_NATIVE_CHAR,1,adims);
  	hid_t s2 = H5Tcreate(H5T_COMPOUND,keyvaluesize);
  	H5Tinsert(s2,"key",HOFFSET(struct event,ts),H5T_NATIVE_UINT64);
  	H5Tinsert(s2,"data",HOFFSET(struct event,data),s1);
	type_ids.push_back(s1);
	type_ids.push_back(s2);
	
        std::string grp_name = "async_g"+sts[i];
        int ret = H5Pset_chunk(dataset_pl,1,chunkdims);

        hid_t file_dataspace = H5Screate_simple(1,&total_records[i],maxdims);

	filespaces.push_back(file_dataspace);

	hsize_t boffset = 0;
        //hid_t es_id = H5EScreate();
        //hid_t fid = H5Fcreate_async(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, async_fapl, es_id);
        hid_t fid = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, async_fapl);
        //hid_t grp_id = H5Gcreate_async(fid, grp_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT, es_id);
        //hid_t dataset1 = H5Dcreate_async(fid, DATASETNAME1,s2,file_dataspace, H5P_DEFAULT,dataset_pl, H5P_DEFAULT,es_id);
        hid_t dataset1 = H5Dcreate(fid, DATASETNAME1,s2,file_dataspace, H5P_DEFAULT,dataset_pl, H5P_DEFAULT);


	hsize_t dims[1];
	dims[0] = total_records[i];
	H5Dset_extent(dataset1, dims);

	hsize_t block_w = 0;
	
	for(int j=0;j<bcounts[i];j++)
	{

           hsize_t block_count = blockcounts[i][j][myrank];
           hid_t mem_dataspace = H5Screate_simple(1,&block_count, NULL);
           memspaces.push_back(mem_dataspace);

  	   char *data_c = data_arrays[i].second->data()+block_w*keyvaluesize;	   

           hsize_t boffset_p = boffset;
	   for(int k=0;k<myrank;k++)
		   boffset_p += blockcounts[i][j][k];
	   hsize_t blocktotal = 0;
	   for(int k=0;k<numprocs;k++) blocktotal += blockcounts[i][j][k];

           ret = H5Sselect_hyperslab(file_dataspace,H5S_SELECT_SET,&boffset_p,NULL,&block_count,NULL);
           //ret = H5Dwrite_async(dataset1,s2, mem_dataspace,file_dataspace,async_dxpl,data_c,es_id);
           ret = H5Dwrite(dataset1,s2, mem_dataspace,file_dataspace,async_dxpl,data_c);
	   boffset += blocktotal;
	   block_w += block_count;
	}
        std::vector<uint64_t> attr_data;
	attr_data.resize(attr_size[0]);
        attr_data[0] = total_records[i];
        attr_data[1] = 8;
        attr_data[2] = datasize;
	attr_data[3] = 1;
	
	int pos = 4;
	attr_data[pos] = minkeys[i];
	pos++;
	attr_data[pos] = maxkeys[i];
	pos++;
	attr_data[pos] = 1;
	pos++;
	attr_data[pos] = total_records[i];

	hid_t attr_id[1];
        //attr_id[0] = H5Acreate_async(dataset1, attr_name[0], H5T_NATIVE_UINT64, attr_space[0], H5P_DEFAULT, H5P_DEFAULT,es_id);
        attr_id[0] = H5Acreate(dataset1, attr_name[0], H5T_NATIVE_UINT64, attr_space[0], H5P_DEFAULT, H5P_DEFAULT);

        //ret = H5Awrite_async(attr_id[0], H5T_NATIVE_UINT64, attr_data.data(),es_id);
        ret = H5Awrite(attr_id[0], H5T_NATIVE_UINT64, attr_data.data());

        //ret = H5Aclose_async(attr_id[0],es_id);
        ret = H5Aclose(attr_id[0]);

	//H5Dclose_async(dataset1,es_id);
	H5Dclose(dataset1);
	H5Pclose(dataset_pl);
        //H5Gclose_async(grp_id,es_id);
        //H5Fclose_async(fid,es_id);
        H5Fclose(fid);
        //event_ids.push_back(es_id);
	valid_id.push_back(i);
    }

    int prefix = 0;
    for(int i=0;i<valid_id.size();i++)
    {
	int id = valid_id[i];
        //H5ESwait(event_ids[i],H5ES_WAIT_FOREVER,&num,&op_failed);
        //H5ESclose(event_ids[i]);
	H5Sclose(filespaces[i]);
	for(int j=0;j<bcounts[id];j++)
	{
           H5Sclose(memspaces[prefix+j]);
	}
	H5Tclose(type_ids[2*i]);
	H5Tclose(type_ids[2*i+1]);
	std::string filename = "file"+sts[id]+".h5";
	int ps = -1;
	m1.lock();
        file_names.insert(filename);
	ps = std::distance(file_names.begin(),std::find(file_names.begin(),file_names.end(),filename));
	m1.unlock();
	if(ps!=-1)
	{
	  (*file_interval)[ps].first.store(minkeys[id]);
	  (*file_interval)[ps].second.store(maxkeys[id]);
	}
	if(clear_nvme) 
	{
	   int keyvaluesize = sizeof(uint64_t)+metadata[i].get_datasize();
	   int nm_index = nm->buffer_index(sts[id]);
	   if(nm_index == -1)
	   {
		throw std::runtime_error("nvme file does not exist");
	   }
	   else
	   {
	     int tag_p = 100;
	     while(nm->get_buffer(nm_index,tag_p,2)==false);
	     nm->erase_from_nvme(sts[id],data_arrays[id].second->size()/keyvaluesize,bcounts[id]);
	     nm->release_buffer(nm_index);
	   }
	}
	if(data_arrays[id].second != nullptr) delete data_arrays[id].second;
	prefix += bcounts[id];
    }
   
    H5Sclose(attr_space[0]);
    H5Pclose(async_fapl);
    H5Pclose(async_dxpl);

}

void read_write_process::pwrite(std::vector<std::string>& sts,std::vector<hsize_t>& total_records,std::vector<hsize_t>& offsets,std::vector<std::pair<std::vector<struct event>*,std::vector<char>*>>& data_arrays,std::vector<uint64_t>&minkeys,std::vector<uint64_t>&maxkeys,bool clear_nvme,std::vector<int> &bcounts,std::vector<std::vector<std::vector<int>>> &blockcounts)
{

   std::vector<std::string> sts_n, sts_e;
   std::vector<hsize_t> trec_n, trec_e;
   std::vector<hsize_t> off_n, off_e;
   std::vector<std::pair<std::vector<struct event>*,std::vector<char>*>> darray_n, darray_e;
   std::vector<uint64_t> minkeys_n, minkeys_e;
   std::vector<uint64_t> maxkeys_n, maxkeys_e;

   for(int i=0;i<sts.size();i++)
   {
	std::string fname = "file"+sts[i]+".h5";
        auto r = std::find(file_names.begin(),file_names.end(),fname);

        if(r == file_names.end())
        {
	   sts_n.push_back(sts[i]);
	   trec_n.push_back(total_records[i]);
	   off_n.push_back(offsets[i]);
	   darray_n.push_back(data_arrays[i]);
	   minkeys_n.push_back(minkeys[i]);
	   maxkeys_n.push_back(maxkeys[i]);
        }
	else
	{
	   sts_e.push_back(sts[i]);	
	   trec_e.push_back(total_records[i]);
	   off_e.push_back(offsets[i]);
	   darray_e.push_back(data_arrays[i]);
   	   minkeys_e.push_back(minkeys[i]);
	   maxkeys_e.push_back(maxkeys[i]);	   
	}
   }

   try
   {
      pwrite_files(sts_n,trec_n,off_n,darray_n,minkeys_n,maxkeys_n,clear_nvme,bcounts,blockcounts);
      pwrite_extend_files(sts_e,trec_e,off_e,darray_e,minkeys_e,maxkeys_e,clear_nvme,bcounts,blockcounts);
   }
   catch(const std::exception &except)
   {
	std::cout <<except.what()<<std::endl;
	exit(-1);
   }

}

int read_write_process::endsessioncount()
{
     int send_v = end_of_session.load();
     std::vector<int> recv_v(numprocs);
     std::fill(recv_v.begin(),recv_v.end(),0);
     MPI_Request *reqs = new MPI_Request[2*numprocs];
     int tag_p = 1500; 

     int nreq = 0;
     for(int i=0;i<numprocs;i++)
     {
	MPI_Isend(&send_v,1,MPI_INT,i,tag_p,MPI_COMM_WORLD,&reqs[nreq]);
	nreq++;
	MPI_Irecv(&recv_v[i],1,MPI_INT,i,tag_p,MPI_COMM_WORLD,&reqs[nreq]);
	nreq++;
     }

     MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

    std::free(reqs);     

    int count = 0;
    for(int i=0;i<numprocs;i++) count += recv_v[i];
    return count;

}

void read_write_process::data_stream(struct thread_arg_w *t)
{

   auto t1 = std::chrono::high_resolution_clock::now();
   bool b = true; 

   int numrounds = 0;

   while(true)
   {
      int nprocs = endsessioncount();
      t1 = std::chrono::high_resolution_clock::now();
      if(nprocs==numprocs) 
      {
	  t->endsession = true;
	  break;
      }

      if(numrounds == 2) break;

      for(;;)
      {
        auto t2 = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration<double>(t2-t1).count() > 50 && b) 
        {
	   b = false;
	   break;
        }
      }

      try
      {
	  sort_events(t->name);
	  numrounds++;
      }
      catch(const std::exception &except)
      {
	  std::cout <<except.what()<<std::endl;
	  exit(-1);
      }
      b = true;
   }

   try
   {
       sort_events(t->name);
   }
   catch(const std::exception &except)
   {
	std::cout <<except.what()<<std::endl;
	exit(-1);
   }


   /*for(int i=0;i<4;i++)
   {
        create_events(t->num_events,t->name,1);
	try
	{
	   sort_events(t->name);
	}
	catch(const std::exception &except)
	{
	   std::cout <<except.what()<<std::endl;
	   exit(-1);
	}
   }*/

}

void read_write_process::io_polling_seq(struct thread_arg_w *t)
{

}

void read_write_process::io_polling(struct thread_arg_w *t)
{
    std::vector<std::string> snames;
    std::vector<std::pair<std::vector<struct event>*,std::vector<char>*>> data;
    std::vector<hsize_t> total_records, offsets,numrecords;
    std::vector<uint64_t> minkeys, maxkeys;
    std::vector<int> bcounts;
    std::vector<std::vector<std::vector<int>>> blockcounts; 


    bool clear_nvme = false;

   while(true)
   {

     std::atomic_thread_fence(std::memory_order_seq_cst);

     while(num_streams.load()==0 && end_of_io_session.load()==0);

     int sync_async[3];
     sync_async[0] = 0;
     int async_empty = num_streams.load() == 0 ? 0 : 1;
     sync_async[1] = async_empty;
     int end_sessions = end_of_io_session.load()==0 ? 0 : 1;
     sync_async[2] = io_queue_async->empty() ? end_sessions : 0;

     int sync_empty_all[3];

     MPI_Allreduce(&sync_async,&sync_empty_all,3,MPI_INT,MPI_SUM,MPI_COMM_WORLD);

     if(sync_empty_all[2]==numprocs) 
     {
	     break;
     }

     if(sync_empty_all[1]==numprocs)
     {
       snames.clear(); data.clear(); total_records.clear(); offsets.clear(); numrecords.clear();
       minkeys.clear(); maxkeys.clear();
       bcounts.clear(); blockcounts.clear();

	CM->SynchronizeClocks();
	CM->ComputeErrorInterval();
        //sync_clocks();


       while(!io_queue_async->empty())
       {
         struct io_request *r=nullptr;

         io_queue_async->pop(r);

         if(r != nullptr)
         {
           if(r->from_nvme)
           {
              hsize_t trecords, offset, numrecords;
	      uint64_t min_key,max_key;
	      int nblocks;
	      std::vector<std::vector<int>> blockc;
	      std::pair<std::vector<struct event>*,std::vector<char>*> data_rp;
	      try
	      {
	         data_rp = create_data_spaces(r->name,offset,trecords,min_key,max_key,true,nblocks,blockc);
	      }
	      catch(const std::exception &except)
	      {
		std::cout <<except.what()<<std::endl;
		exit(-1);
	      }
              snames.push_back(r->name);
              total_records.push_back(trecords);
              offsets.push_back(offset);
	      minkeys.push_back(min_key);
	      maxkeys.push_back(max_key);
           //t->numrecords.push_back(numrecords);
              data.push_back(data_rp);
	      bcounts.push_back(nblocks);
 	      blockcounts.push_back(blockc);
	      clear_nvme = true;
          }

	  delete r;
         }
      }

      num_streams.store(0);
      std::atomic_thread_fence(std::memory_order_seq_cst);
      pwrite(snames,total_records,offsets,data,minkeys,maxkeys,clear_nvme,bcounts,blockcounts);
      snames.clear();
      data.clear();
      total_records.clear();
      offsets.clear();
      numrecords.clear();
      bcounts.clear();
      blockcounts.clear();
     }

  }
}

std::string read_write_process::GetEvent(std::string &s,uint64_t &ts,int s_id)
{
 if(ipaddrs[s_id].compare(myipaddr)==0)
 {
    tl::endpoint ep = thallium_shm_client->lookup(shmaddrs[s_id]);
    tl::remote_procedure rp = thallium_shm_client->define("EmulatorGetEvent");
    return rp.on(ep)(s,ts);
 }
 else
 {
   tl::remote_procedure rp = thallium_client->define("EmulatorGetEvent");
   return rp.on(serveraddrs[s_id])(s,ts);
 }
}

std::string read_write_process::GetNVMEEvent(std::string &s,uint64_t &ts,int s_id)
{

   if(ipaddrs[s_id].compare(myipaddr)==0)
   {
	tl::endpoint ep = thallium_shm_client->lookup(shmaddrs[s_id]);
	tl::remote_procedure rp = thallium_shm_client->define("EmulatorGetNVMEEvent");
	return rp.on(ep)(s,ts);
   }
   else
   {
	tl::remote_procedure rp = thallium_client->define("EmulatorGetNVMEEvent");
	return rp.on(serveraddrs[s_id])(s,ts);
   }
}

std::string read_write_process::FindEvent(std::string &s,uint64_t &ts)
{
   std::string eventstring;
   int pid = get_event_proc(s,ts);
   if(pid != -1)
   {

	return GetEvent(s,ts,pid);
   }
   else
   {
	pid = get_nvme_proc(s,ts);
	if(pid != -1) 
	{
          return GetNVMEEvent(s,ts,pid);
	}
   }
   return eventstring;
}
