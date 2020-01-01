#!/usr/bin/env python

# Convert XaoS cat file to Java properties file

import fileinput

key = None
for line in fileinput.input():
    line = line.strip('\r\n')
    if key is None:
        # we are not in the middle of a multiline value
        strip = line.strip()
        if len(strip) == 0 or strip.startswith('#'):
            # print comments and blank lines verbatim
            print line
            continue
        else:
            # split the line into key and value
            quote = line.find('"')
            key = line[:quote].strip()
            value = line[quote+1:]
    else:
        # line is a continuation of the previous value
        value += line
    # look for terminating quote
    quote = value.rfind('"')
    if quote != -1 and value[quote-1] != '\\':
        # non-escaped quote found
        # look for line-end comment
        pound = value.find('#', quote)
        if pound != -1:
            # print line-end comment before the property
            print value[pound:]

        # print the property key/value pair
        print key + '=' + value[:quote]
        key = None
    else:
        # value continues for another line
        value += '\\n'
