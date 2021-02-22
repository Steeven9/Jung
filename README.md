# Jung
Bachelor Project at USI, Lugano.

A tool for Instrumentation and Performance Analysis of Distributed Systems based on [Freud](https://github.com/usi-systems/freud) and [gRPC](https://grpc.io).


## Install gRPC

See the tutorial at [https://grpc.io/docs/languages/cpp/quickstart/](https://grpc.io/docs/languages/cpp/quickstart/)


## Compile and run the example

`mkdir -p cmake/build`

`pushd cmake/build`

`cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..`

`make`

`./jung_server`

Then in another terminal run the client (from the `cmake/build` folder)

`./jung_client`


## Running on another machine

If your server is running on another machine (which is probably the case), simply pass its address/hostname to the client as parameter (port is `50051` by default):

`./jung_client --target=HOSTNAME[:PORT]`
