
template<typename KeyT,typename ValueT,typename hashfcn,typename equalfcn>
int hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::partition_no(KeyT &k)
{

      uint64_t hashval = hashfcn()(k);
      uint64_t key = hashval;
      uint64_t mask = UINT64_MAX;
      mask = mask << (64-nbits);
      mask = mask >> (64-nbits);
      key = key & mask;
      key = key >> (nbits-nbits_p);   
      int id = (int)key;
      int rem = ntables%numprocs;
      int offset = rem*(tables_per_proc+1);
      int pid = -1;
      if(id < offset)
      {
	pid = id/(tables_per_proc+1);
      }  
      else
      {
	id = id-offset;
	pid = rem+(id/tables_per_proc);
      }
      return pid;
}

template<typename KeyT,typename ValueT,typename hashfcn,typename equalfcn>
bool hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::put_entry(KeyT &k,ValueT &v)
{
	int pid = partition_no(k);

	bool b = false;
	
	if(pid == serverid)
	   b = LocalPutEntry(k,v);
	else
	   b = PutEntry(k,v,pid);
	return b;
}

template<typename KeyT,typename ValueT,typename hashfcn,typename equalfcn>
int hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::get_entry(KeyT& k,std::vector<ValueT>&values)
{
	int pid = partition_no(k);
        int ret = -1;

	if(pid==serverid)
	    values = LocalGetEntry(k);
	else 
	    values = GetEntry(k,pid);

	std::vector<struct keydata> events = get_events(k,values,pid);	
	//ret = create_async_io_request(k); 
	return ret;
}

template<typename KeyT,typename ValueT,typename hashfcn,typename equalfcn>
void hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::create_async_io_request(KeyT &k,std::vector<ValueT> &values)
{
      struct request r;
      r.name = filename;
      r.attr_name = attributename;
      r.id = 0;

      if(typeid(k)==typeid(r.intkey))
      {
           r.keytype = 0;
   	   r.intkey = (int)k;	   
      }
      else if(typeid(k)==typeid(r.unsignedlongkey))
      {
	   r.keytype = 1;
	   r.unsignedlongkey = (uint64_t)k;
      }
      else if(typeid(k)==typeid(r.floatkey))
      {
	   r.keytype = 2;
	   r.floatkey = (float)k;
      }
      else if(typeid(k)==typeid(r.doublekey))
      {
	   r.keytype = 2;
	   r.doublekey = (double)k;
      }
      r.sender = myrank;
      r.flush = false;

      io_t->LocalPutRequest(r);

}

