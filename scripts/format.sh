#!/bin/bash

git ls-files '*.cpp' '*.hpp' '*.c' '*.h' | xargs clang-format --style file -i