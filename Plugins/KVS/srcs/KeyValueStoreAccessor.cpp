
template<typename T,typename N>
int KeyValueStoreAccessor::add_new_inverted_list(std::string &table,std::string &attr_name,int size,int ntables,N &emptykey,data_server_client *d,KeyValueStoreIO *io,int c)
{
      T *invlist = new T(numprocs,myrank,size,ntables,emptykey,table,attr_name,d,io,c); 
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