template<typename KeyT,typename ValueT,typename hashfcn,typename equalfcn>
std::vector<struct keydata> hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::get_events(KeyT &k,std::vector<ValueT> &values,int pid)
{
   std::string fname = "file";
   fname += filename+".h5";

   if(file_exists)
   {

     while(!io_t->announce_sync(io_count));

     hid_t xfer_plist = H5Pcreate(H5P_DATASET_XFER);
     hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);

     hid_t fid = H5Fopen(fname.c_str(),H5F_ACC_RDONLY,fapl);

     std::string d_string = attributename;
     std::string s_string = attributename+"attr";

     hsize_t adims[1];
     adims[0] = 8;
     hid_t s1 = H5Tarray_create(H5T_NATIVE_CHAR,1,adims);
     hid_t s2 = H5Tcreate(H5T_COMPOUND,sizeof(struct keydata));
     H5Tinsert(s2,"key",HOFFSET(struct keydata,ts),H5T_NATIVE_UINT64);
     H5Tinsert(s2,"value",HOFFSET(struct keydata,data),s1);

     if(cached_keyindex_mt.size()>0 && cached_keyindex.size()>0 && pid==myrank)
     {
	   std::string dstring = "Data1";   
           hid_t dataset_t = H5Dopen2(fid,dstring.c_str(),H5P_DEFAULT);
	   hid_t file_dataspace = H5Dget_space(dataset_t);

           uint64_t hashvalue = hashfcn()(k); 
	   int pos = hashvalue%maxsize;
	   hsize_t offset = cached_keyindex_mt[2*pos+1]-cached_keyindex_mt[1];
	   hsize_t numkeys = cached_keyindex_mt[2*pos];

	   hsize_t offset_r = UINT64_MAX;

	   for(int i=offset;i<offset+numkeys;i++)
	   {
		if(cached_keyindex[i].key==k)
		{
		    offset_r = cached_keyindex[i].index;
		    break;
		}
	   }

	   if(offset_r != UINT64_MAX)
	   {
               hsize_t blocksize = 1;
	       hid_t mem_dataspace = H5Screate_simple(1,&blocksize,NULL);
	       std::vector<struct keydata> e(1);	

	       int ret = H5Sselect_hyperslab(file_dataspace, H5S_SELECT_SET,&offset_r,NULL,&blocksize,NULL);
	       ret = H5Dread(dataset_t,s2, mem_dataspace, file_dataspace, xfer_plist,e.data());
		

	       numevents++;

	       H5Sclose(mem_dataspace);
	       H5Sclose(file_dataspace);
	       H5Dclose(dataset_t);
	       H5Fclose(fid);
	       H5Tclose(s2);
	       H5Tclose(s1);
	       H5Pclose(xfer_plist);
	       H5Pclose(fapl);
     	       io_t->reset_sync(io_count);
	       return e;
	   }

	   H5Sclose(file_dataspace);
	   H5Dclose(dataset_t);

     }
     else if(!H5Lexists(fid,d_string.c_str(),H5P_DEFAULT) || !H5Lexists(fid,s_string.c_str(),H5P_DEFAULT))
     {
       hsize_t attr_size[1];
       attr_size[0] = MAXBLOCKS*4+4;
       const char *attrname[1];
       hid_t attr_space[1];
       attr_space[0] = H5Screate_simple(1, attr_size, NULL);

       attrname[0] = "Datasizes";

       std::string data_string = "Data1";
       hid_t dataset1 = H5Dopen2(fid,data_string.c_str(), H5P_DEFAULT);

       hid_t attr_id = H5Aopen(dataset1,attrname[0],H5P_DEFAULT);
       std::vector<uint64_t> attrs;
       attrs.resize(attr_size[0]);

       int ret = H5Aread(attr_id,H5T_NATIVE_UINT64,attrs.data());

       int numblocks = attrs[3];

       std::vector<int> blockids;
     
       int pos = 4;

       if(values.size()>=1)
       {
	ValueT v = values[0];
	for(int i=0;i<numblocks;i++)
	{
	   uint64_t minkey = attrs[4+i*4+0];
	   uint64_t maxkey = attrs[4+i*4+1];

	   if(v >= minkey && v <= maxkey) 
	   {
		  blockids.push_back(i);
		  break;
	   }
	}
     }
    
    if(blockids.size()==1)
    {

	    hsize_t offset_r = 0;
	    for(int i=0;i<blockids[0];i++)
		    offset_r += attrs[4+i*4+3];

	    hid_t file_dataspace = H5Dget_space(dataset1);
	    hsize_t blocksize = attrs[4+blockids[0]*4+3];

	    ret = H5Sselect_hyperslab(file_dataspace, H5S_SELECT_SET,&offset_r,NULL,&blocksize,NULL);
            hid_t mem_dataspace = H5Screate_simple(1,&blocksize, NULL);
	    std::vector<struct keydata> *buffer = new std::vector<struct keydata> ();
	    buffer->resize(blocksize);
            ret = H5Dread(dataset1,s2, mem_dataspace, file_dataspace, xfer_plist,buffer->data());

	    numevents++;
	    
	    for(int i=0;i<buffer->size();i++)
	    {
		if((*buffer)[i].ts==values[0])
		{
		   break;
		}
	    }

	    delete buffer;
	    H5Sclose(mem_dataspace);
	    H5Sclose(file_dataspace);
       }

       H5Aclose(attr_id);
       H5Dclose(dataset1);
    } 
    else
    {
        std::string dstring = "Data1";
	hid_t dataset_t = H5Dopen2(fid,dstring.c_str(),H5P_DEFAULT);
        hid_t file_dataspace = H5Dget_space(dataset_t);
	hid_t dataset_mt = H5Dopen2(fid,s_string.c_str(),H5P_DEFAULT);
	hid_t file_dataspace_mt = H5Dget_space(dataset_mt);
	hid_t dataset_table = H5Dopen2(fid,d_string.c_str(),H5P_DEFAULT);
	hid_t file_dataspace_table = H5Dget_space(dataset_table);

	std::vector<int> table_mt;

	int rem = ntables%numprocs;

	hsize_t offset_mt = 0;

	for(int i=0;i<pid;i++)
	{
	    int nt = 0;
	    if(i < rem) nt = tables_per_proc+1;
	    else nt = tables_per_proc;    
	    offset_mt += 2*nt*pow(2,nbits_r);
	}

	int nt = 0;
	if(pid < rem) nt = tables_per_proc+1;
	else nt = tables_per_proc;

	int maxsize_p = nt*pow(2,nbits_r);

	hsize_t blocksize = 2*maxsize_p;

	table_mt.resize(blocksize);

	int ret = H5Sselect_hyperslab(file_dataspace_mt, H5S_SELECT_SET,&offset_mt,NULL,&blocksize,NULL);
        hid_t mem_dataspace_mt = H5Screate_simple(1,&blocksize, NULL);
        ret = H5Dread(dataset_mt,H5T_NATIVE_INT, mem_dataspace_mt, file_dataspace_mt, xfer_plist,table_mt.data());

	uint64_t hashvalue = hashfcn()(k);
        int pos = hashvalue%maxsize_p;
        hsize_t offset_t = table_mt[2*pos+1];
        blocksize = table_mt[2*pos];

	std::vector<struct KeyIndex<KeyT>> Keyindex;
	Keyindex.resize(blocksize);

	ret = H5Sselect_hyperslab(file_dataspace_table,H5S_SELECT_SET,&offset_t,NULL,&blocksize,NULL);
	hid_t mem_dataspace_table = H5Screate_simple(1,&blocksize,NULL);
        ret = H5Dread(dataset_table,kv1,mem_dataspace_table,file_dataspace_table,xfer_plist,Keyindex.data());

	hsize_t offset_r = UINT64_MAX;

	for(int i=0;i<Keyindex.size();i++)
	{
	  if(Keyindex[i].key==k)
	  {
		offset_r = Keyindex[i].index;
		break;
	  }
	}

	if(offset_r != UINT64_MAX)
	{
	   std::vector<struct keydata> e(1);		
	   blocksize = 1;
	   ret = H5Sselect_hyperslab(file_dataspace,H5S_SELECT_SET,&offset_r,NULL,&blocksize,NULL);
	   hid_t mem_dataspace_e = H5Screate_simple(1,&blocksize,NULL);
	   ret = H5Dread(dataset_t,s2,mem_dataspace_e,file_dataspace,xfer_plist,e.data());
	   H5Sclose(mem_dataspace_e);
	   numevents++;
	}
	
	
        H5Sclose(mem_dataspace_table);
	H5Sclose(mem_dataspace_mt);
	H5Sclose(file_dataspace_table);
	H5Sclose(file_dataspace_mt);
	H5Sclose(file_dataspace);
	H5Dclose(dataset_table);
	H5Dclose(dataset_mt);
	H5Dclose(dataset_t);

     }

     H5Tclose(s1);
     H5Tclose(s2);
     H5Fclose(fid);
     H5Pclose(fapl);
     H5Pclose(xfer_plist);

     io_t->reset_sync(io_count);
   }




   std::vector<struct keydata> events;
   return events;

}

