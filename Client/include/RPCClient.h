//
// Created by kfeng on 12/26/22.
//

#ifndef CHRONOLOG_RPCCLIENT_H
#define CHRONOLOG_RPCCLIENT_H

#include <iostream>
#include <unordered_map>
#include <functional>
#include <thallium.hpp>
#include <utility>
#include <sys/types.h>
#include <unistd.h>
#include "macro.h"
#include "RPCFactory.h"

class RPCClient {
public:
    RPCClient() {
        LOGD("%s constructor is called", typeid(*this).name());
        rpc = std::make_shared<ChronoLogRPC>(); 
        set_prefix("ChronoLog");
        LOGD("%s constructor finishes, object created@%p in thread PID=%d",
             typeid(*this).name(), this, getpid());
    }

    ~RPCClient()
    {
    }

    int Connect(const std::string &uri, std::string &client_id,std::string &group_id, uint32_t &role, int &flags, uint64_t &clock_offset) {
        LOGD("%s in ChronoLogAdminRPCProxy at addresss %p called in PID=%d, with args: uri=%s, client_id=%s",
             __FUNCTION__, this, getpid(), uri.c_str(), client_id.c_str());
        return CHRONOLOG_RPC_CALL_WRAPPER("Connect", 0, int, uri, client_id, group_id, role, flags, clock_offset);
    }

    int Disconnect(const std::string &client_id, int &flags) {
        LOGD("%s is called in PID=%d, with args: client_id=%s, flags=%d",
             __FUNCTION__, getpid(), client_id.c_str(), flags);
        return CHRONOLOG_RPC_CALL_WRAPPER("Disconnect", 0, int, client_id, flags);
    }

    int CreateChronicle(std::string &name,
		        std::string &client_id,
                        const std::unordered_map<std::string, std::string> &attrs,
                        int &flags) {
        LOGD("%s is called in PID=%d, with args: name=%s, flags=%d, attrs=",
             __FUNCTION__, getpid(), name.c_str(), flags);
        for (auto iter = attrs.begin(); iter != attrs.end(); ++iter) {
            LOGD("%s=%s", iter->first.c_str(), iter->second.c_str());
        }
        return CHRONOLOG_RPC_CALL_WRAPPER("CreateChronicle", 0, int, name, client_id, attrs, flags);
    }

    int DestroyChronicle(std::string &name, std::string &client_id, const int &flags) {
        LOGD("%s is called in PID=%d, with args: name=%s, flags=%d", __FUNCTION__, getpid(), name.c_str(), flags);
        return CHRONOLOG_RPC_CALL_WRAPPER("DestroyChronicle", 0, int, name, client_id, flags);
    }

    int AcquireChronicle(std::string &name, std::string &client_id, const int &flags) {
        LOGD("%s is called in PID=%d, with args: name=%s, flags=%d", __FUNCTION__, getpid(), name.c_str(), flags);
        return CHRONOLOG_RPC_CALL_WRAPPER("AcquireChronicle", 0, int, name, client_id, flags);
    }

    int ReleaseChronicle(std::string &name, std::string &client_id, const int &flags) {
        LOGD("%s is called in PID=%d, with args: name=%s, flags=%d", __FUNCTION__, getpid(), name.c_str(), flags);
        return CHRONOLOG_RPC_CALL_WRAPPER("ReleaseChronicle", 0, int, name, client_id, flags);
    }
    int UpdateChroniclePermissions(std::string &name,std::string &client_id,std::string &perm)
    {
        LOGD("%s is called in PID=%d,with args: name=%s,perm=%s",__FUNCTION__,getpid(),name.c_str(),perm.c_str());
	return CHRONOLOG_RPC_CALL_WRAPPER("UpdateChroniclePermissions",0,int,name,client_id,perm);
    }
    int CreateStory(std::string &chronicle_name,
                    std::string &story_name,
		    std::string &client_id,
                    const std::unordered_map<std::string, std::string> &attrs,
                    int &flags) {
        LOGD("%s is called in PID=%d, with args: chronicle_name=%s, story_name=%s, flags=%d ,attrs=",
             __FUNCTION__, getpid(), chronicle_name.c_str(), story_name.c_str(), flags);
        for (auto iter = attrs.begin(); iter != attrs.end(); ++iter) {
            LOGD("%s=%s", iter->first.c_str(), iter->second.c_str());
        }
        return CHRONOLOG_RPC_CALL_WRAPPER("CreateStory", 0, int, chronicle_name, story_name, client_id,attrs, flags);
    }

    int DestroyStory(std::string &chronicle_name, std::string &story_name, std::string &client_id,const int &flags) {
        LOGD("%s is called in PID=%d, with args: chronicle_name=%s, story_name=%s, flags=%d",
             __FUNCTION__, getpid(), chronicle_name.c_str(), story_name.c_str(), flags);
        return CHRONOLOG_RPC_CALL_WRAPPER("DestroyStory", 0, int, chronicle_name, story_name, client_id, flags);
    }

