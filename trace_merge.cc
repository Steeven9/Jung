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

#include <fstream>
#include <iostream>
#include <vector>

#include "custom_instr.h"

using namespace std;

struct custom_func {
    string name;
    int exec_time;
    int network_time;
    int server_time;
    int memory_usage;
    int server_memory;
    const vector<basic_feature*> & feature_list;

    custom_func(const string & n, const vector<basic_feature*> & f_l)
	: name(n), feature_list(f_l) {};

    virtual string print() const {
		return name + " took " + to_string(exec_time) + "ms, of which " + 
            to_string(network_time) + "ms in network and " + to_string(server_time) + 
            " in server. Used " + to_string(memory_usage) + " memory client-side and " + 
            to_string(server_memory) + " memory server-side.";
    }

    virtual void addTime(int t) {
        exec_time += t;
    }

    virtual void addServerTime(int t) {
        server_time += t;
    }
};


custom_func * make_custom_func(const string & n, const vector<basic_feature*> & f_l) {
    return new custom_func(n, f_l);
}

/* 
    Read a line from the client log, then if there
    is a RPC request, search the relevant data in the
    server log and append it.
*/
int main() {
    ifstream client_log;
    ifstream server_log;
    ofstream merged_log;

    client_log.open("client_log.txt");
    server_log.open("server_log.txt");
    merged_log.open("merged_log.txt");

    if (!client_log.is_open()) {
        cerr << "Error: cannot open client log" << endl;
        exit(EXIT_FAILURE);
    }

    if (!server_log.is_open()) {
        cerr << "Error: cannot open server log" << endl;
        exit(EXIT_FAILURE);
    }

    if (!merged_log.is_open()) {
        cerr << "Error: cannot write merged log" << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    while(getline(client_log, line)) {
        merged_log << line << endl;
        // If RPC request, add server data
        if (line.find("RPC START") != string::npos) {
            getline(server_log, line);
            merged_log << line + " [server]" << endl;
            getline(server_log, line);
            merged_log << line + " [server]" << endl;
        }
    }

    cout << "Merge completed successfully" << endl;

    client_log.close();
    server_log.close();
    merged_log.close();
}
