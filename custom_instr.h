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

#ifndef custom_instr
#define custom_instr

#include <string>
#include <stdlib.h>
#include <chrono>
#include <fstream>

using namespace std;

extern ofstream log_p;

/*
	Writes the given string to the log file.
*/
extern void write_log(string msg);

/*
	A custom malloc implementation that writes to the log
	how much memory has been allocated.
*/
extern void* custom_malloc(string func_name, size_t size);

/*
	A custom free implementation that writes to the log
	how much memory has been freed.
*/
extern void custom_free(string func_name, void* ptr);

/*
	Strart our custom instrumentation tracker.
	Side is either "server" or "client".
*/
extern void start_instrum(string func_name, string side, int param);

/*
	Stops the instrumentation.
*/
extern void finish_instrum(string func_name);

#endif
