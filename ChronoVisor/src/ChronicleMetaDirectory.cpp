#include <ChronicleMetaDirectory.h>
#include "city.h"
#include <chrono>
#include <unistd.h>
#include <mutex>
#include <typedefs.h>
#include <ClientRegistryManager.h>

namespace chl = chronolog;

ChronicleMetaDirectory::ChronicleMetaDirectory()
{
    LOG_DEBUG("[ChronicleMetaDirectory] Constructor is called. Object created at {} in thread PID={}"
         , static_cast<const void*>(this), getpid());
    chronicleMap_ = new std::unordered_map <uint64_t, Chronicle*>();
}

ChronicleMetaDirectory::~ChronicleMetaDirectory()
{
    delete chronicleMap_;
}

/**
 * Create a Chronicle
 * @param name: name of the Chronicle
 * @param attrs: attributes associated with the Chronicle
 * @return chronolog::to_int(chronolog::ClientErrorCode::Success) if succeed to create the Chronicle \n
 *         chronolog::to_int(chronolog::ClientErrorCode::ChronicleExists) if a Chronicle with the same name already exists \n
 *         chronolog::to_int(chronolog::ClientErrorCode::Unknown) otherwise
 */
int ChronicleMetaDirectory::create_chronicle(const std::string &name
                                             , const std::map <std::string, std::string> &attrs)
{
    LOG_DEBUG("[ChronicleMetaDirectory] Creating Chronicle Name={}", name.c_str());
    for(auto iter = attrs.begin(); iter != attrs.end(); ++iter)
    {
        LOG_DEBUG("[ChronicleMetaDirectory] Attribute of Chronicle {}: {}={}", name.c_str(), iter->first.c_str()
             , iter->second.c_str());
    }
    std::lock_guard <std::mutex> chronicleMapLock(g_chronicleMetaDirectoryMutex_);
    /* Check if Chronicle already exists, fail if true */
    uint64_t cid;
    cid = CityHash64(name.c_str(), name.length());
    auto chronicleMapRecord = chronicleMap_->find(cid);
    if(chronicleMapRecord != chronicleMap_->end())
    {
        LOG_WARNING("[ChronicleMetaDirectory] A Chronicle with the same ChronicleName={} already exists", name.c_str());
        return chronolog::to_int(chronolog::ClientErrorCode::ChronicleExists);
    }
    auto*pChronicle = new Chronicle();
    pChronicle->setName(name);
    pChronicle->setCid(cid);
    auto res = chronicleMap_->emplace(cid, pChronicle);
    if(res.second)
    {
        LOG_DEBUG("[ChronicleMetaDirectory] ChronicleName={} is created", name.c_str());
        return chronolog::to_int(chronolog::ClientErrorCode::Success);
    }
    else
    {
        LOG_ERROR("[ChronicleMetaDirectory] Fail to create ChronicleName={}", name.c_str());
        return chronolog::to_int(chronolog::ClientErrorCode::Unknown);
    }
}

/**
 * Destroy a Chronicle \n
 * No need to check its Stories. Users are required to release all Stories before releasing a Chronicle
 * @param name: name of the Chronicle
 * @param flags: flags
 * @return chronolog::to_int(chronolog::ClientErrorCode::Success) if succeed to destroy the Chronicle \n
 *         chronolog::to_int(chronolog::ClientErrorCode::NotExist) if the Chronicle does not exist \n
 *         chronolog::to_int(chronolog::ClientErrorCode::Acquired) if the Chronicle is acquired by others and cannot be destroyed \n
 *         chronolog::to_int(chronolog::ClientErrorCode::Unknown) otherwise
 */
