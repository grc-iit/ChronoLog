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
// Created by jaime on 2/10/2022.
//

#ifndef CHRONOLOG__SERVECTOR_H_
#define CHRONOLOG__SERVECTOR_H_

#include <vector>
#include "TimeRecord.h"
#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>

//TODO: Extend to a template<class T>
class SerVector {
 private:
  std::vector<TimeRecord> vec;
 public:
  std::vector<TimeRecord> get(){
    return vec;
  }

  /**************************************
   Constructors
  ***************************************/

  /**************************************
   Element Access
  ***************************************/
  //Returning by reference allows for using as lvalue on assignments
  TimeRecord& at(size_t i){
    return vec.at(i);
  }

  TimeRecord& front(){
    return vec.front();
  }

  TimeRecord& back(){
    return vec.back();
  }

  TimeRecord* data(){
    return vec.data();
  }

  auto& operator[](std::size_t idx) { return vec[idx]; }

  /**************************************
   Iterators. Missing constant iterators
  ***************************************/
  std::vector<TimeRecord>::iterator begin(){
    return vec.begin();
  }

  std::vector<TimeRecord>::reverse_iterator rbegin(){
    return vec.rbegin();
  }

  std::vector<TimeRecord>::iterator end(){
    return vec.end();
  }

  std::vector<TimeRecord>::reverse_iterator rend(){
    return vec.rend();
  }

  /**************************************
   Capacity operators. No shrink_to_fit
  ***************************************/
  bool empty(){
      return vec.empty();
  }

  size_t size(){
    return vec.size();
  }

  size_t max_size(){
    return vec.max_size();
  }

  void reserve(size_t new_size){
    vec.reserve(new_size);
  }

  size_t capacity(){
    return vec.capacity();
  }

  /**************************************
   Modifiers
  **************************************/
  void clear(){
    vec.clear();
  }

  void insert(){
  }

  void emplace(){

  }

  void erase(){

  }

  void push_back(){

  }

  void emplace_back(){

  }

  void pop_back(){

  }

  void resize(size_t new_size){
    vec.resize(new_size);
  }

  void swap(){

  }

  /**************************************
   Modifiers
  **************************************/
  template<class Archive>
  void serialize(Archive & archive)
  {
    archive( vec ); // serialize things by passing them to the archive
  }

  void load(){

  }
};

#endif //CHRONOLOG__SERVECTOR_H_
