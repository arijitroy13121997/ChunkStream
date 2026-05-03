# ChunkStream

## Overview

ChunkStream is a C++17 TCP-based file transfer utility designed to reliably transfer large files (up to 16GB) across networked systems.

Features:

* Chunk-based file transfer
* End-to-end integrity verification using SHA-256
* Error handling with retry mechanism
* Multi-client support using a thread pool
* Cross-platform compatibility (Linux and Windows)

---

## Architecture

### Components

Sender:

* Reads file from disk
* Splits file into fixed-size chunks
* Sends chunks sequentially with acknowledgement

Receiver:

* Accepts incoming connections
* Processes protocol messages
* Writes data to a temporary file
* Verifies integrity and finalizes file

Core Modules:

* protocol.h - message structure
* socket_utils.cpp - send/receive abstraction
* checksum_helper.cpp - SHA-256 computation
* handle_transfer.cpp - core transfer logic

---

## Protocol

Message Types:

| Type  | Description     |
| ----- | --------------- |
| START | Begin transfer  |
| DATA  | File chunk      |
| ACK   | Acknowledgement |
| END   | Final hash      |

Flow:

Sender → START → Receiver
Sender → DATA → Receiver → ACK
(repeat)
Sender → END(hash) → Receiver
Receiver → END ACK → Sender

---

## Requirements

### Linux

```bash
chmod +x setup.sh
./setup.sh
```

Manual installation:

```bash
sudo apt install -y build-essential cmake libssl-dev nlohmann-json3-dev libgtest-dev
```

### Windows (MSYS2 MinGW64)

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-openssl mingw-w64-x86_64-nlohmann-json
```

---

## Build

```bash
cmake -S . -B build
cmake --build build
```

Generated binaries:

* build/sender
* build/receiver
* build/chunkstream_test (if tests are enabled)

---

## Usage

Start receiver:

```bash
./build/receiver
```

Run sender:

```bash
./build/sender <file_path>
```

If no file is provided, the default file from configuration is used.

---

## Configuration

Configuration file:

```bash
cfg/config.json
```

Example:

```json
{
    "ip": "127.0.0.1",
    "port": 8080,
    "file": "data.txt",
    "chunk_size": 65536,
    "thread_count": 4
}
```

---

## Testing

Run all tests:

```bash
ctest --test-dir build --output-on-failure
```

Or run directly:

```bash
./build/chunkstream_test
```

Test coverage includes:

* Basic transfer
* Hash mismatch detection
* Multi-chunk transfer
* Empty file transfer
* Client disconnect handling

---

## Cross-Platform Support

Handled differences:

* POSIX vs Winsock APIs
* close vs closesocket
* ssize_t compatibility
* Windows linking (ws2_32)

---


## Project Structure

```text
cfg/                 Configuration files
docs/                Diagrams (C4, sequence, etc.)
inc/                 Header files
src/                 Source code
tests/               GoogleTest tests
CMakeLists.txt       Build configuration
setup.sh             Linux setup script
```

---

## Diagrams

Architecture diagrams are available in:

```bash
docs/diagrams.drawio
```

## Note
* Compression(zlib, lz4 etc.) is not used as it adds complexity and CPU overhead while giving little benefit for many file types.


---
