#!/usr/bin/env bash
valgrind --track-origins=yes --leak-check=full --log-file=valgrind.log ./DisServiceLauncher $@
