#!/bin/bash
export DEBUGINFOD_URLS=""
exec /usr/bin/gdb "$@"
