#!/bin/sh
set -e

export PG_HOST="127.0.0.1"
export PG_PORT=5432
export PG_USER="akhil"
export PG_PWD="akhileshwar"
export PG_CONN="dbname=percival"

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
