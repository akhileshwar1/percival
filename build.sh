#!/bin/sh
set -e

# Build platform executable.
gcc \
    -g -O0 \
    -Wall -Wextra \
    -fsanitize=address \
    main.cpp \
    -o percival.out \
