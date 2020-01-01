#!/bin/sh
# This scripts decides if there are same names in different directories.
# If yes, we should consider renaming one of the files.
find -name '*.xpf' -printf "%f\n" | sort | uniq -c | grep -v " 1 "