template<typename KeyT,typename ValueT,typename hashfcn,typename equalfcn>
void hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::cache_latest_table()
{
   std::string fname = "file";
   fname += filename+".h5";

   if(file_exists)
   {
     while(!io_t->announce_sync(io_count));
     
     hid_t xfer_plist = H5Pcreate(H5P_DATASET_XFER);
     hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);

     hid_t fid = H5Fopen(fname.c_str(),H5F_ACC_RDONLY,fapl);

     std::string attr_t = attributename+"attr";
     std::string d_string = attributename;

     if(H5Lexists(fid,d_string.c_str(),H5P_DEFAULT) && H5Lexists(fid,attr_t.c_str(),H5P_DEFAULT))
     {
	cached_keyindex_mt.clear();
	cached_keyindex_mt.resize(2*maxsize);
        std::fill(cached_keyindex_mt.begin(),cached_keyindex_mt.end(),0);

        hsize_t offsett = 2*pre_table*pow(2,nbits_r);

        hid_t dataset_t = H5Dopen2(fid,attr_t.c_str(),H5P_DEFAULT);

        hid_t file_dataspace_table = H5Dget_space(dataset_t);

        hid_t dataset_k = H5Dopen2(fid,d_string.c_str(),H5P_DEFAULT);

	hid_t file_dataspace_t = H5Dget_space(dataset_k);

	hsize_t tsize = 2*maxsize;

	hid_t mem_dataspace_t = H5Screate_simple(1,&tsize,NULL);

	int ret = H5Sselect_hyperslab(file_dataspace_table,H5S_SELECT_SET,&offsett,NULL,&tsize,NULL);
        ret = H5Dread(dataset_t,H5T_NATIVE_INT,mem_dataspace_t,file_dataspace_table,xfer_plist,cached_keyindex_mt.data());

	int total_keys = 0;
	for(int i=0;i<maxsize;i++) total_keys += cached_keyindex_mt[2*i];

	cached_keyindex.clear();
	cached_keyindex.resize(total_keys);
	hsize_t blocksize = total_keys;

	hid_t mem_dataspace_k = H5Screate_simple(1,&blocksize,NULL);

	hsize_t offset_k = cached_keyindex_mt[1];

	ret = H5Sselect_hyperslab(file_dataspace_t,H5S_SELECT_SET,&offset_k,NULL,&blocksize,NULL);
	ret = H5Dread(dataset_k,kv1,mem_dataspace_k,file_dataspace_t,xfer_plist,cached_keyindex.data());

	H5Dclose(dataset_k);
	H5Dclose(dataset_t);
	H5Sclose(mem_dataspace_k);
	H5Sclose(mem_dataspace_t);
	H5Sclose(file_dataspace_t);
	H5Sclose(file_dataspace_table);
     }
     H5Fclose(fid);
     H5Pclose(fapl);
     H5Pclose(xfer_plist);
     io_t->reset_sync(io_count);
   }

}


