#include <regex>
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <numeric>
#include <complex>
#include <time.h>
#include <unistd.h>

//#define NO_LFENCE
//#define NO_MFENCE

#ifdef    NO_LFENCE
#define   lfence()
#else
#include <emmintrin.h>
#define   lfence()  _mm_lfence()
#endif

#ifdef    NO_MFENCE
#define   mfence()
#else
#include <emmintrin.h>
#define   mfence()  _mm_mfence()
#endif

#define TEST_REPS (100000 * 1000)

double cpu_base_frequency() {
    std::regex re("model name\\s*:[^@]+@\\s*([0-9.]+)\\s*GHz"); // seems only work for Intel CPUs, AMD CPUs do not show frequency in cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::smatch m;
    for(std::string line; getline(cpuinfo, line);) {
        regex_match(line, m, re);
        if(m.size() == 2) {
            std::cout << "CPU freq obtained from /proc/cpuinfo: " << std::stod(m[1]) << " GHz" << std::endl;
            return std::stod(m[1]);
        }
    }
    double freq = 3.8; // base clock of Ryzen 7 5700G is 3.8GHz
    std::cout << "Fail to get CPU freq from /proc/cpuinfo, returning hard-coded freq " << freq << " GHz" << std::endl;
    return freq;
}

double const CPU_GHZ_INV = 1 / cpu_base_frequency();

static long diff_in_ns(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec * 1000000000.0 + diff.tv_nsec);
}

template <class T>
void printVectorStats(std::vector<T> v) {
    T min = *std::min_element(v.begin(), v.end());
    T max = *std::max_element(v.begin(), v.end());
    size_t n = v.size() / 2;
    std::nth_element(v.begin(), v.begin() + n, v.end());
    double median = v[n];
    double sum = std::accumulate(v.begin(), v.end(), 0.0);
    double mean = sum / v.size();

    std::vector<double> diff(v.size());
    std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
    double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / v.size());
    std::cout << "Min/Max/Mean/Median/Stddev (ns)" << std::endl
              << min << "\t" << max << "\t" << mean << "\t" << median << "\t" << stdev << std::endl;
}

