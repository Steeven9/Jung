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

#include "custom_instr.h"

using namespace std;

ofstream log_p;
chrono::time_point<chrono::steady_clock> start_time;
uint32_t uid = 0;

//TODO add buffer for logging and flush it every now and then (thread?)

int custom_mutex_init(struct custom_mutex * mutex, const pthread_mutexattr_t * attr) {
	return pthread_mutex_init(mutex->mutex, attr);
}

void write_log(string func_name, string msg) {
	// Get current relative timestamp
	const auto now = chrono::steady_clock::now();
	size_t timestamp = chrono::duration_cast<chrono::TIMER_PRECISION>(now - start_time).count();

	log_p << timestamp << " " + func_name << " " + msg << endl;
}

void* custom_malloc(string func_name, size_t size) {
	void* ptr = malloc(size);
	if (!ptr) {
		cerr << "Error: cannot allocate memory" << endl;
		exit(EXIT_FAILURE);
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
		cerr << "Error: cannot reallocate memory" << endl;
		exit(EXIT_FAILURE);
	}	
	return new_ptr;
}

void custom_free(string func_name, void* ptr) {
	if (!ptr) {
		cerr << "Error: cannot free memory" << endl;
		exit(EXIT_FAILURE);
	}
	write_log(func_name, "free");
	free(ptr);
}

int custom_pthread_mutex_lock(std::string func_name, struct custom_mutex* mutex) {
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

int custom_pthread_mutex_trylock(std::string func_name, struct custom_mutex* mutex) {
	int result = pthread_mutex_trylock(mutex->mutex);
	if (result == 0) {
		auto now = chrono::steady_clock::now();
		mutex->hold_start_time = now;
		write_log(func_name, "mutex_trylock");
	}
	return result;
}

int custom_pthread_mutex_unlock(std::string func_name, struct custom_mutex* mutex) {
	int result = pthread_mutex_unlock(mutex->mutex);
	if (result == 0) {
		auto now = chrono::steady_clock::now();
		size_t hold_time = chrono::duration_cast<chrono::TIMER_PRECISION>(now - mutex->hold_start_time).count();
		write_log(func_name, "mutex_unlock " + to_string(hold_time));
	}	
	return result;
}

int custom_pthread_cond_wait(std::string func_name, pthread_cond_t* cond, struct custom_mutex* mutex) {
	//TODO fix
	write_log(func_name, "cond_wait_called");
	int result = pthread_cond_wait(cond, mutex->mutex);
	write_log(func_name, "cond_wait_returned");
	return result;
}

int custom_pthread_cond_timedwait(std::string func_name, pthread_cond_t* cond, 
 struct custom_mutex* mutex, const struct timespec* abstime) {
	//TODO fix
	write_log(func_name, "cond_timedwait_called");
	int result = pthread_cond_timedwait(cond, mutex->mutex, abstime);
	write_log(func_name, "cond_timedwait_returned");
	return result;
}

void start_instrum(string func_name, Side side,
 const vector<basic_feature*> & feature_list) {
	if (side == server) {
		// Append instead of overwrite
		log_p.open(SERVER_LOGFILE, ofstream::app);
	} else if (side == client) {
		log_p.open(CLIENT_LOGFILE, ofstream::app);
	} else {
		cerr << "Error: incorrect parameter " << side << " (start_instrum)" << endl;
        exit(EXIT_FAILURE);
	}

	if (!log_p.is_open()) {
        cerr << "Error: cannot open " << side << " log" << endl;
        exit(EXIT_FAILURE);
    }

	start_time = chrono::steady_clock::now();

	string msg = "FUNC_START";
	for (auto f : feature_list) {
		msg += " ";
		msg += f->print();
	}

	write_log(func_name, msg);
}

void finish_instrum(string func_name) {	
	struct rusage data;
	//RUSAGE_THREAD is not defined on darwin so we use SELF for portability
	getrusage(RUSAGE_SELF, &data);
	write_log(func_name, "pagefault " + to_string(data.ru_minflt) + " " + to_string(data.ru_majflt));
	write_log(func_name, "FUNC_END");
	log_p.close();
}
