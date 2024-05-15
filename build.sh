#!/bin/bash

cc $(find src -name '*.c') -std=c99 -Isrc -okoshka -g

