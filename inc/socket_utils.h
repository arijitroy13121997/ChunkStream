#pragma once
#include <cstddef>
#include <sys/types.h>

ssize_t send_all(int sock, const void* data, size_t len);
ssize_t recv_all(int sock, void* buffer, size_t len);