#!/bin/bash

echo "Installing dependencies..."

sudo apt update
sudo apt install -y build-essential cmake libssl-dev nlohmann-json3-dev
sudo apt install -y build-essential cmake libssl-dev nlohmann-json3-dev libgtest-dev


cd /usr/src/gtest 
sudo cmake .
sudo make
sudo cp -r lib/*.a /usr/lib

echo "Building project..."

mkdir -p build
cd build
cmake ..
make
ctest

echo "Setup complete!"