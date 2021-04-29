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

This will produce two files, `client_log.txt` and `server_log.txt`, that can be merged in a unique trace by running `./trace_merge`. 
This will in turn produce a unique `trace_log.txt` that contains the unified costs encoded in binary format. 
Alternatively, by running `./trace_merge --simple`, a simple merged log (`merged_log.txt`) can be optained instead.
If your server is running on another machine, be sure to retrieve the logfile before merging.

_Note_: all the file names are customizable in `custom_instr.h`.

_Disclaimer_: the memory usage counter is not keeping track of the variations due to `realloc` calls. 
While the library provides a warning for potential memory leaks, this might be inaccurate due to the complexity of memory managment in C.
If you get any warnings, consider running your application through a dedicated tool like [Valgrind](https://valgrind.org/).
If you don't get any, consider doing it anyway, don't trust me.


## Docker

A Docker image of the example server is available on [Docker Hub](https://hub.docker.com/repository/docker/steeven9/jung), which you can spin up with `docker-compose up`.
You can also build it yourself with `docker build -t jung ./`.


## Running on another machine

If your server is running on another machine (which is probably the case), simply pass its address/hostname 
to the client as parameter (port is `50051` by default, editable in `jung_server.cc`):

`./jung_client --target=HOSTNAME[:PORT]`


## Using the library

If you want to measure your own application, simply include `custom_instr.h`, which provides all the necessary
functions to manage the instrumentation. Make sure to also include the corresponding `.cc` file in your compilation unit.

To merge the obtained traces, compile and run `trace_merge.cc` (which requires `custom_instr.h` as well).
