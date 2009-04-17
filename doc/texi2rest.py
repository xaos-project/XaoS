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

###############################################################################
# Configuration: these values change the behavior of the conversion

inline = {
    # emphasize
    'emph'      : emphasis,
    'strong'    : strong,

    # font
    'b'         : strong,
    'i'         : emphasis,
    'r'         : ignore,
    'sansserif' : ignore,
    'slanted'   : emphasis,
    'titlefont' : ignore,
    'tt'        : literal,
    'sc'        : ignore,

    # markup
    'code'      : literal,
    'command'   : role('command'),
    'env'       : role('envvar'),
    'file'      : role('file'),
    'option'    : role('option'),
    'samp'      : role('samp'),
    'verb'      : literal,
    'dfn'       : role('dfn'),
    'cite'      : role('title'),
    'key'       : role('kbd'),
    'kbd'       : role('kbd'),
    'var'       : emphasis,
    'acronym'   : abbr('acronymword', 'acronymdesc'),
    'abbrev'    : abbr('abbrevword', 'abbrevdesc'),
    'url'       : literal,

    # math
    'math'      : role('math'),
    'dmn'       : ignore,

    # reference
    'xref'      : ref('xrefnodename', 'xrefprintedname'),
    'inforef'   : ref('inforefnodename', 'inforefrefname'),
    'indexterm' : index,
    'email'     : hyperlink('emailaddress', 'emailname'),
    'uref'      : hyperlink('urefurl', 'urefdesc'),

    # misc
    'logo'      : ignore,
    'punct'     : ignore
}

section = {
    # level 1
    'top'                 : section('*', True),
    'chapter'             : section('*', True),
    'unnumbered'          : section('*', True),
    'appendix'            : section('*', True),

    # level 2
    'section'             : section('='),
    'unnumberedsec'       : section('='),
    'appendixsec'         : section('='),

    # level 3
    'subsection'          : section('-'),
    'unnumberedsubsec'    : section('-'),
    'appendixsubsec'      : section('-'),

    # level 4
    'subsubsection'       : section('^'),
    'unnumberedsubsubsec' : section('^'),
    'appendixsubsubsec'   : section('^')
}
###############################################################################

dom1 = xml.dom.minidom.parse(sys.argv[1])
ditem = handleNode(dom1.getElementsByTagName("texinfo")[0])
ditem.propagate_indents()
(utf8_encode, utf8_decode, utf8_reader, utf8_writer) = codecs.lookup('utf-8')
outf = utf8_writer(sys.stdout)
outf.write(ditem.format(79) + '\n')
