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

enum Side { client, server };

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
extern uint32_t uid;

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
extern void* custom_realloc(std::string func_name, void* ptr, size_t size);

/*
	A custom free implementation that writes to the log
	if memory has been freed.
*/
extern void custom_free(std::string func_name, void* ptr);

/*
	A custom mutex_lock implementation that writes 
	to the log how much time it waited for the lock.
*/
extern int custom_pthread_mutex_lock(std::string func_name, pthread_mutex_t* mutex);

/*
	A custom mutex_trylock implementation.
*/
extern int custom_pthread_mutex_trylock(std::string func_name, pthread_mutex_t* mutex);

/*
	A custom mutex_unlock implementation that writes 
	to the log how much time it held the lock.
*/
extern int custom_pthread_mutex_unlock(std::string func_name, pthread_mutex_t* mutex);

/*
	A custom cond_wait implementation that writes 
	to the log how much time it waited for the condition.
*/
extern int custom_pthread_cond_wait(std::string func_name, pthread_cond_t* cond, pthread_mutex_t* mutex);

/*
	A custom cond_timedwait implementation that writes 
	to the log how much time it waited for the condition.
*/
extern int custom_pthread_cond_timedwait(std::string func_name, pthread_cond_t* cond, 
	pthread_mutex_t* mutex, const struct timespec* abstime);

/*
	Starts our custom instrumentation.
	Side is either server or client.
*/
extern void start_instrum(std::string func_name, Side side, 
 const std::vector<basic_feature*> & feature_list);

/*
	Stops the instrumentation.
*/
extern void finish_instrum(std::string func_name);

#endif
