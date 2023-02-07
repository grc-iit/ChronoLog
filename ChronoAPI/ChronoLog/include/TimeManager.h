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
// Created by kfeng on 2/14/22.
//

#ifndef CHRONOLOG_TIMEMANAGER_H
#define CHRONOLOG_TIMEMANAGER_H

#include <atomic>
#include "ClocksourceManager.h"
#include "TimeRecord.h"

#define TIME_DB_INIT_WAIT 1
#define TIME_DB_UPDATE_INTERVAL 3

class TimeManager {
public:
    TimeManager() {
        refTimestampUpdateInterval_ = TIME_DB_UPDATE_INTERVAL;
        init(refTimestampUpdateInterval_);
    }

    explicit TimeManager(double updateInterval) {
        refTimestampUpdateInterval_ = updateInterval;
        init(refTimestampUpdateInterval_);
    }

    explicit TimeManager(double updateInterval, ClocksourceType clocksourceType) {
        refTimestampUpdateInterval_ = updateInterval;
        init(refTimestampUpdateInterval_, clocksourceType);
    }

    void setClocksourceType(ClocksourceType clocksourceType) { clocksourceType_ = clocksourceType; }

    std::unique_ptr<TimeRecord> genPeriodicTimeRecord() {
        std::unique_ptr<TimeRecord> record = std::make_unique<TimeRecord>(clocksource_);
        if (firstTimeRecord_) {
            // very first time of generating a TimeRecord for big bang, sleep 1s and calculate drift rate
            record->updateTimestamp();
            uint64_t lastTimestamp = record->getTimestamp();
            record->setUpdateInterval(TIME_DB_INIT_WAIT);
            usleep(TIME_DB_INIT_WAIT * 1e6);
            record->updateTimeRecord(lastTimestamp);
            latestDriftRate_ = record->getDriftRate();
        } else {
            record->setUpdateInterval(refTimestampUpdateInterval_);
            record->setDriftRate(latestDriftRate_);
            if (lastTimestamp_ == 0) {
                // first interval TimeRecord
                lastTimestamp_ = clocksource_->getTimestamp();
            } else {
                uint64_t curTimestamp_ = clocksource_->getTimestamp();
                latestDriftRate_ = (double) (curTimestamp_ - lastTimestamp_) /
                                   refTimestampUpdateInterval_ / 1000000000.0 - 1;
                lastTimestamp_ = curTimestamp_;
            }
        }
        firstTimeRecord_ = false;
        record->updateTimestamp();
        return record;
    }

    std::unique_ptr<TimeRecord> genOnDemandTimeRecord() {
        std::unique_ptr<TimeRecord> record = std::make_unique<TimeRecord>(clocksource_);
        record->updateTimestamp();
        record->setDriftRate(latestDriftRate_);
        return record;
    }

public:
    Clocksource *clocksource_{};               ///< clocksource (clock_gettime, std::high_precision_clock::now(), or TSC)
    double refTimestampUpdateInterval_;      ///< how often reference timestamp is updated for drift rate calculation
    uint64_t lastTimestamp_{};
    std::atomic<double> latestDriftRate_{};
    bool firstTimeRecord_{};
    ClocksourceType clocksourceType_{};

private:
    void init(double updateInterval, ClocksourceType clocksourceType = ClocksourceType::C_STYLE) {
        firstTimeRecord_ = true;
        refTimestampUpdateInterval_ = updateInterval;
        clocksourceType_ = clocksourceType;
        ClocksourceManager *manager = ClocksourceManager::getInstance();
        manager->setClocksourceType(clocksourceType_);
        clocksource_ = manager->getClocksource();
    }
};


#endif //CHRONOLOG_TIMEMANAGER_H
