/*
 *
 * Copyright 2015 gRPC authors.
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

#include <grpcpp/grpcpp.h>

#include "jung.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using jung::JungRequest;
using jung::JungReply;
using jung::Jung;

using namespace std;

class JungClient {
 public:
  JungClient(shared_ptr<Channel> channel)
      : stub_(Jung::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  string Respond(const string& message) {
    // Data we are sending to the server.
    JungRequest request;
    request.set_message(message);

    // Container for the data we expect from the server.
    JungReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Respond(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      cout << status.error_code() << ": " << status.error_message()
                << endl;
      return "RPC failed";
    }
  }

 private:
  unique_ptr<Jung::Stub> stub_;
};

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  string target_str;
  string arg_str("--target");
  if (argc > 1) {
    string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
		//TODO add default port if not passed
        target_str = arg_val.substr(start_pos + 1);
      } else {
        cout << "The only correct argument syntax is --target=" << endl;
        return 0;
      }
    } else {
      cout << "The only acceptable argument is --target=" << endl;
      return 0;
    }
  } else {
    target_str = "localhost:50051";
  }
  JungClient jung(grpc::CreateChannel(
      target_str, grpc::InsecureChannelCredentials()));

  // Send the message
  string message("mamma");
  string reply = jung.Respond(message);

  cout << "Sent: " << message << endl;
  cout << "Received: " << reply << endl;

  return 0;
}
