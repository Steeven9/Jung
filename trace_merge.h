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
    uint64_t exec_time = 0;
    uint64_t network_time = 0;
    uint64_t server_time = 0;
    uint64_t lock_holding_time = 0;
    uint64_t waiting_time = 0;
    uint64_t server_lock_holding_time = 0;
    uint64_t server_waiting_time = 0;
    uint64_t memory_usage = 0;
    uint64_t server_memory_usage = 0;
    uint64_t mem_leaks = 0;
    uint64_t server_mem_leaks = 0;
    uint64_t min_pagefault = 0;
    uint64_t maj_pagefault = 0;
    uint64_t server_min_pagefault = 0;
    uint64_t server_maj_pagefault = 0;
    const std::vector<basic_feature*> feature_list;

    custom_func(const std::string & n, const std::vector<basic_feature*> & f_l)
	: name(n), feature_list(f_l) {};

    virtual std::string print() const {
        std::string msg = name + " took " + std::to_string(exec_time) + " " + TIMER_UNIT + ", of which approx. " + 
            std::to_string(network_time) + " " + TIMER_UNIT + " in network and approx. " + std::to_string(server_time) + 
            " " + TIMER_UNIT + " in server.\nUsed " + std::to_string(memory_usage) + " bytes of memory client-side and " + 
            std::to_string(server_memory_usage) + " bytes of memory server-side.\nThere were " + std::to_string(min_pagefault) + 
            " minor pagefaults and " + std::to_string(maj_pagefault) + " major ones client-side; " 
            + std::to_string(server_min_pagefault) + " minor pagefaults and " + std::to_string(server_maj_pagefault) + 
            " major ones server-side.\nWaited for " + std::to_string(waiting_time) + " " + TIMER_UNIT + " and held lock for " + 
            std::to_string(lock_holding_time) + " " + TIMER_UNIT + ".";

        if (mem_leaks > 0) {
            msg += "\nPossible client memory leak detected! " + std::to_string(mem_leaks) + " malloc call(s) not freed.";
        }

        if (server_mem_leaks > 0) {
            msg += "\nPossible server memory leak detected! " + std::to_string(server_mem_leaks) + " malloc call(s) not freed.";
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
    Produces a unified performance trace with the data
    from client and server RPC calls.
*/
extern void generate_perf_trace();

/*
    Encodes the performance stats in Freud's binary format
    so that it can be read by freud-statistics.
    See https://github.com/usi-systems/freud/blob/master/freud-pin/dumper.cc
*/
extern void encode_perf_trace(std::unordered_map<std::string, custom_func *> func_list);

/* 
    Reads a line from the client log, then if there
    is a RPC request, search the relevant lines in the
    server log and append them.
*/
extern void simple_merge();

#endif
