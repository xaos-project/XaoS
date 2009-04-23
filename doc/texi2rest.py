#!/usr/bin/python
"""
texi2rest - Convert texinfo xml represenation to reStructuredText

makeinfo --xml *texifile*
texi2rest *xmlfile* > *restfile*

Copyright (C) 2009  J.B. Langston III
jb-langston@austin.rr.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""

import xml.dom.minidom
import sys
import textwrap
import codecs

import rest
import texinfo

(utf8_encode, utf8_decode, utf8_reader, utf8_writer) = codecs.lookup('utf-8')
outf = utf8_writer(sys.stdout)

dom = xml.dom.minidom.parse('./xaosdev.xml') #sys.argv[1]
nodes = dom.getElementsByTagName('para')

for node in nodes:
    outf.write(rest.parseMarkup(texinfo.inline, node) + '\n\n')
