//
// Created by kfeng on 3/9/22.
//

#ifndef CHRONOLOG_CHRONICLEMETADIRECTORY_H
#define CHRONOLOG_CHRONICLEMETADIRECTORY_H

#include <string>
#include <unordered_map>
#include <vector>
#include <Chronicle.h>
<<<<<<< HEAD
#include <memory>
=======
#include "chronolog_types.h" 

class ClientRegistryManager;

typedef uint64_t StoryId;
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a

class ChronicleMetaDirectory {
public:
    ChronicleMetaDirectory();
    ~ChronicleMetaDirectory();

//    std::shared_ptr<std::unordered_map<std::string, Chronicle *>> getChronicleMap() { return chronicleMap_; }
    std::unordered_map<uint64_t, Chronicle *> *getChronicleMap() { return chronicleMap_; }

    int create_chronicle(const std::string& name);
    int create_chronicle(const std::string& name, const std::unordered_map<std::string, std::string>& attrs);
    int destroy_chronicle(const std::string& name, int& flags);

    int create_story(std::string &chronicle_name, const std::string& story_name,
                     const std::unordered_map<std::string, std::string>& attrs);
<<<<<<< HEAD
    int destroy_story(std::string &chronicle_name, const std::string& story_name, int& flags);
    int acquire_story(const std::string& client_id, const std::string& chronicle_name,
                      const std::string& story_name, int& flags);
    int release_story(const std::string& client_id, const std::string& chronicle_name,
                      const std::string& story_name, int& flags);
=======
    int destroy_story(std::string const& chronicle_name, const std::string& story_name);
    int acquire_story(chronolog::ClientId const & client_id, const std::string& chronicle_name,
                      const std::string& story_name, int& flags, StoryId &, bool&);
    int release_story(chronolog::ClientId const & client_id, const std::string& chronicle_name,
                      const std::string& story_name, StoryId &, bool&);
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a

    uint64_t record_event(uint64_t sid, void *data);
    uint64_t playback_event(uint64_t sid);

    int get_chronicle_attr(std::string& name, const std::string& key, std::string& value);
    int edit_chronicle_attr(std::string& name, const std::string& key, const std::string& value);

    int show_chronicles(std::vector<std::string>&);
    int show_stories( const std::string& chronicle_name, std::vector<std::string> &);

private:
//    std::shared_ptr<std::unordered_map<std::string, Chronicle *>> chronicleMap_;
    std::unordered_map<uint64_t , Chronicle *> *chronicleMap_;
//    std::unordered_map<uint64_t, Chronicle *> *acquiredChronicleMap_;
    std::unordered_map<uint64_t, Story *> *acquiredStoryMap_;
    std::unordered_multimap<uint64_t, std::string> *acquiredChronicleClientMap_;
    std::unordered_multimap<uint64_t, std::string> *acquiredStoryClientMap_;
    std::mutex g_chronicleMetaDirectoryMutex_;
    std::mutex g_acquiredChronicleMapMutex_;
    std::mutex g_acquiredStoryMapMutex_;
//    std::unordered_map<std::string, uint64_t> *chronicleName2IdMap_;
//    std::unordered_map<uint64_t, std::string> *chronicleId2NameMap_;
};

#endif //CHRONOLOG_CHRONICLEMETADIRECTORY_H
