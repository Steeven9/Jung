# Jung
Bachelor Project at USI, Lugano.

A tool for Instrumentation and Performance Analysis of Distributed Systems based on [Freud](https://github.com/usi-systems/freud) and [gRPC](https://grpc.io).


## Install gRPC and protobuf

### Linux
`apt install protobuf-compiler-grpc libgrpc-dev libgrpc++-dev`

### macOS
`brew install grpc`

`export PKG_CONFIG_PATH=/usr/local/opt/openssl/lib/pkgconfig:/usr/local/opt/grpc/lib/pkgconfig:$PKG_CONFIG_PATH`

### Windows
Get a Linux VM


## Compile and run the example

`make`

`./jung_server`

Then, in another terminal, run the client

`./jung_client`

This will produce two files, `client_log.txt` and `server_log.txt`, that can be merged in a unique trace by running `./trace_merge`. This will in turn produce a unique `merged_log.txt` that contains the unified costs. This file can then be analyzed with `freud-statistics`.


## Docker

A server Docker image is available. You can build it with `docker build -t jung ./` and then run it e.g. with `docker run -d -p 50051:50051 -v ~/Jung:/usr/Jung --name jung jung` or simply get it from [Docker Hub](https://hub.docker.com/repository/docker/steeven9/jung).


## Running on another machine

If your server is running on another machine (which is probably the case), simply pass its address/hostname to the client as parameter (port is `50051` by default):

`./jung_client --target=HOSTNAME[:PORT]`
