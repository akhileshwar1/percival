#!/bin/sh
set -e

# Build platform executable.
g++ \
    -g -O0 \
    -Wall -Wextra \
    -fsanitize=address \
    -I/usr/include/postgresql \
    main.cpp \
    -lpq \
    -lmicrohttpd \
    -o percival.out \
