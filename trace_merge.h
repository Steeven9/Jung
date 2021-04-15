/*
 *
 * Copyright 2021 Stefano Taillefert.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef TRACE_MERGE_H_INCLUDED
#define TRACE_MERGE_H_INCLUDED

#include <string>
#include <vector>

#include "custom_instr.h"

struct custom_func {
    std::string name;
    long exec_time = 0;
    long network_time = 0;
    long server_time = 0;
    int memory_usage = 0;
    int server_memory = 0;
    int mem_leaks = 0;
    int server_memory_leaks = 0;
    const std::vector<basic_feature*> feature_list;

    custom_func(const std::string & n, const std::vector<basic_feature*> & f_l)
	: name(n), feature_list(f_l) {};

    virtual std::string print() const {
        std::string msg = name + " took " + std::to_string(exec_time) + " ms, of which approx. " + 
            std::to_string(network_time) + " ms in network and approx. " + std::to_string(server_time) + 
            " ms in server.\nUsed " + std::to_string(memory_usage) + " bytes of memory client-side and " + 
            std::to_string(server_memory) + " bytes of memory server-side.";

        if (mem_leaks > 0) {
            msg += "\nPossible client memory leak detected! " + std::to_string(mem_leaks) + " malloc call(s) not freed.";
        }

        if (server_memory_leaks > 0) {
            msg += "\nPossible server memory leak detected! " + std::to_string(server_memory_leaks) + " malloc call(s) not freed.";
        }

        if (feature_list.size() > 0) {
            msg += "\nFound " + std::to_string(feature_list.size()) + " feature(s): ";
            for (auto f : feature_list) {
                msg += f->print() + " ";
            }
        }

        return msg;
    }
};


custom_func * make_custom_func(const std::string & n, const std::vector<basic_feature*> & f_l) {
    return new custom_func(n, f_l);
}

/* 
    Produce a unified performance trace with the data
    from client and server RPC calls.
*/
extern void generate_perf_trace();

/* 
    Read a line from the client log, then if there
    is a RPC request, search the relevant lines in the
    server log and append them.
*/
extern void simple_merge();

#endif
