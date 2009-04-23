from rest import *

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
    
    # footnotes
    'footnote'  : footnote,

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