int ChronicleMetaDirectory::destroy_chronicle(const std::string &name)
{
    LOG_DEBUG("[ChronicleMetaDirectory] Destroying ChronicleName={}", name.c_str());
    std::lock_guard <std::mutex> chronicleMapLock(g_chronicleMetaDirectoryMutex_);
    /* First check if Chronicle exists, fail if false */
    uint64_t cid;
    cid = CityHash64(name.c_str(), name.length());
    auto chronicleMapRecord = chronicleMap_->find(cid);
    if(chronicleMapRecord != chronicleMap_->end())
    {
        /* Check if Chronicle is acquired by checking if each of its Story is acquired, fail if true */
        Chronicle*pChronicle = chronicleMapRecord->second;
        auto storyMap = pChronicle->getStoryMap();
        int ret = chronolog::to_int(chronolog::ClientErrorCode::Success);
        for(auto storyMapRecord: storyMap)
        {
            Story*pStory = storyMapRecord.second;
            if(!pStory->getAcquirerMap().empty())
            {
                ret = chronolog::to_int(chronolog::ClientErrorCode::Acquired);
                for(const auto &acquirerMapRecord: pStory->getAcquirerMap())
                {
                    LOG_DEBUG("[ChronicleMetaDirectory] StoryID={} in Chronicle Name={} is still acquired by ClientID={}"
                         , pStory->getSid(), name.c_str(), acquirerMapRecord.first);
                }
            }
        }
        if(ret != chronolog::to_int(chronolog::ClientErrorCode::Success))
        {
            return ret;
        }
        if(pChronicle->getAcquisitionCount() != 0)
        {
            LOG_ERROR("[ChronicleMetaDirectory] Something is wrong, no Story is being acquired, but ChronicleName={}'s AcquisitionCount is not 0"
                 , name.c_str());
            return chronolog::to_int(chronolog::ClientErrorCode::Unknown);
        }
        /* No Stories in Chronicle is acquired, ready to destroy */
        delete pChronicle;
        auto nErased = chronicleMap_->erase(cid);
        if(nErased == 1)
        {
            LOG_DEBUG("[ChronicleMetaDirectory] ChronicleName={} is destroyed", name.c_str());
            return chronolog::to_int(chronolog::ClientErrorCode::Success);
        }
        else
        {
            LOG_ERROR("[ChronicleMetaDirectory] Fail to destroy ChronicleName={}", name.c_str());
            return chronolog::to_int(chronolog::ClientErrorCode::Unknown);
        }
    }
    else
    {
        LOG_WARNING("[ChronicleMetaDirectory] ChronicleName={} does not exist", name.c_str());
        return chronolog::to_int(chronolog::ClientErrorCode::NotExist);
    }
}


/**
 * Destroy a Story
 * @param chronicle_name: name of the Chronicle that the Story belongs to
 * @param story_name: name of the Story
 * @param flags: flags
 * @return chronolog::to_int(chronolog::ClientErrorCode::Success) if succeed to destroy the Story \n
 *         chronolog::to_int(chronolog::ClientErrorCode::Acquired) if the Story is acquired by others and cannot be destroyed \n
 *         chronolog::to_int(chronolog::ClientErrorCode::NotExist) if the Chronicle does not exist \n
 *         chronolog::to_int(chronolog::ClientErrorCode::Unknown) otherwise
 */
int ChronicleMetaDirectory::destroy_story(std::string const &chronicle_name, const std::string &story_name)
{
    LOG_DEBUG("[ChronicleMetaDirectory] Destroying StoryName={} in ChronicleName={}", story_name.c_str()
         , chronicle_name.c_str());
    std::lock_guard <std::mutex> chronicleMapLock(g_chronicleMetaDirectoryMutex_);
    /* First check if Chronicle exists, fail if false */
    uint64_t cid;
    cid = CityHash64(chronicle_name.c_str(), chronicle_name.length());
    auto chronicleMapRecord = chronicleMap_->find(cid);
    if(chronicleMapRecord != chronicleMap_->end())
    {
        Chronicle*pChronicle = chronicleMap_->find(cid)->second;
        /* Then check if Story exists, fail if false */
        uint64_t sid = pChronicle->getStoryId(story_name);
        if(sid == 0)
        {
            LOG_WARNING("[ChronicleMetaDirectory] StoryID={} StoryName={} does not exist", sid, story_name.c_str());
            return chronolog::to_int(chronolog::ClientErrorCode::NotExist);
        }
        /* Then check if Story is acquired, fail if true */
        Story*pStory = pChronicle->getStoryMap().at(sid);
        if(!pStory->getAcquirerMap().empty())
        {
            for(const auto &acquirerMapRecord: pStory->getAcquirerMap())
            {
                LOG_DEBUG("[ChronicleMetaDirectory] StoryID={} in ChronicleName={} is still acquired by client_id={}"
                     , pStory->getSid(), chronicle_name.c_str(), acquirerMapRecord.first);
            }
            return chronolog::to_int(chronolog::ClientErrorCode::Acquired);
        }
        /* Ask the Chronicle to destroy the Story */
        CL_Status res = pChronicle->removeStory(chronicle_name, story_name);
        if(res != chronolog::to_int(chronolog::ClientErrorCode::Success))
        {
            LOG_ERROR("[ChronicleMetaDirectory] Fail to remove StoryName={} in ChronicleName={}", story_name.c_str()
                 , chronicle_name.c_str());
        }
        return res;
    }
    else
    {
        LOG_WARNING("[ChronicleMetaDirectory] ChronicleName={} does not exist", chronicle_name.c_str());
        return chronolog::to_int(chronolog::ClientErrorCode::NotExist);
    }
}

