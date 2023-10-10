#include "PSClient.h"

void pubsubclient::create_pub_sub_service(std::string &s,std::vector<int> &p,std::vector<int> &sb)
{
   auto r = table_roles.find(s);
   if(r == table_roles.end())
   { 
       assert (subscribers.size()==publishers.size());
       int numtables = subscribers.size();

       std::pair<std::string,int> pn(s,numtables);

       subscribers.push_back(sb);
       publishers.push_back(p);

       table_roles.insert(pn);

       int role = -1;
       if(std::find(sb.begin(),sb.end(),myrank)!=sb.end())
	  role = 0;
       if(std::find(p.begin(),p.end(),myrank)!=p.end())
	 role = (role == -1) ?  1 : 2;
       std::pair<std::string,int> np(s,role);
       client_role.insert(np);	       
  }
  else
  {
	int n = r->second;

	subscribers[n].assign(sb.begin(),sb.end());
	publishers[n].assign(p.begin(),p.end());
	int role = -1;
	if(std::find(sb.begin(),sb.end(),myrank) != sb.end())
	   role = 0;
	if(std::find(p.begin(),p.end(),myrank) != p.end())
	   role = (role == -1) ? 1 : 2;
	
	auto r1 = client_role.find(s);
	r1->second = role;
  }
}

void pubsubclient::add_subs(std::string &s,std::vector<int> &sb)
{
    auto r = table_roles.find(s);
    if(r != table_roles.end())
    {
	int n = r->second;

	for(int i=0;i<sb.size();i++)
	{
	   if(std::find(subscribers[n].begin(),subscribers[n].end(),sb[i])==subscribers[n].end())
		subscribers[n].push_back(sb[i]);
	}
	
	if(std::find(sb.begin(),sb.end(),myrank) != sb.end())
	{
	   auto r1 = client_role.find(s);

	   int role = r1->second;
	   if(role==1) role = 2;
	   r1->second = role;
	}
    }
}

void pubsubclient::add_pubs(std::string &s,std::vector<int> &p)
{
   auto r = table_roles.find(s);

   if(r != table_roles.end())
   {
	int n = r->second;

	for(int i=0;i<p.size();i++)
	{
	  if(std::find(publishers[n].begin(),publishers[n].end(),p[i])==publishers[n].end())
		publishers[n].push_back(p[i]);
	}

	if(std::find(p.begin(),p.end(),myrank) != p.end())
	{
          auto r1 = client_role.find(s);

	  int role = r1->second;
	  if(role==0) role = 2;
	  r1->second = role;
	}
   }
}

void pubsubclient::remove_pubs(std::string &s,std::vector<int> &p)
{
   auto r = table_roles.find(s);

   if(r != table_roles.end())
   {
	int n = r->second;

	std::vector<int> temp;
	for(int i=0;i<publishers[n].size();i++)
	{
	  if(std::find(p.begin(),p.end(),publishers[n][i])==p.end())
	    temp.push_back(publishers[n][i]);
	}

	publishers[n].assign(temp.begin(),temp.end());
   }

   if(std::find(p.begin(),p.end(),myrank)!=p.end())
   {
	auto r1 = client_role.find(s);

	int role = r1->second;
	if(role==2) role = 0;
	else if(role==1) role = -1;
   }
}

void pubsubclient::remove_subs(std::string &s,std::vector<int> &sb)
{

	auto r = table_roles.find(s);
	if(r != table_roles.end())
	{
	   int n = r->second;
	   std::vector<int> temp;
	   for(int i=0;i<subscribers[n].size();i++)
	   {
		if(std::find(sb.begin(),sb.end(),subscribers[n][i])==sb.end())
		   temp.push_back(subscribers[n][i]);
	   }

	   subscribers[n].assign(temp.begin(),temp.end());

	   if(std::find(sb.begin(),sb.end(),myrank)!=sb.end())
	   {
		auto r1 = client_role.find(s);
		int role = r1->second;
		if(role == 2) role = 1;
		else if(role == 0) role = -1;
		r1->second = role;
	   }
	}
}

void pubsubclient::add_message_cache(std::string &s,int nmessages,int msg_size)
{
   auto r = table_roles.find(s);

   if(r != table_roles.end())
   {
	auto r1 = mcnum.find(s);

	if(r1 == mcnum.end())
	{
	  message_cache *m = new message_cache(numprocs,myrank,s,ds,msg_size,nmessages);
	  mcs.push_back(m);
	  std::pair<std::string,int> p(s,mcs.size()-1);
	  mcnum.insert(p); 
	}
   }
}
