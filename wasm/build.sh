#!/bin/bash
emcc skill_filter.c \
    -O3 \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS='["_filter_description","_get_keyword_count","_get_keyword","_malloc","_free"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -o ../static/skill_filter.js
