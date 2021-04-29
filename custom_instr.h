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

#ifndef CUSTOM_INSTR_H_INCLUDED
#define CUSTOM_INSTR_H_INCLUDED

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#define SERVER_LOGFILE "server_log.txt"
#define CLIENT_LOGFILE "client_log.txt"
#define TRACE_LOGFILE  "trace_log.txt"
#define MERGED_LOGFILE "merged_log.txt"

// Must be in chrono::
#define TIMER_PRECISION milliseconds
#define TIMER_UNIT "ms"

class basic_feature {
public:
    virtual std::string print() const = 0;
};

template <class T>
struct feature : public basic_feature {
    std::string name;
    T value;

    feature(const std::string & n, const T & v)
	: name(n), value(v) {};

    virtual std::string print() const {
		std::ostringstream output;
    	output << name << "=" << value;
		return output.str();
    }
};

template <class T>
feature<T> * make_feature(const std::string & n, const T & v) {
    return new feature<T>(n, v);
}

extern std::ofstream log_p;

/*
	Writes the given string to the log file.
	Output format:  time_elapsed function_name event [params]
	Example: 150 do_stuff RPC_start
	Example: 152 do_stuff malloc 10
*/
extern void write_log(std::string func_name, std::string msg);

/*
	A custom malloc implementation that writes to the log
	how much memory has been allocated.
*/
extern void* custom_malloc(std::string func_name, size_t size);

/*
	A custom realloc implementation that writes to the log
	how much memory has been reallocated (if any).
*/
extern void* custom_realloc(std::string func_name, void * ptr, size_t size);

/*
	A custom free implementation that writes to the log
	if memory has been freed.
*/
extern void custom_free(std::string func_name, void* ptr);

//TODO add custom_pthread_mutex_lock, custom_pthread_mutex_trylock, 
//custom_pthread_mutex_unlock, custom_pthread_cond_wait, custom_pthread_cond_timedwait

/*
	Starts our custom instrumentation.
	Side is either "server" or "client".
*/
extern void start_instrum(std::string func_name, std::string side, 
 const std::vector<basic_feature*> & feature_list);

/*
	Stops the instrumentation.
*/
extern void finish_instrum(std::string func_name);

#endif
