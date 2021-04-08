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

#include "trace_merge.h"

using namespace std;

void merge_traces() {
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

int main() {
    merge_traces();
}
