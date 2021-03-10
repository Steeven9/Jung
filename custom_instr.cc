/*
 *
 * Copyright 2021 gRPC authors and Stefano Taillefert.
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

#include "custom_instr.h"

using namespace std;

ofstream log_p;

void write_log(string msg) {
	// get current timestamp
    int64_t timestamp = std::chrono::duration_cast<chrono::milliseconds>(
		chrono::system_clock::now().time_since_epoch()).count();

	log_p << "[" << timestamp << "] " << msg << endl;
}

/*
	A custom malloc implementation that writes to the log
	how much memory has been allocated.
*/
void* custom_malloc(string func_name, size_t size) {
	void* ptr = malloc(size);
	if (!ptr) {
		cerr << "Cannot allocate memory" << endl;
		exit(EXIT_FAILURE);
	}
	write_log(func_name + " Malloc " + to_string(size));
	return ptr;
}

/*
	A custom free implementation that writes to the log
	how much memory has been freed.
*/
void custom_free(string func_name, void* ptr) {
	if (!ptr) {
		cerr << "Cannot free memory" << endl;
		exit(EXIT_FAILURE);
	}
	write_log(func_name + " Free");
	free(ptr);
}

/*
	Strart our custom instrumentation tracker.
	Side is either "server" or "client".
*/
void start_instrum(string func_name, string side, int param) {
	ios_base::openmode mode = ofstream::out;
	if (side == "server") {
		mode = ofstream::app;
	}

	log_p.open(side + "_log.txt", mode);

	string msg = func_name + " START";
	if (param != -1) {
		msg += " param=" + to_string(param);
	}

	write_log(msg);
}

/*
	Stops the instrumentation.
*/
void finish_instrum(string func_name) {
	write_log(func_name + " END");
	log_p.close();
}