template<typename KeyT, typename ValueT,typename hashfcn,typename equalfcn>
void hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::fill_invlist_from_file(std::string &s,int offset)
{
   std::string filename = "file";
   filename += s+".h5";

   hid_t xfer_plist = H5Pcreate(H5P_DATASET_XFER);
   hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
   H5Pset_fapl_mpio(fapl,MPI_COMM_WORLD, MPI_INFO_NULL);
   H5Pset_dxpl_mpio(xfer_plist, H5FD_MPIO_COLLECTIVE);

   hsize_t chunkdims[1];
   chunkdims[0] = 8192;
   hsize_t maxdims[1];
   maxdims[0] = (hsize_t)H5S_UNLIMITED;

   hid_t dataset_pl = H5Pcreate(H5P_DATASET_CREATE);

   int ret = H5Pset_chunk(dataset_pl,1,chunkdims);

   hid_t fid = H5Fopen(filename.c_str(),H5F_ACC_RDWR,fapl);     

   hsize_t attr_size[1];
   attr_size[0] = MAXBLOCKS*4+4;
   const char *attrname[1];
   hid_t attr_space[1];
   attr_space[0] = H5Screate_simple(1, attr_size, NULL);

   attrname[0] = "Datasizes";

   std::string data_string = "Data1";
   hid_t dataset1 = H5Dopen2(fid,data_string.c_str(), H5P_DEFAULT);

   hid_t attr_id = H5Aopen(dataset1,attrname[0],H5P_DEFAULT);
   std::vector<uint64_t> attrs;
   attrs.resize(attr_size[0]);

   ret = H5Aread(attr_id,H5T_NATIVE_UINT64,attrs.data());

    hsize_t adims[1];
    adims[0] = VALUESIZE;
    hid_t s1 = H5Tarray_create(H5T_NATIVE_CHAR,1,adims);
    hid_t s2 = H5Tcreate(H5T_COMPOUND,sizeof(struct event));
    H5Tinsert(s2,"key",HOFFSET(struct event,ts),H5T_NATIVE_UINT64);
    H5Tinsert(s2,"value",HOFFSET(struct event,data),s1);

    int numblocks = attrs[3];


    if(myrank==0) std::cout <<" numblocks = "<<numblocks<<std::endl;

    hid_t file_dataspace = H5Dget_space(dataset1);

    std::vector<struct event> *buffer = new std::vector<struct event> ();

    int pos = 4;
    hsize_t offset_r = 0;
    for(int i=0;i<numblocks;i++)
    {
	int nrecords = attrs[pos+i*4+3];

	int records_per_proc = nrecords/numprocs;
	int rem = nrecords%numprocs;

	hsize_t pre = offset_r;
	for(int i=0;i<myrank;i++)
	{	
	   int size_p=0;
	   if(i < rem) size_p = records_per_proc+1;
	   else size_p = records_per_proc;
	   pre += size_p;
	}

	hsize_t blocksize;
        if(myrank < rem) blocksize = records_per_proc+1;
 	else blocksize = records_per_proc;	
	
	buffer->clear();
	buffer->resize(blocksize);

        ret = H5Sselect_hyperslab(file_dataspace, H5S_SELECT_SET,&pre,NULL,&blocksize,NULL);
        hid_t mem_dataspace = H5Screate_simple(1,&blocksize, NULL);
        ret = H5Dread(dataset1,s2, mem_dataspace, file_dataspace, xfer_plist,buffer->data());
	H5Sclose(mem_dataspace);

	add_entries_to_tables(s,buffer,pre,offset);

	offset_r += nrecords;
    }
    
  delete buffer;
  H5Tclose(s2);
  H5Tclose(s1);
  H5Sclose(file_dataspace);
  H5Sclose(attr_space[0]);
  H5Dclose(dataset1);
  H5Aclose(attr_id);
  H5Fclose(fid);

}

