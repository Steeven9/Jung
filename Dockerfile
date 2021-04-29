FROM ubuntu:latest

ENV TZ=Europe/Zurich
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt update
RUN apt install -y build-essential pkg-config protobuf-compiler-grpc libgrpc-dev libgrpc++-dev

RUN mkdir -p /usr/Jung
COPY . /usr/Jung
WORKDIR /usr/Jung

RUN make

ENTRYPOINT ["./jung_server"]
