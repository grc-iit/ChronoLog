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
// Created by kfeng on 12/19/22.
//

#ifndef CHRONOLOG_ERRCODE_H
#define CHRONOLOG_ERRCODE_H

#define CL_SUCCESS                          0       /* Success */
#define CL_ERR_UNKNOWN                      -1      /* Error does not belong to any categories below */
#define CL_ERR_INVALID_ARG                  -2      /* Invalid arguments */
#define CL_ERR_NOT_EXIST                    -3      /* Specified Chronicle or Story does not exist */
#define CL_ERR_ACQUIRED                     -4      /* Specified Chronicle or Story is acquired, cannot be destroyed */
#define CL_ERR_CHRONICLE_EXISTS             -5      /* Specified Chronicle exists, cannot be created/renamed to */
#define CL_ERR_STORY_EXISTS                 -6      /* Specified Story exists, cannot be created/renamed to */
#define CL_ERR_Archive_EXISTS               -7      /* Specified Archive exists, cannot be created/renamed to */
#define CL_ERR_CHRONICLE_PROPERTY_FULL      -8      /* Property list of Chronicle is full, cannot add new property */
#define CL_ERR_STORY_PROPERTY_FULL          -9      /* Property list of Story is full, cannot add new property */
#define CL_ERR_CHRONICLE_METADATA_FULL      -10     /* Metadata list of Chronicle is full, cannot add new property */
#define CL_ERR_STORY_METADATA_FULL          -11     /* Metadata list of Story is full, cannot add new property */

#endif //CHRONOLOG_ERRCODE_H
