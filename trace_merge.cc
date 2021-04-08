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
    string server_line;
    vector<string> line_vect;
    size_t pos;
    custom_func * func;
    long startTime;

    //First line - initialize struct
    getline(client_log, line);
    //Separate the line on spaces and put the tokens in vector
    while ((pos = line.find(" ")) != string::npos) {
        line_vect.push_back(line.substr(0, pos));
        line.erase(0, pos + 1);
    }
    line_vect.push_back(line);

    if (line_vect[2] == "FUNC_START") {
        //TODO variable number of params
        //TODO correct variable params typing (not int)
        //TODO correctly pass feature object?
        string param_name = line_vect[3].substr(0, line_vect[3].find("="));
        int param_value = stoi(line_vect[3].substr(line_vect[3].find("=") + 1));
        func = make_custom_func(line_vect[1], { make_feature(param_name, param_value) });
        startTime = stol(line_vect[0]);
    } else {
        cerr << "Error: incorrect logfile format" << endl;
        exit(EXIT_FAILURE);
    }

    //Get the client logfile line by line
    while(getline(client_log, line)) {
        line_vect.clear();

        merged_log << line << endl;

        //Separate the line on spaces and put the tokens in vector
        while ((pos = line.find(" ")) != string::npos) {
            line_vect.push_back(line.substr(0, pos));
            line.erase(0, pos + 1);
        }
        line_vect.push_back(line);

        //Memory allocation
        if (line_vect[2] == "malloc") {
            func->memory_usage += stoi(line_vect[3]);
            ++func->mem_leaks;
        }

        //Memory freeing
        if (line_vect[2] == "free") {
            --func->mem_leaks;
        }

        //RPC start
        if (line_vect[2] == "RPC_start") {
            //TODO network time, server time, server memory usage
            //TODO save RPC ID to recall server info
            getline(server_log, server_line);
            merged_log << server_line + " [server]" << endl;
        }

        //RPC end
        if (line_vect[2] == "RPC_end") {
            //TODO same as above
            getline(server_log, server_line);
            merged_log << server_line + " [server]" << endl;
        }

        //Function end - done
        if (line_vect[2] == "FUNC_END") {
            func->exec_time = stol(line_vect[0]) - startTime;
        }
    }

    cout << "Merge completed successfully" << endl;

    cout << func->print() << endl;

    client_log.close();
    server_log.close();
    merged_log.close();
}

int main(int argc, char** argv) {
    merge_traces();
}
