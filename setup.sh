#!/bin/bash

echo "Installing dependencies..."

sudo apt update
sudo apt install -y build-essential cmake libssl-dev nlohmann-json3-dev

echo "Building project..."

mkdir -p build
cd build
cmake ..
make

echo "Setup complete!"