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

#include "custom_instr.h"

// Uncomment to get a full text timestamp instead of ms
// WARNING: WILL BREAK THE MERGER, DEBUG ONLY
//#define HUMAN_READABLE

using namespace std;

ofstream log_p;
//TODO add precise custom timer (chrono::steady_clock) in ns

void write_log(string msg) {
	// get current timestamp
	#ifdef HUMAN_READABLE
		time_t result = std::time(nullptr);
		string timestamp = asctime(localtime(&result));
		timestamp = timestamp.substr(0, timestamp.length() - 1); // get rid of null-terminator
	#else
    	int64_t timestamp = chrono::duration_cast<chrono::milliseconds>(
			chrono::system_clock::now().time_since_epoch()).count();
	#endif

	log_p << timestamp << " " + msg << endl;
}

void* custom_malloc(string func_name, size_t size) {
	void* ptr = malloc(size);
	if (!ptr) {
		cerr << "Error: cannot allocate memory" << endl;
		exit(EXIT_FAILURE);
	}
	write_log(func_name + " malloc " + to_string(size));
	return ptr;
}

void custom_free(string func_name, void* ptr) {
	if (!ptr) {
		cerr << "Error: cannot free memory" << endl;
		exit(EXIT_FAILURE);
	}
	write_log(func_name + " free");
	free(ptr);
}

void start_instrum(string func_name, string side,
 const vector<basic_feature*> & feature_list) {
	ios_base::openmode mode = ofstream::out;
	if (side == "server") {
		// append instead of overwrite
		mode = ofstream::app;
	}

	log_p.open(side + "_log.txt", mode);

	if (!log_p.is_open()) {
        cerr << "Error: cannot open " << side << " log" << endl;
        exit(EXIT_FAILURE);
    }

	string msg = func_name + " FUNC_START";
	for (auto f : feature_list) {
		msg += " ";
		msg += f->print();
	}

	write_log(msg);
}

void finish_instrum(string func_name) {
	write_log(func_name + " FUNC_END");
	log_p.close();
}
