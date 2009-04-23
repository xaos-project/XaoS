def emphasis(node):
    return '*' + extractText(node) + '*'
    
def strong(node):
    return '**' + extractText(node) + '**'
    
def literal(node):
    return '``' + extractText(node) + '``'

def role(name):
    return lambda node: ':' + name + ':`' + extractText(node) + '`'
    
def abbr(abbrev, description):
    def parse(parent):
        string = ''
        for node in parent.childNodes:
            if node.nodeName == abbrev:
                string = extractText(node) + string
            elif node.nodeName == description:
                string = string + ' (' + extractText(node) + ')'
        string = ':abbr:`' + string + '`'
        return string
    return parse

def ref(label, title):
    def parse(parent):
        string = ''
        for node in parent.childNodes:
            if node.nodeName == label:
                string = extractText(node) + string
            elif node.nodeName == title:
                string = string + ' <' + extractText(node) + '>'
        string = ':ref:`' + string + '`'
        return string
    return parse
    
def hyperlink(address, title):
    def parse(parent):
        string = ''
        for node in parent.childNodes:
            if node.nodeName == title:
                string = extractText(node) + string
            elif node.nodeName == address:
                string = string + ' <' + extractText(node) + '>'
        string = '`' + string + '`_'
        return string
    return parse
    
def index(node):
    return '.. index:: ' + extractText(node) + '\n'

def ignore(node):
    return extractText(node)

def delete(node):
    return ''
    
def footnote(node):
    return '[#]_'
    
def section(decoration, overline=False):
    def parse(parent):
        string = ''
        for node in parent.childNodes:
            if node.nodeName == 'title':
                title = parseInlineMarkup(node)
                string = title + '\n' + decoration * len(title) + '\n\n' + string
                if overline:
                    string = decoration * len(title) + '\n' + string
            else:
                string += parseMarkup(node) + '\n\n'
        return string
    return parse
        
def extractText(parent):
    'extract text from node ignoring any markup'
    string = ''
    for node in parent.childNodes:
        if node.nodeType == node.TEXT_NODE:
            string += node.data
        else:
            string += extractText(node)
    return string

def parseMarkup(markup, parent):
    '''parse node and convert markup'''
    string = ''
    for node in parent.childNodes:
        if node.nodeType == node.TEXT_NODE:
            string += node.data
        else:
            string += markup[node.nodeName](node)
    return string
