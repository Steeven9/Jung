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

#include <iostream>
#include <string>
#include <stdlib.h>
#include <chrono>
#include <fstream>
#include <vector>
#include <sys/time.h>
#include <sys/resource.h>
#include <mutex>
#include <unordered_map>

#include "custom_instr.h"

using namespace std;

ofstream log_p;
unordered_map<string, chrono::time_point<chrono::steady_clock>> start_times;
unordered_map<string, int> uid_list;
mutex log_guard, dump_guard, uid_guard;
vector<string> log_buffer;
Side side_p;

int custom_mutex_init(struct custom_mutex * mutex, const pthread_mutexattr_t * attr) {
	return pthread_mutex_init(mutex->mutex, attr);
}

uint32_t getNextUid(string func_name) {
	lock_guard<mutex> lock(uid_guard);
	return ++uid_list[func_name];
}

void write_log(string func_name, string msg) {
	lock_guard<mutex> lock(log_guard);
	// Get current relative timestamp
	const auto now = chrono::steady_clock::now();
	const auto start_time = start_times[func_name];
	size_t timestamp = chrono::duration_cast<chrono::TIMER_PRECISION>(now - start_time).count();

	log_buffer.push_back(to_string(timestamp) + " " + func_name + " " + msg);
}

void* custom_malloc(string func_name, size_t size) {
	void* ptr = malloc(size);
	if (!ptr) {
		handle_error("cannot allocate memory");
	}
	write_log(func_name, "malloc " + to_string(size));
	return ptr;
}

void* custom_realloc(string func_name, void * ptr, size_t size) {
	void* new_ptr;
	// If ptr is a null pointer, the realloc function behaves 
	// like the malloc function for the specified size
	if (!ptr) {
		new_ptr = custom_malloc(func_name, size);
	} else {
		new_ptr = realloc(ptr, size);
		write_log(func_name, "realloc " + to_string(size));
	}
	
	if (!new_ptr) {
		handle_error("cannot reallocate memory");
	}	
	return new_ptr;
}

void custom_free(string func_name, void* ptr) {
	if (!ptr) {
		handle_error("cannot free memory");
	}
	write_log(func_name, "free");
	free(ptr);
}

int custom_pthread_mutex_lock(string func_name, struct custom_mutex* mutex) {
	auto wait_start_time = chrono::steady_clock::now();
	int result = pthread_mutex_lock(mutex->mutex);
	if (result == 0) {
		auto now = chrono::steady_clock::now();
		mutex->hold_start_time = now;
		size_t wait_time = chrono::duration_cast<chrono::TIMER_PRECISION>(now - wait_start_time).count();
		write_log(func_name, "mutex_lock " + to_string(wait_time));
	}
	return result;
}

int custom_pthread_mutex_trylock(string func_name, struct custom_mutex* mutex) {
	int result = pthread_mutex_trylock(mutex->mutex);
	if (result == 0) {
		auto now = chrono::steady_clock::now();
		mutex->hold_start_time = now;
		write_log(func_name, "mutex_trylock");
	}
	return result;
}

int custom_pthread_mutex_unlock(string func_name, struct custom_mutex* mutex) {
	int result = pthread_mutex_unlock(mutex->mutex);
	if (result == 0) {
		auto now = chrono::steady_clock::now();
		size_t hold_time = chrono::duration_cast<chrono::TIMER_PRECISION>(now - mutex->hold_start_time).count();
		write_log(func_name, "mutex_unlock " + to_string(hold_time));
	}	
	return result;
}

int custom_pthread_cond_wait(string func_name, pthread_cond_t* cond, struct custom_mutex* mutex) {
	// Unlocks mutex (->update holding time), waits on cond (-> add waiting time),
	// then relocks mutex and returns
	auto start = chrono::steady_clock::now();
	size_t hold_time = chrono::duration_cast<chrono::TIMER_PRECISION>(start - mutex->hold_start_time).count();
	write_log(func_name, "mutex_unlock " + to_string(hold_time) + " [cond_wait]");

	int result = pthread_cond_wait(cond, mutex->mutex);
	auto now = chrono::steady_clock::now();
	size_t wait_time = chrono::duration_cast<chrono::TIMER_PRECISION>(start - now).count();
	mutex->hold_start_time = now;
	write_log(func_name, "cond_wait_returned " + to_string(wait_time));
	return result;
}

int custom_pthread_cond_timedwait(string func_name, pthread_cond_t* cond, 
 struct custom_mutex* mutex, const struct timespec* abstime) {
	// Unlocks mutex (->update holding time), waits on cond until abstime
	// (-> add waiting time) then relocks mutex and returns
	auto start = chrono::steady_clock::now();
	size_t hold_time = chrono::duration_cast<chrono::TIMER_PRECISION>(start - mutex->hold_start_time).count();
	write_log(func_name, "mutex_unlock " + to_string(hold_time) + " [cond_timedwait]");

	int result = pthread_cond_timedwait(cond, mutex->mutex, abstime);
	auto now = chrono::steady_clock::now();
	size_t wait_time = chrono::duration_cast<chrono::TIMER_PRECISION>(start - now).count();
	mutex->hold_start_time = now;
	write_log(func_name, "cond_timedwait_returned " + to_string(wait_time));
	return result;
}

void start_instrum(std::string func_name, Side side, 
 const std::vector<feature*> & feature_list) {
	start_times[func_name] = chrono::steady_clock::now();
	side_p = side;

	string msg = "FUNC_START";
	for (auto f : feature_list) {
		msg += " ";
		msg += f->print();
	}

	write_log(func_name, msg);
}

void finish_instrum(string func_name) {	
	struct rusage data;
	// RUSAGE_THREAD is not defined on darwin, so we fallback on SELF for portability.
	// Process stats like pagefaults will be off, but at least we get _something_
	#ifdef RUSAGE_THREAD
		getrusage(RUSAGE_THREAD, &data);
	#else
		getrusage(RUSAGE_SELF, &data);
	#endif
	write_log(func_name, "pagefault " + to_string(data.ru_minflt) + " " + to_string(data.ru_majflt));
	write_log(func_name, "FUNC_END");
	dump_log();
}

void dump_log() {
	lock_guard<mutex> lock(dump_guard);
	//TODO move this to a separate shceduled thread
	// Append instead of overwrite
	if (side_p == server) {
		log_p.open(SERVER_LOGFILE, ofstream::app);
	} else if (side_p == client) {
		log_p.open(CLIENT_LOGFILE, ofstream::app);
	} else {
		cerr << "Error: incorrect side parameter" << endl;
        exit(EXIT_FAILURE);
	}

	if (!log_p.is_open()) {
        cerr << "Error: cannot open log" << endl;
        exit(EXIT_FAILURE);
    }

	for (auto s : log_buffer) {
		log_p << s << endl;
	}

	log_buffer.clear();
	log_p.close();
}

void handle_error(string msg, int error_code) {
	cerr << "Error: " << msg << endl;
	dump_log();
	exit(error_code);
}
