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
// Created by kfeng on 2/8/22.
//

#ifndef CHRONOLOG_CLOCKSOURCEMANAGER_H
#define CHRONOLOG_CLOCKSOURCEMANAGER_H

#include <chrono>
#include <cstdint>
#include <ctime>
#include <unistd.h>
#include <emmintrin.h>

#define   lfence()  _mm_lfence()
#define   mfence()  _mm_mfence()

enum ClocksourceType {
    TBD = 0,
    C_STYLE = 1,
    CPP_STYLE = 2,
    TSC = 3
};

class Clocksource {
public:
    static Clocksource *Create(ClocksourceType type);
    virtual ~Clocksource() = default;;
    /**
     * @name Get timestamp
     */
    ///@{
    virtual uint64_t getTimestamp() = 0;
    ///@}
};

class ClocksourceCStyle : public Clocksource {
public:
    uint64_t getTimestamp() override {
        struct timespec t{};
        clock_gettime(CLOCK_MONOTONIC, &t);
        return (t.tv_sec * (uint64_t) 1e9 + t.tv_nsec);
    }
};

class ClocksourceTSC : public Clocksource {
public:
    uint64_t getTimestamp() override {
        unsigned int proc_id;
        uint64_t  t = __builtin_ia32_rdtscp(&proc_id);
        lfence();
        return t;
    }
};
class ClocksourceCPPStyle : public Clocksource {
public:
    uint64_t getTimestamp() override {
        using namespace std::chrono;
        using clock = steady_clock;
        clock::time_point t = clock::now();
        return (t.time_since_epoch().count());
    }
};

class ClocksourceManager {
private:
    ClocksourceManager() : clocksource_(nullptr), clocksourceType_(ClocksourceType::TBD) {}

public:
    ~ClocksourceManager() {
        if (clocksource_) {
            delete[] clocksource_;
            clocksource_ = nullptr;
        }
        if (clocksourceManager_) {
            delete[] clocksourceManager_;
            clocksourceManager_ = nullptr;
        }
    }

    static ClocksourceManager *getInstance() {
        if (!clocksourceManager_) {
            clocksourceManager_ = new ClocksourceManager();
        }
        return clocksourceManager_;
    }

    void setClocksourceType(ClocksourceType type) {
        this->clocksourceType_ = type;
    }

    Clocksource *getClocksource() {
        this->clocksource_ = Clocksource::Create(this->clocksourceType_);
        return this->clocksource_;
    }
private:
    static ClocksourceManager *clocksourceManager_;
    Clocksource *clocksource_;
    ClocksourceType clocksourceType_;
};

#endif //CHRONOLOG_CLOCKSOURCEMANAGER_H