/**
 * Acquire a Story
 * @param client_id: ClientID to acquire the Story
 * @param chronicle_name: name of the Chronicle that the Story belongs to
 * @param story_name: name of the Story
 * @param flags: flags
 * @param story_id to populate with the story_id assigned to the story
 * @return chronolog::to_int(chronolog::ClientErrorCode::Success) if succeed to destroy the Story \n
 *         chronolog::to_int(chronolog::ClientErrorCode::NotExist) if the Chronicle does not exist \n
 *         chronolog::to_int(chronolog::ClientErrorCode::Unknown) otherwise
 */
int ChronicleMetaDirectory::acquire_story(chl::ClientId const &client_id, const std::string &chronicle_name
                                          , const std::string &story_name
                                          , const std::map <std::string, std::string> &attrs, int &flags
                                          , StoryId &story_id)
{
    LOG_DEBUG("[ChronicleMetaDirectory] ClientID={} acquiring StoryName={} in ChronicleName={} with Flags={}", client_id
         , story_name.c_str(), chronicle_name.c_str(), flags);

    std::lock_guard <std::mutex> chronicleMapLock(g_chronicleMetaDirectoryMutex_);
    /* First check if Chronicle exists, fail if false */
    uint64_t cid;
    cid = CityHash64(chronicle_name.c_str(), chronicle_name.length());
    auto chronicleMapRecord = chronicleMap_->find(cid);
    if(chronicleMapRecord == chronicleMap_->end())
    {
        LOG_WARNING("[ChronicleMetaDirectory] ChronicleName={} does not exist", chronicle_name.c_str());
        return chronolog::to_int(chronolog::ClientErrorCode::NotExist);
    }
    Chronicle*pChronicle = chronicleMapRecord->second;
    /* Then check if Story already_acquired_by_this_client, fail if false */
    auto ret = pChronicle->addStory(story_name, attrs);
    if(ret.first != chronolog::to_int(chronolog::ClientErrorCode::Success))
    {
        return ret.first;
    }
    Story*pStory = ret.second;
    /* Last check if this client has acquired this Story already, do nothing and return success if true */
    auto acquirerMap = pStory->getAcquirerMap();
    auto acquirerMapRecord = acquirerMap.find(client_id);
    if(acquirerMapRecord != acquirerMap.end())
    {
        LOG_DEBUG("[ChronicleMetaDirectory] StoryName={} has already been acquired by ClientID={}", story_name.c_str()
             , client_id);
        /* All checks passed, manipulate metadata */
        return chronolog::to_int(chronolog::ClientErrorCode::Acquired);
    }

    /* All checks passed, manipulate metadata */
    story_id = pStory->getSid();

    /* Increment AcquisitionCount */
    pStory->incrementAcquisitionCount();
    /* Add this client to acquirerClientList of the Story */
    pStory->addAcquirerClient(client_id, clientRegistryManager_->get_client_info(client_id));
    /* Add this Story to acquiredStoryMap for this client */
    clientRegistryManager_->add_story_acquisition(client_id, story_id, pStory);
    return chronolog::to_int(chronolog::ClientErrorCode::Success);
}

