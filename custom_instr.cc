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

using namespace std;

ofstream log_p;
chrono::time_point<chrono::steady_clock> start_time;

void write_log(string msg) {
	// get current relative timestamp
	const auto now = chrono::steady_clock::now();
	size_t timestamp = chrono::duration_cast<chrono::milliseconds>(now - start_time).count();

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
	if (side == "server") {
		// append instead of overwrite
		log_p.open(SERVER_LOGFILE, ofstream::app);
	} else if (side == "client") {
		log_p.open(CLIENT_LOGFILE);
	} else {
		cerr << "Error: incorrect parameter " << side << " (start_instrum)" << endl;
        exit(EXIT_FAILURE);
	}

	if (!log_p.is_open()) {
        cerr << "Error: cannot open " << side << " log" << endl;
        exit(EXIT_FAILURE);
    }

	start_time = chrono::steady_clock::now();

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