    int AcquireStory(std::string &chronicle_name, std::string &story_name, std::string &client_id, const int &flags) {
        LOGD("%s is called in PID=%d, with args: chronicle_name=%s, story_name=%s, flags=%d",
             __FUNCTION__, getpid(), chronicle_name.c_str(), story_name.c_str(), flags);
        return CHRONOLOG_RPC_CALL_WRAPPER("AcquireStory", 0, int, chronicle_name, story_name, client_id, flags);
    }

    int ReleaseStory(std::string &chronicle_name, std::string &story_name, std::string &client_id, const int &flags) {
        LOGD("%s is called in PID=%d, with args: chronicle_name=%s, story_name=%s, flags=%d",
             __FUNCTION__, getpid(), chronicle_name.c_str(), story_name.c_str(), flags);
        return CHRONOLOG_RPC_CALL_WRAPPER("ReleaseStory", 0, int, chronicle_name, story_name, client_id, flags);
    }
    int UpdateStoryPermissions(std::string &chronicle_name,std::string &story_name,std::string &client_id,std::string &perm)
    {
	LOGD("%s is called in PID=%d with args: chronicle_name=%s story_name=%s perm=%s",__FUNCTION__,getpid(),chronicle_name.c_str(),story_name.c_str(),perm.c_str());
	return CHRONOLOG_RPC_CALL_WRAPPER("UpdateStoryPermission",0,int,chronicle_name,story_name,client_id,perm);
    }
    int GetChronicleAttr(std::string &name, const std::string &key, std::string &client_id, std::string &value) {
        LOGD("%s is called in PID=%d, with args: name=%s, key=%s", __FUNCTION__, getpid(), name.c_str(), key.c_str());
        return CHRONOLOG_RPC_CALL_WRAPPER("GetChronicleAttr", 0, int, name, key, client_id, value);
    }

    int EditChronicleAttr(std::string &name, const std::string &key, std::string &client_id, const std::string &value) {
        LOGD("%s is called in PID=%d, with args: name=%s, key=%s, value=%s",
             __FUNCTION__, getpid(), name.c_str(), key.c_str(), value.c_str());
        return CHRONOLOG_RPC_CALL_WRAPPER("EditChronicleAttr", 0, int, name, key, client_id, value);
    }
    int RequestRoleChange(std::string& client_id,uint32_t &role)
    {
	LOGD("%s is called in PID=%d, with args: client_id=%s role =%u",__FUNCTION__,getpid(),client_id.c_str(),role);
	return CHRONOLOG_RPC_CALL_WRAPPER("RequestRoleChange",0,int,client_id,role);
    }
    int AddGrouptoChronicle(std::string &chronicle_name,std::string &client_id,std::string &new_group_id)
    {
	 LOGD("%s is called in PID=%d with args: chronicle_name=%s client_id=%s, new_group_id = %s",__FUNCTION__,getpid(),chronicle_name.c_str(),client_id.c_str(),new_group_id.c_str());
	 return CHRONOLOG_RPC_CALL_WRAPPER("AddGrouptoChronicle",0,int,chronicle_name,client_id,new_group_id);
    }
    int RemoveGroupFromChronicle(std::string &chronicle_name,std::string &client_id,std::string &new_group_id)
    {
	 LOGD("%s is called in PID=%d with args: chronicle_name=%s client_id=%s, new_group_id = %s",__FUNCTION__,getpid(),chronicle_name.c_str(),client_id.c_str(),new_group_id.c_str());
	 return CHRONOLOG_RPC_CALL_WRAPPER("RemoveGroupFromChronicle",0,int,chronicle_name,client_id,new_group_id);
    }
    int AddGrouptoStory(std::string &chronicle_name,std::string &story_name,std::string &client_id,std::string &new_group_id)
    {
	LOGD("%s is called in PID=%d with args: chronicle_name=%s story_name=%s client_id=%sm new_group_id=%s",__FUNCTION__,getpid(),chronicle_name.c_str(),story_name.c_str(),client_id.c_str(),new_group_id.c_str());
	return CHRONOLOG_RPC_CALL_WRAPPER("AddGrouptoStory",0,int,chronicle_name,story_name,client_id,new_group_id);
    }
    int RemoveGroupFromStory(std::string &chronicle_name,std::string &story_name,std::string &client_id,std::string &new_group_id)
    {
	LOGD("%s is called in PID=%d with args: chronicle_name=%s story_name=%s client_id=%sm new_group_id=%s",__FUNCTION__,getpid(),chronicle_name.c_str(),story_name.c_str(),client_id.c_str(),new_group_id.c_str());
	return CHRONOLOG_RPC_CALL_WRAPPER("RemoveGroupFromStory",0,int,chronicle_name,story_name,client_id,new_group_id);
    }

private:
    void set_prefix(std::string prefix) {
        func_prefix = std::move(prefix);
        switch (CHRONOLOG_CONF->RPC_IMPLEMENTATION) {
            case CHRONOLOG_THALLIUM_SOCKETS:
            case CHRONOLOG_THALLIUM_TCP:
            case CHRONOLOG_THALLIUM_ROCE:
                func_prefix += "Thallium";
                break;
        }
    }

    ChronoLogCharStruct func_prefix;
    //ChronoLogRPC *rpc;
    std::shared_ptr<ChronoLogRPC> rpc;
};


#endif //CHRONOLOG_RPCCLIENT_H
