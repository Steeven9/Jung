# Jung
Bachelor Project at USI, Lugano

## Install gRPC

See the tutorial at [https://grpc.io/docs/languages/cpp/quickstart/](https://grpc.io/docs/languages/cpp/quickstart/)

## Compile and run the example

`mkdir -p cmake/build`

`pushd cmake/build`

`cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..`

`make -j`

`./jung_server`

Then in another terminal run the client (from the `cmake/build` folder)

`./jung_client`
