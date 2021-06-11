#!/usr/bin/env bash
echo 'Jung & Freud install script v1.0'

echo '== Cloning repositories =='
git clone https://github.com/Steeven9/Jung
git clone https://github.com/usi-systems/freud

echo '== Installing dependencies =='
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    sudo apt install -y build-essential pkg-config protobuf-compiler-grpc libgrpc-dev libgrpc++-dev r-base r-base-dev gnuplot
    export R_HOME=/usr/local/lib/R
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # MacOS
    brew install grpc gnuplot r
    export R_HOME=/usr/local/lib/R
    export PKG_CONFIG_PATH=/usr/local/opt/openssl/lib/pkgconfig:/usr/local/opt/grpc/lib/pkgconfig:$PKG_CONFIG_PATH
else
    # Unknown
    echo 'Unknown system type, skipping dependencies auto-installation.'
fi

echo '== Compiling =='
cd freud/freud-statistics
git pull
make

cd ../../Jung
git pull
make

echo '== Done! =='
echo 'Some environment variables have been automatically set in this shell.'
echo 'Please consult the READMEs of both projects for more information.'