/**
 * Release a Story
 * @param client_id: ClientID to release the Story
 * @param chronicle_name: name of the Chronicle that the Story belongs to
 * @param story_name: name of the Story
 * @param flags: flags
 * @param story_id to populate with the story_id assigned to the story
 * @return chronolog::to_int(chronolog::ClientErrorCode::Success) if succeed to destroy the Story \n
 *         chronolog::to_int(chronolog::ClientErrorCode::NotExist) if the Chronicle does not exist \n
 *         chronolog::to_int(chronolog::ClientErrorCode::Unknown) otherwise
 */
//TO_DO return acquisition_count after the story has been released
int ChronicleMetaDirectory::release_story(chl::ClientId const &client_id, const std::string &chronicle_name
                                          , const std::string &story_name, StoryId &story_id)
{
    LOG_DEBUG("[ChronicleMetaDirectory] ClientID={} releasing StoryName={} in ChronicleName={}", client_id
         , story_name.c_str(), chronicle_name.c_str());
    std::lock_guard <std::mutex> chronicleMapLock(g_chronicleMetaDirectoryMutex_);
    /* First check if Chronicle exists, fail if false */
    uint64_t cid;
    cid = CityHash64(chronicle_name.c_str(), chronicle_name.length());
    int ret = chronolog::to_int(chronolog::ClientErrorCode::NotExist);
    auto chronicleRecord = chronicleMap_->find(cid);
    if(chronicleRecord != chronicleMap_->end())
    {
        Chronicle*pChronicle = chronicleRecord->second;
        /* Then check if Story exists, fail if false */
        uint64_t sid = pChronicle->getStoryId(story_name);
        if(sid == 0)
        {
            return chronolog::to_int(chronolog::ClientErrorCode::NotExist);
        }
        Story*pStory = pChronicle->getStoryMap().find(sid)->second;
        /* Check if this client actually acquired this Story before, fail if false */
        auto acquirerMap = pChronicle->getStoryMap().at(sid)->getAcquirerMap();
        auto acquirerMapRecord = acquirerMap.find(client_id);
        if(acquirerMapRecord != acquirerMap.end())
        {
            /* All checks passed and entry found, manipulate metadata */
            /* Decrement AcquisitionCount */
            pStory->decrementAcquisitionCount();
            story_id = pStory->getSid();
            /* Remove this client from acquirerClientList of the Story */
            pStory->removeAcquirerClient(client_id);
            /* Remove this Story from acquiredStoryMap for this client */
            ret = clientRegistryManager_->remove_story_acquisition(client_id, sid);
            if(ret != chronolog::to_int(chronolog::ClientErrorCode::Success))
            {
                return ret;
            }
        }
        else
        {
            LOG_DEBUG("[ChronicleMetaDirectory] StoryName={} is not acquired by ClientID={}, cannot release"
                 , story_name.c_str(), client_id);
            ret = chronolog::to_int(chronolog::ClientErrorCode::NotAcquired);
        }
    }
    return ret;
}

int ChronicleMetaDirectory::get_chronicle_attr(std::string const &name, const std::string &key, std::string &value)
{
    LOG_DEBUG("[ChronicleMetaDirectory] Getting attributes Key={} from ChronicleName={}", key.c_str(), name.c_str());
    std::lock_guard <std::mutex> chronicleMapLock(g_chronicleMetaDirectoryMutex_);
    /* First check if Chronicle exists, fail if false */
    uint64_t cid;
    cid = CityHash64(name.c_str(), name.length());
    auto chronicleMapRecord = chronicleMap_->find(cid);
    if(chronicleMapRecord != chronicleMap_->end())
    {
        Chronicle*pChronicle = chronicleMap_->find(cid)->second;
        if(pChronicle)
        {
            /* Then check if property exists, fail if false */
            auto propertyRecord = pChronicle->getPropertyList().find(key);
            if(propertyRecord != pChronicle->getPropertyList().end())
            {
                value = propertyRecord->second;
                return chronolog::to_int(chronolog::ClientErrorCode::Success);
            }
            else
            {
                LOG_WARNING("[ChronicleMetaDirectory] Property Key={} does not exist in ChronicleName={}", key.c_str()
                     , name.c_str());
                return chronolog::to_int(chronolog::ClientErrorCode::NotExist);
            }
        }
        else
        {
            LOG_ERROR("[ChronicleMetaDirectory] Something is wrong, stored Chronicle object is null");
            return chronolog::to_int(chronolog::ClientErrorCode::Unknown);
        }
    }
    else
    {
        LOG_WARNING("[ChronicleMetaDirectory] ChronicleName={} does not exist", name.c_str());
        return chronolog::to_int(chronolog::ClientErrorCode::NotExist);
    }
}

