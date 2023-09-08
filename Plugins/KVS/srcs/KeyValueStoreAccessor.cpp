
template<typename T,typename N>
int KeyValueStoreAccessor::add_new_inverted_list(std::string &table,std::string &attr_name,int size,int ntables,N &emptykey,data_server_client *d,KeyValueStoreIO *io,int c)
{
      T *invlist = new T(numprocs,myrank,size,ntables,emptykey,table,attr_name,d,io,c);

      std::string type = md.get_type(attr_name);
      int keytype = 0;
      if(type.compare("int")==0) keytype = 0;
      else if(type.compare("unsignedlong")==0) keytype=1;
      else if(type.compare("float")==0) keytype=2;
      else if(type.compare("double")==0) keytype=3;
      
      kio->add_query_service(keytype,(void*)invlist); 
      invlist->bind_functions();

      std::pair<std::string,void*> sp;
      sp.first = attr_name;
      sp.second = (void*)invlist;
      lists.push_back(sp);
      std::pair<std::string,int> ip;
      ip.first = attr_name;
      ip.second = lists.size()-1;
      secondary_attributes.insert(ip);
      return lists.size()-1;
}
template<typename T>
bool KeyValueStoreAccessor::delete_inverted_list(int n)
{
    if(n < lists.size())
    {
	T *invlist = reinterpret_cast<T*>(lists[n].second);
	delete invlist;
	return true;
    }
    return false;
}
template<typename T,typename N>
bool KeyValueStoreAccessor::insert_entry(int pos, N&key,uint64_t &ts)
{
   if(pos >= lists.size()) return false;

   T *invlist = reinterpret_cast<T*>(lists[pos].second);
   bool b = invlist->put_entry(key,ts);
   return b;
}

template<typename T,typename N,typename M>
bool KeyValueStoreAccessor::Put(int pos,std::string &s,N &key, M &value)
{
   if(pos >= lists.size()) return false;

   char keyarray[8];
   N key_t = key;
   char *arr = (char *)(&key_t);
   for(int i=0;i<8;i++)
   {
	keyarray[i] = arr[i];
   }
   std::string key_s(keyarray,8);
   std::string data = key_s+value;  
     
   std::vector<uint64_t> ts;
   ts = if_q->PutEmulatorEvent(s,data,myrank);
   bool b = false;
   if(ts[0] != UINT64_MAX)
   {
	T *invlist = reinterpret_cast<T*>(lists[pos].second);
	b = invlist->put_entry(key,ts[0]);
	if(b) inserts++;
	return true;
   }
   else if(ts[1]==2) return false;
   else return true;
}


template<typename T,typename N>
bool KeyValueStoreAccessor::openfilerw(int pos,std::string &s)
{
   std::string filename = "file";
   filename += s+".h5";

   bool b = if_q->CheckFileExistence(filename,myrank);

   T *invlist = reinterpret_cast<T*>(lists[pos].second);
   bool ret = invlist->open_file(b);
   return ret;
}	

template<typename T,typename N>
void KeyValueStoreAccessor::closefilerw(int pos)
{
  T *invlist = reinterpret_cast<T*>(lists[pos].second);
  invlist->close_file();
}

template<typename T,typename N>
bool KeyValueStoreAccessor::Get(int pos,std::string &s,N &key)
{
   if(pos >= lists.size()) return false;

   uint64_t ts = UINT64_MAX;
   bool b = false;
   std::vector<uint64_t> values;

   T *invlist = reinterpret_cast<T*>(lists[pos].second);
   int pid = invlist->get_entry(key,values);

   if(values.size()>0)
   {
     ts = values[0];
     std::string eventstring = if_q->GetEmulatorEvent(s,ts,myrank);
     if(eventstring.length()==0)
     {	
	if(!invlist->CheckLocalFileExists())
	{
	   std::string filename = "file";
	   filename += s+".h5";
	   if(if_q->CheckFileExistence(filename,myrank))
	   {
		invlist->LocalFileExists();
	   }
	}
	
	invlist->AddPending(key,values,pid);
     }
		
   }
   else
   {
	pid = invlist->partition_no(key);

	if(!invlist->CheckLocalFileExists())
	{
	   std::string filename = "file";
	   filename +=  s + ".h5";
	   if(if_q->CheckFileExistence(filename,myrank))
	   {
		invlist->LocalFileExists();
	   }
	}
	invlist->AddPending(key,values,pid);	

   }
   return false;

}

template<typename T,typename N>
bool KeyValueStoreAccessor::Emulator_Request(int pos, std::string &s, N&key)
{
   if(pos >= lists.size()) return false;
   std::vector<uint64_t> values;

   T *invlist = reinterpret_cast<T*>(lists[pos].second);
   int ret = invlist->get_entry(key,values);

   bool b = false;

   if(values.size() > 0)
   {
	struct query_req r;
        r.name = s;
	r.minkey = values[0];
	r.maxkey = values[0];
	r.sender = myrank;
	b = if_q->PutEmulatorRequest(r,myrank);	
   }
   return b;
}

template<typename T,typename N>
std::vector<uint64_t> KeyValueStoreAccessor::get_entry(int pos,N &key)
{
   std::vector<uint64_t> values;
   if(pos >= lists.size()) return values;

   T *invlist = reinterpret_cast<T*>(lists[pos].second);
   int ret = invlist->get_entry(key,values);
   return values;
}

template<typename T>
void KeyValueStoreAccessor::cache_invertedtable(std::string &attr_name)
{
   int pos = get_inverted_list_index(attr_name);
   if(pos==-1) return;

   T *invlist = reinterpret_cast<T*>(lists[pos].second);

   if(!invlist->CheckLocalFileExists())
   {
	std::string filename = "file";
	filename += "table1.h5";

	if(if_q->CheckFileExistence(filename,myrank))
	{
	   invlist->LocalFileExists();
	}
   }

   invlist->cache_latest_table();
}

template<typename T>
void KeyValueStoreAccessor::flush_invertedlist(std::string &attr_name)
{
    int offset = md.locate_offset(attr_name);

    if(offset==-1) return;

    int pos = get_inverted_list_index(attr_name); 

    if(pos==-1) return;

    T *invlist = reinterpret_cast<T*>(lists[pos].second);

    if(!invlist->CheckLocalFileExists())
    {
           std::string filename = "file";
           filename += "table1.h5";
           if(if_q->CheckFileExistence(filename,myrank))
           {
                invlist->LocalFileExists();
           }
    }
    
    struct sync_request *r = new struct sync_request();
    std::string type = md.get_type(attr_name);
    int keytype = 0;
    if(type.compare("int")==0) keytype = 0;
    else if(type.compare("unsignedlong")==0) keytype=1;
    else if(type.compare("float")==0) keytype=2;
    else if(type.compare("double")==0) keytype=3;

    r->funcptr = (void*)invlist;
    r->name = md.db_name();
    r->attr_name = attr_name; 
    r->offset = offset;
    r->keytype = keytype;
    r->flush = true;

    bool ret = kio->LocalPutSyncRequest(r);

}
