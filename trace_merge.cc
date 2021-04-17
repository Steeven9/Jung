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
#include <tuple>

#include "trace_merge.h"

using namespace std;

vector<tuple<int, int>> server_log_indices;
vector<string> server_log_lines;

/* 
    Helper function to parse the server logfile
    and store indices of lines to simplify
    and speed up further access.
*/
void preprocess_server_log() {
    ifstream server_log;

    server_log.open(SERVER_LOGFILE);

    if (!server_log.is_open()) {
        cerr << "Error: cannot open server log" << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    int line_num = 0;
    while(getline(server_log, line)) {
        server_log_lines.push_back(line);

        if (line.find(" FUNC_START") != string::npos) {
            size_t pos;
            vector<string> line_vect;
            while ((pos = line.find(" ")) != string::npos) {
                line_vect.push_back(line.substr(0, pos));
                line.erase(0, pos + 1);
            }
            int index = stoi(line_vect[2]);
            server_log_indices.push_back(make_tuple(index, line_num));
        }

        ++line_num;
    }

    server_log.close();
}

/*
    Helper function to get the line number of 
    the start of a given RPC server execution.
*/
int get_line_num(int RPC_id) {
    int line_num;
    for (auto t : server_log_indices) {
        if (get<0>(t) == RPC_id) {
            line_num = get<1>(t);
            break;
        }
    }
    return line_num;
}

/* 
    Helper function to get the execution time on 
    server side.
*/
long calc_server_time(string RPC_id) {
    int line_num = get_line_num(stoi(RPC_id));

    string line = server_log_lines[line_num];
    long start_time = stol(line.substr(0, line.find(" ")));
    
    //Search for end time skipping eventual other lines
    while (line.find(" " + RPC_id + " FUNC_END") == string::npos) {
        line = server_log_lines[++line_num];
    }
    
    return stol(line.substr(0, line.find(" "))) - start_time;
}

/* 
    Helper function to get the memory usage 
    or possible memory leaks on server side.
*/
long calc_server_memory(string RPC_id, bool return_leaks) {
    long mem_usage = 0;
    int mem_leaks = 0;

    int line_num = get_line_num(stoi(RPC_id));

    string line = server_log_lines[line_num];
    while (line.find(" " + RPC_id + " FUNC_END") == string::npos) {
        if (line.find(" " + RPC_id + " malloc") != string::npos) {
            mem_usage = stol(line.substr(line.find("malloc ") + 7));
            ++mem_leaks;
        }
        if (line.find(" " + RPC_id + " realloc") != string::npos) {
            cout << "Found realloc call (RPC #" << RPC_id << "). Memory stats might be inaccurate" << endl;
        }
        if (line.find(" " + RPC_id + " free") != string::npos) {
            --mem_leaks;
        }
        line = server_log_lines[++line_num];
    }

    return return_leaks ? mem_leaks : mem_usage;
}

void generate_perf_trace() {
    ifstream client_log;
    ofstream trace_log;

    client_log.open(CLIENT_LOGFILE);
    trace_log.open(TRACE_LOGFILE);

    if (!client_log.is_open()) {
        cerr << "Error: cannot open client log" << endl;
        exit(EXIT_FAILURE);
    }

    if (!trace_log.is_open()) {
        cerr << "Error: cannot write trace log" << endl;
        exit(EXIT_FAILURE);
    }

    preprocess_server_log();

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
        vector<basic_feature*> feature_list;
        string param_name;
        string param_value;
        for (int i = 3; i < line_vect.size(); ++i) {
            param_name = line_vect[i].substr(0, line_vect[i].find("="));
            param_value = line_vect[i].substr(line_vect[i].find("=") + 1);
            feature_list.push_back(make_feature(param_name, param_value));
        }
        
        func = make_custom_func(line_vect[1], feature_list);
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
            RPC_start_time = stol(line_vect[0]);
        }

        //RPC end
        if (line_vect[2] == "RPC_end") {
            long server_time = calc_server_time(line_vect[3]);

            func->server_time += server_time;
            func->network_time += stol(line_vect[0]) - RPC_start_time - server_time;
            func->server_memory += calc_server_memory(line_vect[3], false);
            func->server_memory_leaks += calc_server_memory(line_vect[3], true);
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

    client_log.open(CLIENT_LOGFILE);
    server_log.open(SERVER_LOGFILE);
    merged_log.open(MERGED_LOGFILE);

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
    string tmp_line;
    while(getline(client_log, line)) {
        tmp_line = line;
        // If RPC request, add server data
        if (line.find("RPC_end") != string::npos) {
            string RPC_id = line.substr(line.find("RPC_end ") + 8);
            getline(server_log, line);
            while (line.find(" " + RPC_id + " FUNC_END") == string::npos) {
                merged_log << line + " [server]" << endl;
                getline(server_log, line);
            }
            merged_log << line + " [server]" << endl;
        }
        merged_log << tmp_line << endl;
    }

    cout << "Simple merge completed successfully" << endl;

    client_log.close();
    server_log.close();
    merged_log.close();
}

int main(int argc, char** argv) {    
    if (argc > 1 && strcmp(argv[1], "--simple") == 0) {
        simple_merge();
    } else {
        generate_perf_trace();
    }    
}
