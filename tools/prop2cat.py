#!/usr/bin/env python

# Convert from java properties file into XaoS catalog file

import fileinput

key = None
for line in fileinput.input():
    line = line.strip('\r\n')
    strip = line.strip()
    if len(strip) == 0 or strip.startswith('#'):
        # print blank lines and comments verbatim
        print line
    else:
        # otherwise this should be a key/value pair
        equal = line.find("=")
        if equal == -1:
            # there shouldn't be any other lines without an equal, but oh well
            print line
        else:
            # this is a property line
            key = line[:equal].strip()
            value = line[equal+1:].replace('\\n', '\n')
            print key + ' "' + value + '"'
