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
#include <string.h>

#include "trace_merge.h"

using namespace std;

/* 
    Helper function to get the execution time on 
    server side.
*/
long calc_server_time(string RPC_id) {
    ifstream server_log;
    long result = -1;

    server_log.open("server_log.txt");

    if (!server_log.is_open()) {
        cerr << "Error: cannot open server log" << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    while(getline(server_log, line)) {
        if (line.find(RPC_id + " FUNC_START") != string::npos) {
            //TODO skip eventual malloc lines
            //TODO preprocess log file and save indices?
            long start_time = stol(line.substr(0, line.find(" ")));
            getline(server_log, line);
            result = stol(line.substr(0, line.find(" "))) - start_time;
        }
    }

    server_log.close();
    return result;
}

/* 
    Helper function to get the memory usage on 
    server side.
*/
long calc_server_memory(string RPC_id) {
    ifstream server_log;
    long result = -1;

    server_log.open("server_log.txt");

    if (!server_log.is_open()) {
        cerr << "Error: cannot open server log" << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    while(getline(server_log, line)) {
        if (line.find(RPC_id + " malloc") != string::npos) {
            //TODO actually get the correct amount of memory
            result = stol(line.substr(line.find("malloc ") + 7));
        }
    }

    server_log.close();
    return result;
}

void generate_perf_trace() {
    ifstream client_log;
    ofstream trace_log;

    client_log.open("client_log.txt");
    trace_log.open("trace_log.txt");

    if (!client_log.is_open()) {
        cerr << "Error: cannot open client log" << endl;
        exit(EXIT_FAILURE);
    }

    if (!trace_log.is_open()) {
        cerr << "Error: cannot write trace log" << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    vector<string> line_vect;
    size_t pos;
    custom_func * func;
    long start_time;
    long RPC_start_time;

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
        start_time = stol(line_vect[0]);
    } else {
        cerr << "Error: incorrect logfile format (no start)" << endl;
        exit(EXIT_FAILURE);
    }

    //Get the client logfile line by line
    while(getline(client_log, line)) {
        line_vect.clear();

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
            //TODO network time, server time, server memory usage, server mem leaks

            RPC_start_time = stol(line_vect[0]);
        }

        //RPC end
        if (line_vect[2] == "RPC_end") {
            //TODO same as above

            long server_time = calc_server_time(line_vect[3]);

            func->server_time += server_time;
            func->network_time += stol(line_vect[0]) - RPC_start_time - server_time;
            func->server_memory += calc_server_memory(line_vect[3]);
        }

        //Function end - done
        if (line_vect[2] == "FUNC_END") {
            func->exec_time = stol(line_vect[0]) - start_time;
        }
    }

    if (line_vect[2] == "FUNC_END") {
        cout << "Trace generation successful" << endl;
    } else {
        cerr << "Error: incorrect logfile format (no end)" << endl;
        exit(EXIT_FAILURE);
    }

    cout << func->print() << endl;

    //TODO figure out the proper format to encode it in
    trace_log << func->print() << endl;

    client_log.close();
    trace_log.close();
}

void simple_merge() {
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
        if (line.find("RPC_start") != string::npos) {
            getline(server_log, line);
            merged_log << line + " [server]" << endl;
            getline(server_log, line);
            merged_log << line + " [server]" << endl;
        }
    }

    cout << "Simple merge completed successfully" << endl;

    client_log.close();
    server_log.close();
    merged_log.close();
}

int main(int argc, char** argv) {    
    if (argc > 1 && strcmp(argv[1], "--simple") == 0) {
        //TODO fix the id lookup if server log is already started
        simple_merge();
    } else {
        generate_perf_trace();
    }    
}
