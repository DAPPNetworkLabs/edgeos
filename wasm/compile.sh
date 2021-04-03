#! /bin/sh
set -e
cd build
cmake .. 
make
# ipfs --api=/dns/ipfs.liquidapps.io/tcp/5001 add kernel/kernel.wasm
cd ..
#wasicc kernel.cpp -o kernel.wasm
# cp main.defaults.yaml kernel.yaml
# xxd -p kernel.wasm | tr -d '\n' >> kernel.yaml
