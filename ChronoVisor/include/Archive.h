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
// Created by kfeng on 3/6/22.
//

#ifndef CHRONOLOG_ARCHIVE_H
#define CHRONOLOG_ARCHIVE_H

#include <string>
#include "city.h"
#include <ostream>
#include <unordered_map>

class Archive {
public:
    Archive() {};

    const std::string &getName() const { return name_; }
    const uint64_t &getAid() const { return aid_; }
    const uint64_t &getCid() const { return cid_; }
    const std::unordered_map<std::string, std::string> &getProperty() const { return propertyList_; }

    void setName(const std::string &name) { name_ = name; }
    void setAid(uint64_t aid) { aid_ = aid; }
    void setCid(uint64_t cid) { cid_ = cid; }
    void setProperty(const std::unordered_map<std::string, std::string>& attrs) {
        for (auto const& entry : attrs) {
            propertyList_.emplace(entry.first, entry.second);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Archive& archive);

private:
    std::string name_;
    uint64_t aid_{};
    uint64_t cid_{};
    std::unordered_map<std::string, std::string> propertyList_;
};

inline std::ostream& operator<<(std::ostream& os, const Archive& archive) {
    os << "name: " << archive.name_ << ", "
       << "aid: " << archive.aid_ << ", "
       << "cid: " << archive.cid_;
    return os;
}

#endif //CHRONOLOG_ARCHIVE_H
