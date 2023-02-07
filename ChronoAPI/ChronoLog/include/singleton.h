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
// Created by kfeng on 3/30/22.
//

#ifndef CHRONOLOG_SINGLETON_H
#define CHRONOLOG_SINGLETON_H

#include <iostream>
#include <memory>
#include <utility>

/**
 * Make a class singleton when used with the class. format for class name T
 * Singleton<T>::GetInstance()
 * @tparam T
 */
namespace ChronoLog {
    template<typename T>
    class Singleton {
    public:
        /**
         * Members of Singleton Class
         */
        /**
         * Uses unique pointer to build a static global instance of variable.
         * @tparam T
         * @return instance of T
         */
        template<typename... Args>
        static std::shared_ptr<T> GetInstance(Args... args) {
            if (instance == nullptr)
                instance = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
            return instance;
        }

        /**
         * Operators
         */
        Singleton &operator=(const Singleton) = delete; /* deleting = operatos*/
        /**
         * Constructor
         */
    public:
        Singleton(const Singleton &) = delete; /* deleting copy constructor. */

    protected:
        static inline std::shared_ptr<T> instance = nullptr;

        Singleton() {} /* hidden default constructor. */
    };

//    template<typename T>
//    std::shared_ptr<T> Singleton<T>::instance = nullptr;
}

#endif //CHRONOLOG_SINGLETON_H
