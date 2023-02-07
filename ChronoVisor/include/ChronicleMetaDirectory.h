/*BSD 2-Clause License

Copyright (c) 2022, Scalable Computing Software Laboratory
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//
// Created by kfeng on 3/9/22.
//

#ifndef CHRONOLOG_CHRONICLEMETADIRECTORY_H
#define CHRONOLOG_CHRONICLEMETADIRECTORY_H

#include <string>
#include <unordered_map>
#include <vector>
#include <Chronicle.h>
#include <memory>

class ChronicleMetaDirectory {
public:
    ChronicleMetaDirectory();
    ~ChronicleMetaDirectory();

//    std::shared_ptr<std::unordered_map<std::string, Chronicle *>> getChronicleMap() { return chronicleMap_; }
    std::unordered_map<uint64_t, Chronicle *> *getChronicleMap() { return chronicleMap_; }

    int create_chronicle(const std::string& name);
    int create_chronicle(const std::string& name, const std::unordered_map<std::string, std::string>& attrs);
    int destroy_chronicle(const std::string& name, int& flags);
    int acquire_chronicle(const std::string& name, int& flags);
    int release_chronicle(const std::string& name, int& flags);

    int create_story(std::string &chronicle_name, const std::string& story_name, const std::unordered_map<std::string, std::string>& attrs);
    int destroy_story(std::string &chronicle_name, const std::string& story_name, int& flags);
    int get_story_list(std::string &chronicle_name, std::vector<std::string> &story_name_list);
    int acquire_story(const std::string& chronicle_name, const std::string& story_name, int& flags);
    int release_story(const std::string& chronicle_name, const std::string& story_name, int& flags);

    uint64_t record_event(uint64_t sid, void *data);
    uint64_t playback_event(uint64_t sid);

    int get_chronicle_attr(std::string& name, const std::string& key, std::string& value);
    int edit_chronicle_attr(std::string& name, const std::string& key, const std::string& value);

private:
//    std::shared_ptr<std::unordered_map<std::string, Chronicle *>> chronicleMap_;
    std::unordered_map<uint64_t , Chronicle *> *chronicleMap_;
    std::unordered_map<uint64_t, Chronicle *> *acquiredChronicleMap_;
    std::unordered_map<uint64_t, Story *> *acquiredStoryMap_;
};

#endif //CHRONOLOG_CHRONICLEMETADIRECTORY_H
