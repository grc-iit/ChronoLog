//
// Created by kfeng on 2/8/22.
//

#ifndef CHRONOLOG_CLOCKSOURCEMANAGER_H
#define CHRONOLOG_CLOCKSOURCEMANAGER_H

#include <chrono>
#include <cstdint>
#include <ctime>
#include <typeinfo>
#include <unistd.h>
<<<<<<< HEAD
#include <emmintrin.h>
=======
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
#include <enum.h>
#include <log.h>

#define   lfence()  _mm_lfence()
#define   mfence()  _mm_mfence()

class ClocksourceTSC 
public:
<<<<<<< HEAD
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
=======
    uint64_t getTimestamp() 
    {
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
        unsigned int proc_id;
        uint64_t  t = __builtin_ia32_rdtscp(&proc_id);
        lfence();
        return t;
    }
    ClocksourceType getClocksourceType() {
        return ClocksourceType::TSC;
    }
};
<<<<<<< HEAD
class ClocksourceCPPStyle : public Clocksource {
public:
    uint64_t getTimestamp() override {
        using namespace std::chrono;
        using clock = steady_clock;
        clock::time_point t = clock::now();
        return (t.time_since_epoch().count());
=======
#endif

class ClockSourceCStyle
{
   public:
     ClockSourceCStyle(){}
     ~ClockSourceCStyle(){}
    uint64_t getTimeStamp()
    {
        struct timespec t{};
        clock_gettime(CLOCK_TAI,&t);
        return t.tv_sec*1000000000+t.tv_nsec;
    }
    ClocksourceType getClocksourceType() {
        return ClocksourceType::C_STYLE;
    }

};

class ClockSourceCPPStyle
{
   public:
    ClockSourceCPPStyle() {}
    ~ClockSourceCPPStyle(){}
    uint64_t getTimeStamp()
    {
        return std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }
    ClocksourceType getClocksourceType() {
        return ClocksourceType::CPP_STYLE;
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
    }
};

template<class ClockSource>
class ClocksourceManager {
private:
    ClocksourceManager() 
        : offset(0)
    { }

public:
    ~ClocksourceManager() 
    { }

    static ClocksourceManager *getInstance() {
        if (!clocksourceManager_) {
            clocksourceManager_ = new ClocksourceManager();
        }
        return clocksourceManager_;
    }

    ClocksourceType getClocksourceType() {
        return clocksource_.getClocksourceType();
    }

    uint64_t TimeStamp()
    {
        return clocksource_.getTimeStamp() + offset;
    }

private:
    static ClocksourceManager *clocksourceManager_;
    ClockSource clocksource_;
    int64_t offset;
};

#endif //CHRONOLOG_CLOCKSOURCEMANAGER_H
