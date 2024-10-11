//
// Created by kfeng on 10/7/24.
//

#ifndef CHRONOLOG_TIMERWRAPPER_H
#define CHRONOLOG_TIMERWRAPPER_H

#include <iostream>
#include <chrono>
#include <functional>
#include <mpi.h>

class TimerWrapper
{
public:
    TimerWrapper(bool enable_timing): enabled(enable_timing)
    {}

    // Timing wrapper for functions that return non-void
    template <typename Func, typename... Args, typename RetType = decltype(std::invoke(std::declval <Func>()
                                                                                       , std::declval <Args>()...)), typename std::enable_if <!std::is_void <RetType>::value, int>::type = 0>
    auto timeBlock(const std::string& func_name, Func &&func, Args &&... args) -> RetType
    {
        if(enabled)
        {
            int rank, size;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            auto start = std::chrono::high_resolution_clock::now();
            auto result = std::invoke(std::forward <Func>(func), std::forward <Args>(
                    args)...);  // Call function or block
            auto end = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration_cast <std::chrono::nanoseconds>(end - start).count() / 1000.0;
            double elapsed_max, elapsed_min, elapsed_ave;
            MPI_Reduce(&elapsed, &elapsed_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
            MPI_Reduce(&elapsed, &elapsed_min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
            MPI_Reduce(&elapsed, &elapsed_ave, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
            elapsed_ave /= size;
            if(rank == 0)
            {
                std::cout << "Max of time in " << func_name << ": " << elapsed_max << " us.\n";
                std::cout << "Min of time in " << func_name << ": " << elapsed_min << " us.\n";
                std::cout << "Ave of time in " << func_name << ": " << elapsed_ave << " us.\n";

            }
            return result;  // Return the result of the function or block
        }
        else
        {
            return std::invoke(std::forward <Func>(func), std::forward <Args>(args)...);  // Call without timing
        }
    }

    // Timing wrapper for functions that return void
    template <typename Func, typename... Args, typename RetType = decltype(std::invoke(std::declval <Func>()
                                                                                       , std::declval <Args>()...)), typename std::enable_if <std::is_void <RetType>::value, int>::type = 0>
    void timeBlock(const std::string& func_name, Func &&func, Args &&... args)
    {
        if(enabled)
        {
            int rank, size;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            auto start = std::chrono::high_resolution_clock::now();
            std::invoke(std::forward <Func>(func), std::forward <Args>(args)...);  // Call function or block
            auto end = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration_cast <std::chrono::nanoseconds>(end - start).count() / 1000.0;
            double elapsed_max, elapsed_min, elapsed_ave;
            MPI_Reduce(&elapsed, &elapsed_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
            MPI_Reduce(&elapsed, &elapsed_min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
            MPI_Reduce(&elapsed, &elapsed_ave, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
            elapsed_ave /= size;
            if(rank == 0)
            {
                std::cout << "Max of time in " << func_name << ": " << elapsed_max << " us.\n";
                std::cout << "Min of time in " << func_name << ": " << elapsed_min << " us.\n";
                std::cout << "Ave of time in " << func_name << ": " << elapsed_ave << " us.\n";

            }
        }
        else
        {
            std::invoke(std::forward <Func>(func), std::forward <Args>(args)...);  // Call without timing
        }
    }

private:
    bool enabled;  // Flag to enable or disable timing
};

#endif //CHRONOLOG_TIMERWRAPPER_H