template<typename KeyT,typename ValueT,typename hashfcn,typename equalfcn>
void hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::flush_table_file(int offset)
{
 std::string fname = "file";
 fname += filename+".h5";

 if(!file_exists) return;

 if(myrank==0) std::cout <<" filename = "<<filename<<" attr = "<<attributename<<std::endl;

 hid_t xfer_plist = H5Pcreate(H5P_DATASET_XFER);
 hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
 H5Pset_fapl_mpio(fapl,MPI_COMM_WORLD, MPI_INFO_NULL);
 H5Pset_dxpl_mpio(xfer_plist, H5FD_MPIO_COLLECTIVE);

 hid_t xfer_plist2 = H5Pcreate(H5P_DATASET_XFER);
 H5Pset_dxpl_mpio(xfer_plist2,H5FD_MPIO_INDEPENDENT);

 hsize_t chunkdims[1];
 chunkdims[0] = 8192;
 hsize_t maxdims[1];
 maxdims[0] = (hsize_t)H5S_UNLIMITED;

 hid_t dataset_pl = H5Pcreate(H5P_DATASET_CREATE);

 int ret = H5Pset_chunk(dataset_pl,1,chunkdims);

 hid_t fid = H5Fopen(fname.c_str(),H5F_ACC_RDWR,fapl);

 hsize_t adims[1];
 adims[0] = 8;
 hid_t s1 = H5Tarray_create(H5T_NATIVE_CHAR,1,adims);
 hid_t s2 = H5Tcreate(H5T_COMPOUND,sizeof(struct keydata));
 H5Tinsert(s2,"key",HOFFSET(struct keydata,ts),H5T_NATIVE_UINT64);
 H5Tinsert(s2,"value",HOFFSET(struct keydata,data),s1);
 hsize_t attr_size[1];
 attr_size[0] = MAXBLOCKS*4+4;
 const char *attrname[1];
 hid_t attr_space[1];
 attr_space[0] = H5Screate_simple(1, attr_size, NULL);

 attrname[0] = "Datasizes";
  
 std::string data_string = "Data1";
 hid_t dataset1 = H5Dopen2(fid,data_string.c_str(), H5P_DEFAULT);

 hid_t attr_id = H5Aopen(dataset1,attrname[0],H5P_DEFAULT);
 std::vector<uint64_t> attrs;
 attrs.resize(attr_size[0]);

 ret = H5Aread(attr_id,H5T_NATIVE_UINT64,attrs.data());
 
 std::vector<std::vector<KeyT>> *keys = new std::vector<std::vector<KeyT>> ();
 std::vector<std::vector<ValueT>> *timestamps = new std::vector<std::vector<ValueT>> ();

 int numblocks = attrs[3];
 int pos = 4;

 if(myrank==0) std::cout <<" total records = "<<attrs[0]<<" total size = "<<attrs[0]*16<<std::endl;

 uint64_t maxts = attrs[pos+(numblocks-1)*4+1];

 int key_pre = 0;
 int total_keys = 0;

 std::vector<struct KeyIndex<KeyT>> KeyTimestamps;

 get_entries_from_tables(KeyTimestamps,key_pre,total_keys,maxts); 

 std::vector<struct KeyIndex<int>> Timestamp_order;

 for(int i=0;i<KeyTimestamps.size();i++)
 {
   struct KeyIndex<int> pt;
   pt.key = i;
   pt.index = KeyTimestamps[i].index;
   Timestamp_order.push_back(pt);
 }

 std::sort(Timestamp_order.begin(),Timestamp_order.end(),compareIndex<int>);


 int block_id = 0;
 std::vector<int> block_ids;
 int i=0;

 int min_block = INT_MAX;
 int max_block = INT_MIN;

 std::vector<std::vector<int>> block_index;

 block_index.resize(numblocks);

 block_ids.resize(Timestamp_order.size());
 std::fill(block_ids.begin(),block_ids.end(),0);

 
 while(i < Timestamp_order.size())
 {
   if(block_id >= numblocks) break;
   uint64_t ts = Timestamp_order[i].index; 
   uint64_t minkey = attrs[pos+block_id*4+0];
   uint64_t maxkey = attrs[pos+block_id*4+1];
   if(ts >= minkey && ts <= maxkey) 
   {
	block_ids.push_back(block_id); 
	block_index[block_id].push_back(i);
	i++;
	if(min_block > block_id) min_block = block_id;
	if(max_block < block_id) max_block = block_id;
   }
   else if(ts < minkey) 
   {
	   block_ids[Timestamp_order[i].key] = -1;
	   i++;
   }
   else if(ts > maxkey) block_id++;
 }


 while(i < Timestamp_order.size())
 {
    block_ids[Timestamp_order[i].key] = -1;
    i++;
 }

 hid_t file_dataspace = H5Dget_space(dataset1);

 std::vector<struct keydata> *buffer = new std::vector<struct keydata> ();

 for(int i=0;i<block_index.size();i++)
 {
    if(block_index[i].size() > 0)
    {

    block_id = i;

    int nrecords = attrs[pos+block_id*4+3];

    hsize_t pre = 0;
    for(int k=0;k<block_id;k++)
    {
           pre += attrs[pos+k*4+3];
    }
    
    hsize_t blocksize = nrecords;
        
    buffer->clear();
    buffer->resize(blocksize);

    ret = H5Sselect_hyperslab(file_dataspace, H5S_SELECT_SET,&pre,NULL,&blocksize,NULL);
    hid_t mem_dataspace = H5Screate_simple(1,&blocksize, NULL);
    ret = H5Dread(dataset1,s2, mem_dataspace, file_dataspace, xfer_plist2,buffer->data());
    H5Sclose(mem_dataspace);

    int k = 0;
    for(int j=0;j<block_index[i].size();j++)
    {
	int p = block_index[i][j];
	if(k < buffer->size())
	{
	  while(Timestamp_order[p].index > (*buffer)[k].ts)
	  {
		  k++;
		  if(k==buffer->size()) break;
	  }
	  if(k==buffer->size()) break;
	  if(Timestamp_order[p].index == (*buffer)[k].ts)
	  {
	   Timestamp_order[p].index = pre+k;
	  }
	  else if(Timestamp_order[p].index < (*buffer)[k].ts)
	  {
		block_ids[Timestamp_order[p].key] = -1;

	  }
	}
    }
    }


 } 

 delete buffer;

 std::vector<struct KeyIndex<KeyT>> KeyTimestamps_s;

 for(int i=0;i<Timestamp_order.size();i++)
 {
	int p = Timestamp_order[i].key;
	KeyTimestamps[p].index = Timestamp_order[i].index;
 }

 for(int i=0;i<KeyTimestamps.size();i++)
 {
	if(block_ids[i] != -1)
	{
           KeyTimestamps_s.push_back(KeyTimestamps[i]);
	}
 }

 KeyTimestamps.clear(); block_ids.clear();

 std::vector<int> m_size_t(numprocs);
 std::vector<int> msizes(numprocs);

 std::fill(m_size_t.begin(),m_size_t.end(),0);
 std::fill(msizes.begin(),msizes.end(),0);
 m_size_t[myrank] = KeyTimestamps_s.size();

 MPI_Allreduce(m_size_t.data(),msizes.data(),numprocs,MPI_INT,MPI_SUM,MPI_COMM_WORLD);

 total_keys = 0;
 for(int i=0;i<msizes.size();i++) total_keys += msizes[i];

 hsize_t attrsize[1];
 attrsize[0] = totalsize*2+4;
 hid_t attrspace[1];
 const char *attrname_k[1];
 std::string attr_t = attributename+"attr";
 attrname_k[0] = attr_t.c_str();
 attrspace[0] = H5Screate_simple(1, attrsize, NULL);

 if(myrank==0) std::cout <<" total_keys = "<<total_keys<<std::endl;

 hsize_t totalkeys = (hsize_t) total_keys;

 std::string d_string = attributename;
 
 if(!H5Lexists(fid,d_string.c_str(),H5P_DEFAULT))
 {

   std::vector<int> numkeys(2*maxsize);
   std::fill(numkeys.begin(),numkeys.end(),0);

   int prefix = 0;
   for(int i=0;i<myrank;i++) 
 	   prefix += msizes[i];

   for(int i=0;i<KeyTimestamps_s.size();i++)
   {
	uint64_t hashvalue = hashfcn()(KeyTimestamps_s[i].key);
	int pos = hashvalue%maxsize;
	numkeys[2*pos]++;
   }


   hid_t file_dataspace_t = H5Screate_simple(1,&totalkeys,maxdims);

   std::string dtable = attributename+"attr";

   hsize_t tsize = 2*totalsize;

   hid_t file_dataspace_table = H5Screate_simple(1,&tsize,maxdims);

   hid_t dataset_t = H5Dcreate(fid,dtable.c_str(),H5T_NATIVE_INT,file_dataspace_table,H5P_DEFAULT,dataset_pl,H5P_DEFAULT);

   hid_t dataset_k = H5Dcreate(fid,d_string.c_str(),kv1,file_dataspace_t, H5P_DEFAULT,dataset_pl, H5P_DEFAULT);

   hsize_t offset_w = 0;

   for(int i=0;i<myrank;i++)
	 offset_w += msizes[i];

   hsize_t kblocksize = KeyTimestamps_s.size();

   hsize_t toffset = pre_table*2*pow(2,nbits_r);
   hsize_t lsize = 2*maxsize;

   numkeys[1] = prefix;
   for(int i=1;i<maxsize;i++)
	   numkeys[2*i+1] = numkeys[2*(i-1)+1]+numkeys[2*(i-1)];

   ret = H5Sselect_hyperslab(file_dataspace_table,H5S_SELECT_SET,&toffset,NULL,&lsize,NULL);
   hid_t mem_dataspace_t = H5Screate_simple(1,&lsize,NULL);
   ret = H5Dwrite(dataset_t,H5T_NATIVE_INT,mem_dataspace_t,file_dataspace_table,xfer_plist,numkeys.data());

   ret = H5Sselect_hyperslab(file_dataspace_t,H5S_SELECT_SET,&offset_w,NULL,&kblocksize,NULL); 
   hid_t mem_dataspace = H5Screate_simple(1,&kblocksize,NULL);
   ret = H5Dwrite(dataset_k,kv1,mem_dataspace,file_dataspace_t,xfer_plist,KeyTimestamps_s.data());
   
   //hid_t attrid_k = H5Acreate(dataset_k, attrname_k[0], H5T_NATIVE_INT, attrspace[0], H5P_DEFAULT, H5P_DEFAULT);
   //ret = H5Awrite(attrid_k,H5T_NATIVE_INT,attrdata.data());

   H5Sclose(mem_dataspace);
   H5Sclose(file_dataspace_t);
   H5Sclose(file_dataspace_table);
   H5Dclose(dataset_t);
   //H5Aclose(attrid_k);
   H5Dclose(dataset_k);
 }
 else
 {

    std::string dtable_str = attributename+"attr";
    hid_t dataset_t = H5Dopen(fid,dtable_str.c_str(),H5P_DEFAULT);

    hid_t file_dataspace_table = H5Dget_space(dataset_t);

    hid_t dataset_k = H5Dopen2(fid,d_string.c_str(),H5P_DEFAULT);
    
    hsize_t tsize = 2*totalsize;

    hid_t mem_dataspace_t = H5Screate_simple(1,&tsize,NULL);

    hsize_t offset_t = 2*pre_table*pow(2,nbits_r);
    
    std::vector<int> numkeys(2*totalsize);
    std::fill(numkeys.begin(),numkeys.end(),0);

    hsize_t offsett =0;

    ret = H5Sselect_hyperslab(file_dataspace_table,H5S_SELECT_SET,&offsett,NULL,&tsize,NULL);
    ret = H5Dread(dataset_t,H5T_NATIVE_INT,mem_dataspace_t,file_dataspace_table,xfer_plist,numkeys.data());

    hsize_t prev_keys = 0;
    for(int i=0;i<maxsize;i++) prev_keys += numkeys[offset_t+2*i];

    hsize_t prevtotal = 0;
    for(int i=0;i<totalsize;i++) prevtotal += numkeys[2*i];

    hsize_t newsize = prevtotal+totalkeys;

    H5Dset_extent(dataset_k,&newsize);
    
    std::vector<struct KeyIndex<KeyT>> preKeyTimestamps;

    preKeyTimestamps.resize(prev_keys);

    hsize_t offset_prev = numkeys[offset_t+1]; 

    hid_t file_dataspace_p = H5Dget_space(dataset_k);
    hid_t mem_dataspace_p = H5Screate_simple(1,&prev_keys,NULL);

    ret = H5Sselect_hyperslab(file_dataspace_p,H5S_SELECT_SET,&offset_prev,NULL,&prev_keys,NULL);
    ret = H5Dread(dataset_k,kv1,mem_dataspace_p,file_dataspace_p,xfer_plist,preKeyTimestamps.data());

    hid_t file_dataspace_t = H5Dget_space(dataset_k);

    std::vector<int> numkeys_n(2*maxsize);
    std::fill(numkeys_n.begin(),numkeys_n.end(),0);

    std::vector<struct KeyIndex<KeyT>> mergeKeyTimestamps = merge_keyoffsets(preKeyTimestamps,KeyTimestamps_s,numkeys_n);

    hsize_t offset_w = numkeys_n[1];
    hsize_t kblocksize = mergeKeyTimestamps.size();
    hid_t mem_dataspace = H5Screate_simple(1,&kblocksize,NULL);

    ret = H5Sselect_hyperslab(file_dataspace_t,H5S_SELECT_SET,&offset_w,NULL,&kblocksize,NULL);
    ret = H5Dwrite(dataset_k,kv1,mem_dataspace,file_dataspace_t,xfer_plist,mergeKeyTimestamps.data());

    offset_w = 2*pre_table*pow(2,nbits_r);
    kblocksize = 2*maxsize;
    ret = H5Sselect_hyperslab(file_dataspace_table,H5S_SELECT_SET,&offset_w,NULL,&kblocksize,NULL);
    hid_t memdataspace_tt = H5Screate_simple(1,&kblocksize,NULL);
    ret = H5Dwrite(dataset_t,H5T_NATIVE_INT,memdataspace_tt,file_dataspace_table,xfer_plist,numkeys_n.data());

    H5Sclose(memdataspace_tt);
    H5Sclose(mem_dataspace_p);
    H5Sclose(file_dataspace_p);
    H5Sclose(mem_dataspace);
    H5Sclose(mem_dataspace_t);
    H5Sclose(file_dataspace_t);
    H5Sclose(file_dataspace_table);
    H5Dclose(dataset_k);
    H5Dclose(dataset_t);
 }

 cached_keyindex_mt.clear();
 cached_keyindex.clear();

 H5Dclose(dataset1);
 //H5Sclose(attrspace[0]);
 H5Sclose(attr_space[0]);
 H5Sclose(file_dataspace);
 H5Aclose(attr_id);
 H5Tclose(s2);
 H5Tclose(s1);
 H5Pclose(dataset_pl);
 H5Fclose(fid);

}