int
ChronicleMetaDirectory::edit_chronicle_attr(std::string const &name, const std::string &key, const std::string &value)
{
    LOG_DEBUG("[ChronicleMetaDirectory] Editing attribute Key={}, Value={} from ChronicleName={}", key.c_str(), value.c_str()
         , name.c_str());
    std::lock_guard <std::mutex> chronicleMapLock(g_chronicleMetaDirectoryMutex_);
    /* First check if Chronicle exists, fail if false */
    uint64_t cid;
    cid = CityHash64(name.c_str(), name.length());
    auto chronicleMapRecord = chronicleMap_->find(cid);
    if(chronicleMapRecord != chronicleMap_->end())
    {
        Chronicle*pChronicle = chronicleMap_->find(cid)->second;
        if(pChronicle)
        {
            /* Then check if property exists, fail if false */
            auto propertyRecord = pChronicle->getPropertyList().find(key);
            if(propertyRecord != pChronicle->getPropertyList().end())
            {
                auto res = pChronicle->getPropertyList().insert_or_assign(key, value);
                if(res.second)
                {
                    return chronolog::to_int(chronolog::ClientErrorCode::Success);
                }
                else
                {
                    LOG_ERROR("[ChronicleMetaDirectory] Something is wrong, fail to insert property Key={}, Value={}"
                         , key.c_str(), value.c_str());
                    return chronolog::to_int(chronolog::ClientErrorCode::Unknown);
                }
            }
            else
            {
                LOG_WARNING("[ChronicleMetaDirectory] Property Key={} does not exist in ChronicleName={}", key.c_str()
                     , name.c_str());
                return chronolog::to_int(chronolog::ClientErrorCode::NotExist);
            }
        }
        else
        {
            LOG_ERROR("[ChronicleMetaDirectory] Something is wrong, stored Chronicle object is null");
            return chronolog::to_int(chronolog::ClientErrorCode::Unknown);
        }
    }
    else
    {
        LOG_WARNING("[ChronicleMetaDirectory] ChronicleName={} does not exist", name.c_str());
        return chronolog::to_int(chronolog::ClientErrorCode::NotExist);
    }
}

/**
 * Show all the Chronicles names
 * @param client_id: Client ID
 * @return a vector of the names of all Chronicles
 */
int ChronicleMetaDirectory::show_chronicles(std::vector <std::string> &chronicle_names)
{
    chronicle_names.clear();

    std::lock_guard <std::mutex> lock(g_chronicleMetaDirectoryMutex_);
    for(auto &[key, value]: *chronicleMap_)
    {
        chronicle_names.emplace_back(value->getName());
    }
    return chronolog::to_int(chronolog::ClientErrorCode::Success);
}

/**
 * Show all the Stories names in a Chronicle
 * @param client_id: Client ID
 * @param chronicle_name: name of the Chronicle
 * @return a vector of the names of given Chronicle \n
 *         empty vector if Chronicle does not exist
 */

int ChronicleMetaDirectory::show_stories(const std::string &chronicle_name, std::vector <std::string> &story_names)
{
    story_names.clear();

    std::lock_guard <std::mutex> chronicleMapLock(g_chronicleMetaDirectoryMutex_);

    /* First check if Chronicle exists, fail if false */

    uint64_t cid;
    cid = CityHash64(chronicle_name.c_str(), chronicle_name.length());
    auto chronicleMapRecord = chronicleMap_->find(cid);
    if(chronicleMapRecord == chronicleMap_->end())
    { return chronolog::to_int(chronolog::ClientErrorCode::NotExist); }

    Chronicle*pChronicle = chronicleMap_->find(cid)->second;

    LOG_DEBUG("[ChronicleMetaDirectory] Chronicle at {}", static_cast<void*>(&(*pChronicle)));
    for(auto &[key, value]: pChronicle->getStoryMap())
    {
        std::string story_name = value->getName();
        story_names.emplace_back(story_name);
    }
    return chronolog::to_int(chronolog::ClientErrorCode::Success);
}
