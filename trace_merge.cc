/*
 *
 * Copyright 2021 Stefano Taillefert and Daniele Rogora.
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
#include <unordered_map>
#include <set>
#include <sys/stat.h>

#include "trace_merge.h"

using namespace std;

vector<tuple<int, int>> server_log_indices;
vector<string> server_log_lines;

/* 
    Helper function to parse the server log file
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
uint64_t calc_server_time(string RPC_id) {
    int line_num = get_line_num(stoi(RPC_id));

    string line = server_log_lines[line_num];
    uint64_t start_time = stol(line.substr(0, line.find(" ")));
    
    // Search for end time skipping eventual other lines
    while (line.find(" " + RPC_id + " FUNC_END") == string::npos) {
        line = server_log_lines[++line_num];
    }
    
    return stol(line.substr(0, line.find(" "))) - start_time;
}

/* 
    Helper function to get the memory usage 
    and possible memory leaks on server side.
*/
tuple<uint64_t, uint64_t> calc_server_memory(string RPC_id) {
    uint64_t mem_usage = 0;
    uint64_t mem_leaks = 0;

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

    return {mem_usage, mem_leaks};
}

/* 
    Helper function to get the pagefaults 
    on server side.
*/
tuple<uint64_t, uint64_t> calc_server_pagefaults(string RPC_id) {
    tuple<uint64_t, uint64_t> result;
    int line_num = get_line_num(stoi(RPC_id));

    string line = server_log_lines[line_num];
    while (line.find(" " + RPC_id + " FUNC_END") == string::npos) {
        if (line.find(" " + RPC_id + " pagefault") != string::npos) {
            size_t pos;
            vector<string> line_vect;
            while ((pos = line.find(" ")) != string::npos) {
                line_vect.push_back(line.substr(0, pos));
                line.erase(0, pos + 1);
            }
            line_vect.push_back(line);
            result = {stol(line_vect[4]), stol(line_vect[5])};
        }
        line = server_log_lines[++line_num];
    }

    return result;
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
    unordered_map<string, custom_func *> func_list;

    // Get the client log file line by line
    while(getline(client_log, line)) {
        line_vect.clear();

        // Separate the line on spaces and put the tokens in vector
        while ((pos = line.find(" ")) != string::npos) {
            line_vect.push_back(line.substr(0, pos));
            line.erase(0, pos + 1);
        }
        line_vect.push_back(line);

        // Extract sample uid and func name
        // (e.g do_stuff1 is the first run of do_stuff).
        // Regexes credit: some random stackoverflow post
        uint32_t uid = stoi(regex_replace(
            line_vect[1],
            regex("[^0-9]*([0-9]+).*"),
            string("$1")
        ));
        string f_name = regex_replace(
            line_vect[1],
            regex("([^0-9]+).*"),
            string("$1")
        );

        if (line_vect[2] == "FUNC_START") {
            vector<feature*> feature_list;
            string param_name;
            string param_type;
            string param_value;
            for (int i = 3; i < line_vect.size(); ++i) {
                // Format: e.g. asd=int&12
                param_name = line_vect[i].substr(0, line_vect[i].find("="));
                param_type = line_vect[i].substr(line_vect[i].find("=") + 1, line_vect[i].find("&") - line_vect[i].find("=") - 1);
                param_value = line_vect[i].substr(line_vect[i].find("&") + 1);
                feature_list.push_back(make_feature(param_name, param_type, param_value));
            }
            
            if (uid == 1) {
                // First run of the function, create structs
                func_list[f_name] = make_custom_func(f_name);
            }
            func_list[f_name]->sample_list[uid] = make_sample(uid);
            func_list[f_name]->sample_list[uid]->feature_list = feature_list;
            func_list[f_name]->sample_list[uid]->start_time = stol(line_vect[0]);
        }

        // Memory allocation
        if (line_vect[2] == "malloc") {
            func_list[f_name]->sample_list[uid]->memory_usage += stoi(line_vect[3]);
            ++func_list[f_name]->sample_list[uid]->mem_leaks;
        }

        //Memory freeing
        if (line_vect[2] == "free") {
            --func_list[f_name]->sample_list[uid]->mem_leaks;
        }

        // RPC start
        if (line_vect[2] == "RPC_start") {
            func_list[f_name]->sample_list[uid]->RPC_start_time = stol(line_vect[0]);
        }

        // RPC end
        if (line_vect[2] == "RPC_end") {
            uint64_t server_time = calc_server_time(line_vect[3]);
            func_list[f_name]->sample_list[uid]->server_time += server_time;
            func_list[f_name]->sample_list[uid]->network_time += stol(line_vect[0]) - 
                func_list[f_name]->sample_list[uid]->RPC_start_time - server_time;

            tuple<uint64_t, uint64_t> server_mem = calc_server_memory(line_vect[3]);
            func_list[f_name]->sample_list[uid]->server_memory_usage += get<0>(server_mem);
            func_list[f_name]->sample_list[uid]->server_mem_leaks += get<1>(server_mem);

            tuple<uint64_t, uint64_t> server_pagefaults = calc_server_pagefaults(line_vect[3]);
            func_list[f_name]->sample_list[uid]->server_min_pagefault += get<0>(server_pagefaults);
            func_list[f_name]->sample_list[uid]->server_maj_pagefault += get<1>(server_pagefaults);
        }

        // Pagefault (minor and major)
        if (line_vect[2] == "pagefault") {
            func_list[f_name]->sample_list[uid]->min_pagefault += stoi(line_vect[3]);
            func_list[f_name]->sample_list[uid]->maj_pagefault += stoi(line_vect[4]);
        }

        // Waiting time (lock)
        if (line_vect[2] == "mutex_lock") {
            func_list[f_name]->sample_list[uid]->waiting_time += stoi(line_vect[3]);
        }

        // Lock holding time
        if (line_vect[2] == "mutex_unlock") {
            func_list[f_name]->sample_list[uid]->lock_holding_time += stoi(line_vect[3]);
        }

        // Waiting time (cond_wait)
        if (line_vect[2] == "cond_wait_returned") {
            func_list[f_name]->sample_list[uid]->waiting_time += stoi(line_vect[3]);
        }

        // Waiting time (cond_timedwait)
        if (line_vect[2] == "cond_timedwait_returned") {
            func_list[f_name]->sample_list[uid]->waiting_time += stoi(line_vect[3]);
        }

        // Function end - done
        if (line_vect[2] == "FUNC_END") {
            func_list[f_name]->sample_list[uid]->exec_time = stol(line_vect[0]) - 
                func_list[f_name]->sample_list[uid]->start_time;
        }
    }

    if (line_vect[2] == "FUNC_END") {
        cout << "Trace generation successful\n" << endl;
    } else {
        cerr << "Error: incorrect log file format (no end)" << endl;
        exit(EXIT_FAILURE);
    }

    for (const auto& f : func_list) {
        cout << f.second->name << endl;
        trace_log << f.second->name << endl;
        for (const auto& s : f.second->sample_list) {
            cout << "Run #" << s.second->uid << endl;
            trace_log << "Run #" << s.second->uid << endl;
            cout << s.second->print() << "\n" << endl;
            trace_log << s.second->print() << "\n" << endl;
        }
    }

    encode_perf_trace(func_list);

    client_log.close();
    trace_log.close();
}

