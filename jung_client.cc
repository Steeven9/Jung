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
#include <memory>
#include <stdlib.h>
#include <string>
#include <filesystem>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "jung.grpc.pb.h"
#include "custom_instr.h"

#define SERVER_PORT 50051
#define NUM_MSG 20
#define NUM_POINTS 3
#define NUM_THREADS 4
#define CLEAR_LOG true

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using jung::JungRequest;
using jung::JungReply;
using jung::Jung;

using namespace std;

string server_address = "localhost:" + to_string(SERVER_PORT);

class JungClient {
	public:
		JungClient(shared_ptr<Channel> channel): stub_(Jung::NewStub(channel)) {}

		// Assembles the client's payload, sends it and presents the response back
		// from the server.
		JungReply Greet(const string& message) {
			// Data we are sending to the server.
			JungRequest request;
			request.set_message(message);

			// Container for the data we expect from the server.
			JungReply reply;

			// Context for the client. It could be used to convey extra information to
			// the server and/or tweak certain RPC behaviors.
			ClientContext context;

			// The actual RPC.
			Status status = stub_->Greet(&context, request, &reply);

			// Act upon its status.
			if (!status.ok()) {
				cerr << "Error #" << status.error_code() << ": " << status.error_message() << endl;
				exit(EXIT_FAILURE);
			}
			return reply;
		}

		JungReply ReturnDouble(const string& message) {
			// Data we are sending to the server.
			JungRequest request;
			request.set_message(message);

			// Container for the data we expect from the server.
			JungReply reply;

			// Context for the client. It could be used to convey extra information to
			// the server and/or tweak certain RPC behaviors.
			ClientContext context;

			// The actual RPC.
			Status status = stub_->ReturnDouble(&context, request, &reply);

			// Act upon its status.
			if (!status.ok()) {
				cerr << "Error #" << status.error_code() << ": " << status.error_message() << endl;
				exit(EXIT_FAILURE);
			}
			return reply;
		}

	private:
		unique_ptr<Jung::Stub> stub_;
};

/*
	The function to be analyzed. Takes an int parameter
	that should make the complexity scale.
*/
void do_stuff(unsigned int param) {
	string func_name(__func__);
	func_name += to_string(getNextUid(func_name));
	start_instrum(func_name, client, { make_feature("param", "int", to_string(param)), 
										make_feature("useless", "double", to_string(12.2)) });

	JungClient jung(grpc::CreateChannel(
		server_address, grpc::InsecureChannelCredentials()));

	// Allocate some memory so we can track it
	custom_malloc(func_name, param);

	// Send the "ciao" messages
	for (int i = 0; i < param; ++i) {
		string message("mamma " + to_string(param));
		write_log(func_name, "RPC_start");
		JungReply reply = jung.Greet(message);

		cout << "Sent: " << message << endl;
		cout << "Received: " << reply.message() << endl;

		// Save the resulting id from the RPC call
		write_log(func_name, "RPC_end " + to_string(reply.id()));
	}

	// Send the Double messages
	for (int i = 0; i < param; ++i) {
		string message(to_string(param));
		write_log(func_name, "RPC_start");
		JungReply reply = jung.ReturnDouble(message);

		cout << "Sent: " << message << endl;
		cout << "Received: " << reply.message() << endl;

		// Save the resulting id from the RPC call
		write_log(func_name, "RPC_end " + to_string(reply.id()));
	}

	finish_instrum(func_name);
}

/*
	A multithreaded function to be analyzed. Takes an 
	int parameter that should make the complexity scale.
*/
void do_multi_stuff(unsigned int param, custom_mutex * mutex) {
	string func_name(__func__);
	func_name += to_string(getNextUid(func_name));
	start_instrum(func_name, client, { make_feature("param", "int", to_string(param)), 
										make_feature("useless", "int", to_string(42069)) });
	// Acquire lock and hold for param sec
	custom_pthread_mutex_lock(func_name, mutex);
	cout << "T" << this_thread::get_id() << " holding for " << param << " seconds..." << endl;
	this_thread::sleep_for(chrono::seconds(param));
	cout << "T" << this_thread::get_id() << " releasing lock..." << endl;
	custom_pthread_mutex_unlock(func_name, mutex);

	finish_instrum(func_name);
}

int main(int argc, char** argv) {
	// Instantiate the client. It requires a channel, out of which the actual RPCs
	// are created. This channel models a connection to an endpoint specified by
	// the argument "--target=" which is the only expected argument.
	// We indicate that the channel isn't authenticated (use of
	// InsecureChannelCredentials()).
	string arg_str("--target");

	if (argc > 1) {
		string arg_val = argv[1];
		size_t start_pos = arg_val.find(arg_str);

		if (start_pos != string::npos) {
			start_pos += arg_str.size();

			if (arg_val[start_pos] == '=') {
				server_address = arg_val.substr(start_pos + 1);

				// Add default port if not explicitly passed
				if (server_address.find(":") == string::npos) {
					server_address += ":" + to_string(SERVER_PORT);
				}
			} else {
				cerr << "Usage: " << argv[0] << " [--target=hostname]" << endl;
				return EXIT_FAILURE;
			}
		} else {
			cerr << "Usage: " << argv[0] << " [--target=hostname]" << endl;
			return EXIT_FAILURE;
		}
	}

	if (filesystem::exists(CLIENT_LOGFILE) && CLEAR_LOG) {
		cout << "Removing previous logs..." << endl;
		remove(CLIENT_LOGFILE);
	}

	cout << "Connecting to " << server_address << "..." << endl;

	cout << "-> Starting RPC test..." << endl;
	for (int i = 0; i < NUM_MSG; ++i) {
		cout << "--> Iteration " << i << "/" << NUM_MSG << endl;
		for (int j = 0; j < NUM_POINTS; ++j) {
			do_stuff(i);
		}
	}

	cout << "-> Starting multithreaded test..." << endl;
	custom_mutex lock;
	pthread_mutex_t m;
	lock.mutex = &m;
	custom_mutex_init(&lock, NULL);

	vector<thread> threads;
	for (int i = 0; i < NUM_THREADS; ++i) {
		threads.push_back(thread(do_multi_stuff, NUM_THREADS, &lock));
	}
	
	for (auto &th : threads) {
		th.join();
	}

	cout << "-> Done!" << endl;

	return EXIT_SUCCESS;
}