template<typename KeyT,typename ValueT,typename hashfcn,typename equalfcn>
std::vector<struct KeyIndex<KeyT>> hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::merge_keyoffsets(std::vector<struct KeyIndex<KeyT>> &prevlist,std::vector<struct KeyIndex<KeyT>> &newlist,std::vector<int> &numkeys)
{
 
   std::vector<struct KeyIndex<KeyT>> result_list;

   int i=0,j=0;

   while(true)
   {
     if(i==prevlist.size()||j==newlist.size()) break;

     uint64_t key1 = hashfcn()(prevlist[i].key);
     uint64_t key2 = hashfcn()(newlist[j].key);
     int pos1 = key1%maxsize;
     int pos2 = key2%maxsize;

     if(pos1 < pos2)
     {
	result_list.push_back(prevlist[i]);
	numkeys[2*pos1]++;
	i++;
     }
     else if(pos2 < pos1)
     {
	result_list.push_back(newlist[j]);
	numkeys[2*pos2]++;
	j++;
     }
     else
     {
	if(key1 < key2)
	{
	    numkeys[2*pos1]+=2;
	    result_list.push_back(prevlist[i]);
	    i++;
	    result_list.push_back(newlist[j]);
	    j++;
	}
	else
	{
	    numkeys[2*pos1]+=2;
	    result_list.push_back(newlist[j]);
	    j++;
	    result_list.push_back(prevlist[i]);
	    i++;
	}

     }

   }

   while(j<newlist.size())
   {
	result_list.push_back(newlist[j]);j++;
   }

   while(i<prevlist.size())
   {	
	   result_list.push_back(prevlist[i]);
	   i++;
   }

   MPI_Request *reqs = (MPI_Request*)std::malloc(2*numprocs*sizeof(MPI_Request));

   std::vector<int> recv_size(numprocs);
   std::fill(recv_size.begin(),recv_size.end(),0);

   int send_size = result_list.size();

   int nreqs = 0;


   for(int i=0;i<numprocs;i++)
   {
	MPI_Isend(&send_size,1,MPI_INT,i,tag,MPI_COMM_WORLD,&reqs[nreqs]);
	nreqs++;
	MPI_Irecv(&recv_size[i],1,MPI_INT,i,tag,MPI_COMM_WORLD,&reqs[nreqs]);
	nreqs++;
   }

   MPI_Waitall(nreqs,reqs,MPI_STATUS_IGNORE);

   int prefix = 0;
   for(int i=0;i<myrank;i++) prefix += recv_size[i];

   numkeys[1] = prefix;
   for(int i=1;i<maxsize;i++) 
     numkeys[2*i+1] = numkeys[2*(i-1)+1]+numkeys[2*(i-1)];

   std::free(reqs);

   return result_list;
}