int main() {
    /**
     * using rdtsc instruction from CPU
     */
    {
        unsigned long long *clock_list = (unsigned long long *) malloc(TEST_REPS * sizeof(unsigned long long));
        std::vector<double> duration_list;
        duration_list.reserve(TEST_REPS);
        for (int i = 0; i < TEST_REPS; i++) {
            mfence();
            lfence();
            clock_list[i] = __builtin_ia32_rdtsc();
        }
        for (int i = 0; i < TEST_REPS - 1; i++)
            duration_list.push_back((clock_list[i + 1] - clock_list[i]) * CPU_GHZ_INV);
        std::cout << "Test with __builtin_ia32_rdtsc() calls: " << std::endl;
        std::cout << "CPU_GHZ_INV: " << CPU_GHZ_INV << std::endl;
        printVectorStats(duration_list);
        std::cout << "Resolution: " << std::endl << "N/A" << std::endl;
        free(clock_list);
    }

    /**
     * using rdtscp instruction from CPU
     */
    {
        unsigned long long *clock_list = (unsigned long long *) malloc(TEST_REPS * sizeof(unsigned long long));
        std::vector<double> duration_list;
        duration_list.reserve(TEST_REPS);
        unsigned int proc_id;
        for (int i = 0; i < TEST_REPS; i++) {
            clock_list[i] = __builtin_ia32_rdtscp(&proc_id);
            lfence();
        }
        for (int i = 0; i < TEST_REPS - 1; i++)
            duration_list.push_back((clock_list[i + 1] - clock_list[i]) * CPU_GHZ_INV);
        std::cout << "\nTest with __builtin_ia32_rdtscp() calls on processor " << proc_id << ": " << std::endl;
        std::cout << "CPU_GHZ_INV: " << CPU_GHZ_INV << std::endl;
        printVectorStats(duration_list);
        std::cout << "Resolution: " << std::endl << "N/A" << std::endl;
        free(clock_list);
    }

    /**
     * using clock_gettime from C
     */
    {
        // use clock_gettime(CLOCK_MONOTONIC)
        struct timespec *clock_list = (struct timespec *) malloc(TEST_REPS * sizeof(struct timespec));
        std::vector<unsigned long long> duration_list;
        duration_list.reserve(TEST_REPS);
        struct timespec res;
        for (int i = 0; i < TEST_REPS; i++) {
            mfence();
            lfence();
            clock_gettime(CLOCK_MONOTONIC, &clock_list[i]);
        }
        for (int i = 0; i < TEST_REPS - 1; i++) {
            mfence();
            lfence();
            duration_list.push_back(diff_in_ns(clock_list[i], clock_list[i + 1]));
        }
        std::cout << "\nTest with clock_gettime(CLOCK_MONOTONIC) calls: " << std::endl;
        printVectorStats(duration_list);
        clock_getres(CLOCK_MONOTONIC, &res);
        std::cout << "Resolution (ns): " << std:: endl
                  << res.tv_sec * 1e9 + res.tv_nsec << std::endl;

//        // use clock_gettime(CLOCK_MONOTONIC_COARSE) with nanosleep(5000000) between calls
//        memset(clock_list, 0, TEST_REPS * sizeof(struct timespec));
//        duration_list.clear();
//        duration_list.reserve(TEST_REPS);
//        struct timespec sleep_ts = {0, 5000000};
//        for (int i = 0; i < TEST_REPS; i++) {
//            mfence();
//            lfence();
//            clock_gettime(CLOCK_MONOTONIC_COARSE, &clock_list[i]);
//            lfence();
//            nanosleep(&sleep_ts, NULL);
//            lfence();
//        }
//        for (int i = 0; i < TEST_REPS - 1; i++) {
//            lfence();
//            duration_list.push_back(diff_in_ns(clock_list[i], clock_list[i + 1]));
//        }
//        std::cout << "\nTest with clock_gettime(CLOCK_MONOTONIC_COARSE) calls (nanosleep(5e+6) between calls): " << std::endl;
//        printVectorStats(duration_list);
//        clock_getres(CLOCK_MONOTONIC_COARSE, &res);
//        std::cout << "Resolution (ns): " << std::endl
//                  << res.tv_sec * 1e9 + res.tv_nsec << std::endl;

        // use clock_gettime(CLOCK_MONOTONIC_RAW)
        duration_list.clear();
        duration_list.reserve(TEST_REPS);
        for (int i = 0; i < TEST_REPS; i++) {
            mfence();
            lfence();
            clock_gettime(CLOCK_MONOTONIC_RAW, &clock_list[i]);
        }
        for (int i = 0; i < TEST_REPS - 1; i++) {
            mfence();
            lfence();
            duration_list.push_back(diff_in_ns(clock_list[i], clock_list[i + 1]));
        }
        std::cout << "\nTest with clock_gettime(CLOCK_MONOTONIC_RAW) calls: " << std::endl;
        printVectorStats(duration_list);
        clock_getres(CLOCK_MONOTONIC_RAW, &res);
        std::cout << "Resolution (ns): " << std::endl
                  << res.tv_sec * 1e9 + res.tv_nsec << std::endl;


        free(clock_list);
    }

    /**
     * using std::chrono from C++
     */
    {
        // get time_point first, calculate timestamp as late as possible
        using namespace std::chrono;
        using clock = steady_clock;
        clock::time_point *clock_list = (clock::time_point *) malloc(TEST_REPS * sizeof(clock::time_point));
        std::vector<duration<double, std::nano>> duration_list;
        std::vector<double> duration_list_double;
        duration_list.reserve(TEST_REPS);
        duration_list_double.reserve(TEST_REPS);
        for (int i = 0; i < TEST_REPS; i++) {
            mfence();
            lfence();
            clock_list[i] = clock::now();
        }
        for (int i = 0; i < TEST_REPS - 1; i++) {
            mfence();
            lfence();
            duration_list.push_back(clock_list[i + 1] - clock_list[i]);
        }
        for (int i = 0; i < TEST_REPS - 1; i++) {
            mfence();
            lfence();
            duration_list_double.push_back(duration_list[i].count());
        }

        std::cout << "\nTest with std::chrono::high_resolution_clock::now() calls (count as late as possible): " << std::endl;
        printVectorStats(duration_list_double);
        std::cout << "Resolution (ns)" << std::endl
                  << (double)std::chrono::high_resolution_clock::period::num /
                     std::chrono::high_resolution_clock::period::den * 1e9 << std::endl;
        free(clock_list);

        // calculate timestamp immediately after getting time_point
        unsigned long long *clock_list2 = (unsigned long long *) malloc(TEST_REPS * sizeof(unsigned long long));
        std::vector<unsigned long long> duration_list2;
        duration_list2.reserve(TEST_REPS);
        for (int i = 0; i < TEST_REPS; i++) {
            mfence();
            lfence();
            clock_list2[i] = clock::now().time_since_epoch().count();
        }
        for (int i = 0; i < TEST_REPS - 1; i++) {
            mfence();
            lfence();
            duration_list2.push_back(clock_list2[i + 1] - clock_list2[i]);
        }
        std::cout << "\nTest with std::chrono::high_resolution_clock::now() calls (count when get time): " << std::endl;
        printVectorStats(duration_list2);
        std::cout << "Resolution (ns):" << std::endl
                  << (double)std::chrono::high_resolution_clock::period::num /
                     std::chrono::high_resolution_clock::period::den * 1e9 << std::endl;
        free(clock_list2);
    }
}
