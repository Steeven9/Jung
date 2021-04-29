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
#include <string>
#include <thread>
#include <filesystem>
#include <stdio.h>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "jung.grpc.pb.h"
#include "custom_instr.h"

#define SERVER_PORT 50051
#define VERBOSE true
#define CLEAR_LOG true

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using jung::JungRequest;
using jung::JungReply;
using jung::Jung;

using namespace std;

// Global counter for IDs
int reply_id = 0;

// Logic and data behind the server's behavior.
class JungServiceImpl final : public Jung::Service {
	Status Greet(ServerContext* context, const JungRequest* request,
					JungReply* reply) override {
		string func_name =  __func__;
		func_name += " " + to_string(++reply_id);
		start_instrum(func_name, server, { make_feature("msg_len", request->message().length()) });

		//Allocate a byte of memory but free it immediately
		void* mem_p = custom_malloc(func_name, 1);

		reply->set_message("Ciao " + request->message());
		reply->set_id(reply_id);

		custom_free(func_name, mem_p);

		if (VERBOSE) {
			cout << "Received " << func_name << ": " << request->message() << endl;
		}
		finish_instrum(func_name);
		return Status::OK;
	}

	Status ReturnDouble(ServerContext* context, const JungRequest* request,
							JungReply* reply) override {
		string func_name =  __func__;
		func_name += " " + to_string(++reply_id);
		start_instrum(func_name, server, { make_feature("d", stoi(request->message())) });

		reply->set_message(to_string(stoi(request->message()) * 2));
		reply->set_id(reply_id);

		// allocate some memory without freeing it. Should warn
		custom_malloc(func_name, stoi(request->message()));

		if (VERBOSE) {
			cout << "Received " << func_name << ": " << request->message() << endl;
		}

		// simulate a computation by sleeping for the given seconds / 2
		this_thread::sleep_for(chrono::seconds(stoi(request->message()) / 2));

		finish_instrum(func_name);
		return Status::OK;
	}
};

void run_server() {
	string server_address("0.0.0.0:" + to_string(SERVER_PORT));
	JungServiceImpl service;

	grpc::EnableDefaultHealthCheckService(true);
	grpc::reflection::InitProtoReflectionServerBuilderPlugin();
	ServerBuilder builder;
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
	builder.RegisterService(&service);
	// Finally assemble the server.
	unique_ptr<Server> server(builder.BuildAndStart());
	cout << "Jung server listening on " << server_address << endl;

	// Wait for the server to shutdown. Note that some other thread must be
	// responsible for shutting down the server for this call to ever return.
	server->Wait();
}

int main(int argc, char** argv) {
	if (filesystem::exists(SERVER_LOGFILE) && CLEAR_LOG) {
		cout << "Removing previous logs..." << endl;
		remove(SERVER_LOGFILE);
	}

	run_server();

	return 0;
}