template<typename KeyT,typename ValueT,typename hashfcn,typename equalfcn>
void hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::add_entries_to_tables(std::string &s,std::vector<struct keydata> *buffer,uint64_t f_offset,int offset)
{
  std::vector<int> send_count,recv_count;

  if(invlist==nullptr) return;

  send_count.resize(numprocs); recv_count.resize(numprocs);
  std::fill(send_count.begin(),send_count.end(),0);
  std::fill(recv_count.begin(),recv_count.end(),0); 

  MPI_Request *reqs = (MPI_Request *)std::malloc(2*numprocs*sizeof(MPI_Request));

  int nreq = 0;
  
  std::vector<std::vector<double>> send_buffers;
  std::vector<std::vector<double>> recv_buffers;
  send_buffers.resize(numprocs); recv_buffers.resize(numprocs);

  int recsize = sizeof(struct event);  
  uint64_t offsets = f_offset;

  for(int i=0;i<buffer->size();i++)
  {
     KeyT key = *(KeyT*)((*buffer)[i].data+offset);
     int p = partition_no(key); 
     send_count[p]+=2;
     send_buffers[p].push_back((double)key);
     send_buffers[p].push_back(offsets);
     offsets += (uint64_t)recsize;
  }

  
  for(int i=0;i<numprocs;i++)
  {
     MPI_Isend(&send_count[i],1,MPI_INT,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
     nreq++;
     MPI_Irecv(&recv_count[i],1,MPI_INT,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
     nreq++;
  }

  MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

  int total_recv = 0;
  for(int i=0;i<numprocs;i++)
  {
     recv_buffers[i].resize(recv_count[i]);
     total_recv += recv_count[i];
  }

  nreq = 0;
  for(int i=0;i<numprocs;i++)
  {
    
    MPI_Isend(send_buffers[i].data(),send_count[i],MPI_DOUBLE,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
    nreq++;
    MPI_Irecv(recv_buffers[i].data(),recv_count[i],MPI_DOUBLE,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
    nreq++;
  }

  MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

  for(int i=0;i<numprocs;i++)
  {
	for(int j=0;j<recv_buffers[i].size();j+=2)
	{
	   KeyT key = (KeyT)(recv_buffers[i][j]);
	   ValueT offset = (ValueT)recv_buffers[i][j+1];
           invlist->bm->insert(key,offset);
	}
  }

  std::free(reqs);
}

template<typename KeyT,typename ValueT,typename hashfcn,typename equalfcn>
void hdf5_invlist<KeyT,ValueT,hashfcn,equalfcn>::get_entries_from_tables(std::vector<struct KeyIndex<KeyT>> &KeyTimestamps,int &key_b,int &numkeys,uint64_t maxts)
{

	if(invlist==nullptr) return;

	std::vector<std::vector<KeyT>> *keys = new std::vector<std::vector<KeyT>> ();
	std::vector<std::vector<ValueT>> *offsets = new std::vector<std::vector<ValueT>> ();

	invlist->bm->fetch_clear_map(keys,offsets,maxts);

	MPI_Request *reqs = (MPI_Request *)std::malloc(2*numprocs*sizeof(MPI_Request));
	int nreq = 0;

	int numentries = 0;
	for(int i=0;i<keys->size();i++)
		numentries += (*keys)[i].size();

	std::vector<int> recv_counts(numprocs);
	int send_count = 0;
	send_count = numentries;

	for(int i=0;i<numprocs;i++)
	{
	   MPI_Isend(&send_count,1,MPI_INT,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
	   nreq++;
	   MPI_Irecv(&recv_counts[i],1,MPI_INT,i,tag,MPI_COMM_WORLD,&reqs[nreq]);
	   nreq++;
	}

	MPI_Waitall(nreq,reqs,MPI_STATUS_IGNORE);

	numkeys = 0;
	for(int i=0;i<recv_counts.size();i++)
	   numkeys += recv_counts[i];

	if(myrank==0) std::cout <<" numkeys = "<<numkeys<<std::endl;

	key_b = 0;
	for(int i=0;i<myrank;i++)
	  key_b += recv_counts[i];

	for(int i=0;i<keys->size();i++)
	{
	  for(int j=0;j<(*keys)[i].size();j++)
	  {
		struct KeyIndex<KeyT> ks;
		ks.key = (*keys)[i][j];
		ks.index = (*offsets)[i][j];
		KeyTimestamps.push_back(ks);
	  }

	}

	delete keys;
	delete offsets;

	std::free(reqs);
}