void encode_perf_trace(unordered_map<string, custom_func *> func_list) {
    for (const auto& f : func_list) {
        string rtn_name = f.second->name;
        time_t now = time(NULL);
        stringstream now_ss;
        now_ss.str("");
        mkdir("symbols/", S_IRWXU | S_IRWXG);
        string folder = "symbols/" + rtn_name;
        now_ss << folder << "/idcm_" << rtn_name << "_" << now << ".bin";

        mkdir(folder.c_str(), S_IRWXU | S_IRWXG);
        ofstream out_file(now_ss.str().c_str(), ios::binary);
        if (!out_file.is_open()) {
            cerr << "Error: cannot open " << now_ss.str() << endl;
            exit(EXIT_FAILURE);
        }

        // Function name
        uint32_t name_len = rtn_name.size();
        out_file.write((char *)&name_len, sizeof(uint32_t));
        out_file.write(rtn_name.c_str(), sizeof(char) * name_len);

        // Feature names
        unordered_map<string, uint64_t> fname_offsets;
		unordered_map<string, uint64_t> ftype_offsets;
		set<string> ftype_names;
		uint32_t tot_fnames = 0;
		fpos<mbstate_t> tot_fnames_position = out_file.tellp();
		out_file.write((char *)&tot_fnames, sizeof(uint32_t));

        for (const auto& s : f.second->sample_list) {
            for (feature* pf: s.second->feature_list) {
                string fname = pf->name;
                if (fname_offsets.find(fname) == fname_offsets.end()) {
                    fname_offsets.insert(make_pair(fname, out_file.tellp()));
                    ftype_names.insert(pf->type);
                    uint16_t fname_len = fname.size();
                    out_file.write((char *)&fname_len, sizeof(uint16_t));
                    out_file.write(fname.c_str(), sizeof(char) * fname_len);
                    tot_fnames++;
                }
            }
        }

        // (No system variables)

        // Type names
        uint32_t tcount = ftype_names.size();
		out_file.write((char *)&tcount, sizeof(uint32_t));
		for (string t: ftype_names) {
			ftype_offsets.insert(make_pair(t, out_file.tellp()));
			uint16_t ftype_len = t.size();
			out_file.write((char *)&ftype_len, sizeof(uint16_t));
			out_file.write(t.c_str(), sizeof(char) * ftype_len);
		}

        // Go back and write the correct number of features
        fpos<mbstate_t> prev_pos = out_file.tellp();
		out_file.seekp(tot_fnames_position);
		out_file.write((char *)&tot_fnames, sizeof(uint32_t));
		out_file.seekp(prev_pos);

        // Number of samples
        uint32_t samples_count = f.second->sample_list.size();
        fpos<mbstate_t> num_of_samples_position = out_file.tellp();
		out_file.write((char *)&samples_count, sizeof(uint32_t));

        // Uid and metrics
        for (const auto& s : f.second->sample_list) {
            uint64_t tot_mem = s.second->memory_usage + s.second->server_memory_usage;
            uint64_t tot_lock_holding_time = s.second->server_lock_holding_time + s.second->lock_holding_time;
            uint64_t tot_waiting_time = s.second->server_waiting_time + s.second->waiting_time;
            uint64_t tot_min_faults = s.second->server_min_pagefault + s.second->min_pagefault;
            uint64_t tot_maj_faults = s.second->server_maj_pagefault + s.second->maj_pagefault;
            out_file.write((char *)&s.second->uid, sizeof(uint32_t));
            out_file.write((char *)&s.second->exec_time, sizeof(uint64_t));
            out_file.write((char *)&tot_mem, sizeof(uint64_t));
            out_file.write((char *)&tot_lock_holding_time, sizeof(uint64_t));
            out_file.write((char *)&tot_waiting_time, sizeof(uint64_t));
            out_file.write((char *)&tot_min_faults, sizeof(uint64_t));
            out_file.write((char *)&tot_maj_faults, sizeof(uint64_t));
            

            // Num of features
            uint32_t pn = 0, tot_features = 0;
            fpos<mbstate_t> tf_pos = out_file.tellp();
            out_file.write((char *)&tot_features, sizeof(uint32_t));
            uint32_t rot_idx = 0;
            string runtime_type;

            // Local and global features
            for (auto feat : s.second->feature_list) {
                // We should have only primitives
                runtime_type = "0";

                // ...skipping some checks...

                uint64_t offs = fname_offsets[feat->name];
                uint64_t toffs = ftype_offsets[feat->type];
                out_file.write((char *)&offs, sizeof(uint64_t));
                out_file.write((char *)&toffs, sizeof(uint64_t));
                int64_t v;
                if (feat->type == "double") {
                    v = stod(feat->value);
                } else if (feat->type == "int") {
                    v = stoi(feat->value);
                } else if (feat->type == "float") {
                    v = stof(feat->value);
                } else if (feat->type == "long") {
                    v = stol(feat->value);
                } else {
                    cerr << "Error: unknown feature type (" << feat->type << ") for " << rtn_name << endl;
                    exit(EXIT_FAILURE);
                }
                out_file.write((char *)&v, sizeof(int64_t));
                ++tot_features;
            }

            if (tot_features != s.second->feature_list.size()) {
                cerr << "Error: features amount mismatch for " << rtn_name << endl;
                exit(EXIT_FAILURE);
            }

            // System features (not used)
            prev_pos = out_file.tellp();
            out_file.seekp(tf_pos);
            out_file.write((char *)&tot_features, sizeof(uint32_t));
            out_file.seekp(prev_pos);

            // Branches (not used)
            uint32_t num_of_branches = 0;
            out_file.write((char *)&num_of_branches, sizeof(uint32_t));

            // Children (not used)
            uint32_t num_of_children = 0;
            out_file.write((char *)&num_of_children, sizeof(uint32_t));
        }

        // prev_pos = out_file.tellp();
		// out_file.seekp(num_of_samples_position);
		// out_file.write((char *)&samples_count, sizeof(uint32_t));
		// out_file.seekp(prev_pos);
        out_file.close();
    }
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
    if (argc > 1) {
        if (strcmp(argv[1], "--simple") == 0) {
            simple_merge();
        } else {
            cerr << "Usage: " << argv[0] << " [--simple]" << endl;
            return EXIT_FAILURE;
        }
    } else {
        generate_perf_trace();
    }

    return EXIT_SUCCESS;
}
