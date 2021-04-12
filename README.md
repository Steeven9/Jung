# Jung
![](https://img.shields.io/github/license/steeven9/jung)
![](https://img.shields.io/docker/cloud/automated/steeven9/jung)
![](https://img.shields.io/docker/cloud/build/steeven9/jung)
![](https://img.shields.io/tokei/lines/github/steeven9/jung)

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

This will produce two files, `client_log.txt` and `server_log.txt`, that can be merged in a unique trace by running `./trace_merge`. This will in turn produce a unique `trace_log.txt` that contains the unified costs. Alternatively, by passing `--simple` to `trace_merge`, a simple merged log (`merged_log.txt`) can be optained instead of the summary.


## Docker

A server Docker image is available. You can build it with `docker build -t jung ./` and then run it e.g. with `docker run -d -p 50051:50051 -v ~/Jung:/usr/Jung --name jung jung` or simply get it from [Docker Hub](https://hub.docker.com/repository/docker/steeven9/jung) and run it in the same way.


## Running on another machine

If your server is running on another machine (which is probably the case), simply pass its address/hostname to the client as parameter (port is `50051` by default):

`./jung_client --target=HOSTNAME[:PORT]`
