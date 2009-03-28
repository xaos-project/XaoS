#!/usr/bin/python
"""
NAME
====

xhtml2rest - Convert xhtml to reStructuredText

SYNOPSIS
========

xhtml2rest *xhtmlfile* > *restfile*

DESCRIPTION
===========

``xhtml2rest``, which, far from being a decent and complete program, is
only something to begin with, hopefully processes the given UTF-8
xhtml file and produces reStructuredText "source code" in the standard
output.  If your input is html and/or not in UTF-8, you can convert it
to UTF-8 xhtml using ``iconv`` and ``tidy``:

    iconv -f *source_encoding* -t utf-8 *source_html* > *html_utf8*

    tidy -utf8 -asxml -o *xhtmlfile* *html_utf8*

    xhtml2rest *xhtmlfile* > *restfile*

Interestingly, since reStructuredText is not simple markup, but has
very strict rules with the intention that the source is perfectly
readable, it turns out that converting html to reStructuredText is
actually *rendering*. ``xhtml2rest`` is a small rendering engine. Since
I had no time to study how existing rendering engines work, I had to
reinvent the wheel. So although the code is clean (I actually wrote it
twice), I doubt that the core logic is adequate for future extensions.
But it's better than nothing. There is some documentation in the code,
but feel free to email me if you need more explanations.

LIMITATIONS
===========

I created ``xhtml2rest`` for a very specific job. It does that job
correctly, but for your web page it might not work. It should not be
very hard, however, either to improve the code, or to determine what
it is in your web page that confuses ``xhtml2rest`` and remove it.

Other than that, there are the following limitations:

* No indented tables

* No multi-col or -row spans in tables

* No support for \<br>

* Not tested in nested tables (check http://www.w3m.org/story.html)

* \<th> support is quick and dirty

* If the same anchor text is met twice, the anchor is ignored

* No indented \<pre> elements (but I'm not sure the HTML standard
  allows them)

* Images are ignored

* The word HARDWIRED in the code indicates a hardwired hack which is
  specific to the job I wanted ``xhtml2rest`` to do.

META
====

``xhtml2rest`` was created by Antonios Christofides,
anthony@itia.ntua.gr, May-June 2005.

Revision: $Revision: 3753 $

The code and this text is hereby placed in the public domain.
"""

import xml.dom.minidom
import re
import sys
import textwrap
import math
import UserList
import warnings
import codecs

###############################################################################
# Global variables. I know. I'm terribly sorry. Please get rid of them.

# 'unindent' is used by list items. A li list item is always indented, but its
# first line is "unindented" and contains the number or bullet. However, it was
# difficult for the li node to tell its #text contents (which may be deeply
# nested) to use that.  So it just places the number or bullet, which must be 4
# characters, like " 1. ", in "unindent". The first text to be rendered uses
# the unindent and then sets it to empty again.

unindent = ''
hyperlinks = {} # text-target pairs found in "a href" elements
###############################################################################

class Ditem:
    """A document item; usually a node, but can be a block of text
    resulting from processing adjacent inline items. If it is a node,
    it is usually the BlockDitem subclass; if it is text, it is
    normally a plain Ditem."""
    def __init__(self, text):
        self.text = text    # Contained text (empty for BlockDitem)
        self.type = ''      # tag for block node, empty for inline
        self.indentlevel = 0  # 0 - unindented; 1 - indented; etc.
    def __repr__(self):
        return self.__class__.__name__+'("""'+self.text+'""")'
    def propagate_indents(self):
        "Propagates indent level recursively to children"
        pass
    def maxwidth(self):
        "Width it will occupy if allowed to render on infinite width"
        self.remove_white_space()
        return len(self.text) + 4*self.indentlevel
    def minwidth(self):
        "Width it will occupy if wrapped as much as possible"
        wordlens = [len(x) for x in self.text.split()]
        if wordlens: return max(wordlens) + 4*self.indentlevel
        else: return 0
    def format(self, width):
        """Returns contents formatted so as not to exceed specified
        width, if possible"""
        global unindent
        if(self.type=='pre'): raise Exception, "What are we doing here?"
        self.remove_white_space()
        # Quick hack to fix a problem. Do we begin with '* '?
        while len(self.text)>=2 and self.text[1]==' ' and self.text[0] in '*-':
            # It may be mistaken for a bullet list. Strip it.
            self.text = self.text[2:]
        if width < self.minwidth(): width = self.minwidth()
        # The textwrap module has the nasty habit of breaking at hyphens. So
        # we'll do a nasty hack: find a character that does not exist in the
        # text, replace all hyphens with that character, ok, you get the point.
        hyphensurrogate = ''
        for c in '!@#$%^&*~':
            if self.text.find(c)<0:
                hyphensurrogate = c
                break
        if not hyphensurrogate: raise Exception, "Houston we have a problem"
        text = self.text.replace('-', hyphensurrogate)
        wrapper = textwrap.TextWrapper(
            initial_indent=((4*self.indentlevel)-len(unindent))*' '+unindent,
            subsequent_indent=4*self.indentlevel*' ',
            width=width, break_long_words = False)
        unindent = ''
        text = wrapper.fill(text)
        text = text.replace(hyphensurrogate, '-')
        return text
    def empty(self):
        "Returns true if contains nothing"
        return not self.text
    def remove_white_space(self):
        "Removes extra white space"
        self.text = re.sub('\s+', ' ', self.text).strip()
    def canmerge(self):
        "Tells whether it's possible to merge this Ditem with adjacent ones"
        return True
    def merge(self, aditem):
        """If possible, merges aditem, which should be an adjacent Ditem that
        comes after this one."""
        if not self.canmerge() or not aditem.canmerge(): return False
        if len(self.text)>0 and self.text[-1] == '_' and len(aditem.text)>0 \
            and aditem.text[0] not in """ \n\t:.,!=/|;"'?<>[]{}()""":
            # Leave space after link if not followed by punctuation
            self.text = self.text + ' ' + aditem.text
        else:
            self.text = self.text + aditem.text
        return True

class BlockDitem(Ditem):
    "A Ditem which contains other Ditems"
    def __init__(self, type):
        Ditem.__init__(self, '')
        self.type = type
        self.children = []  # Contained Ditems
    def __repr__(self):
        return self.__class__.__name__+'("'+self.type+'"); children = '+repr(self.children)
    def maxwidth(self):
        childmaxwidths = [x.maxwidth() for x in self.children]
        return childmaxwidths and max(childmaxwidths) or 0
    def minwidth(self):
        childminwidths = [x.minwidth() for x in self.children]
        return childminwidths and max(childminwidths) or 0
    def propagate_indents(self):
        for x in self.children:
            x.indentlevel = self.indentlevel
            x.propagate_indents()
    def format(self, width):
        if width < self.minwidth(): width = self.minwidth()
        results = [x.format(width) for x in self.children]
        results = [x for x in results if x]
        return "\n\n".join(results)
    def empty(self):
        return not (self.children)
    def canmerge(self):
        return False

class PreDitem(Ditem):
    "A Ditem representing a literal block"
    def maxwidth(self):
        return max([len(x) for x in self.text.split('\n')])
    def minwidth(self):
        return self.maxwidth() # Literal block; width's given
    def remove_white_space(self):
        pass
    def format(self, width):
        result = '::\n\n'
        for x in self.text.split('\n'):
            result = result + '    ' + x + '\n'
        result = result + '..\n\n'
        return result
    def canmerge(self):
        return False

class HeadingDitem(BlockDitem):
    "A Ditem representing an h1, h2, ..., h9"
    def __init__(self, type):
        BlockDitem.__init__(self, type)
    def minwidth(self):
        return self.maxwidth()  # Headings don't wrap
    def format(self, width):
        assert(len(self.children)==1)
        text = self.children[0].format(32767)
        level = eval(self.type[1])
        underliner = "=-`'.~*+^"[level-1]
        return text + '\n' + len(text)*underliner

class BlockQuoteDitem(BlockDitem):
    "A Ditem representing a blockquote"
    def __init__(self, type):
        BlockDitem.__init__(self, type)
    def propagate_indents(self):
        self.indentlevel = self.indentlevel + 1
        BlockDitem.propagate_indents(self)

class ListDitem(BlockDitem):
    "A Ditem representing an ol, ul, or dl"
    def __init__(self, type):
        BlockDitem.__init__(self, type)
    def format(self, width):
        # First pass the list type and order to the children
        order = 1
        for x in self.children:
            if isinstance(x, ListItemDitem):
                x.listtype = self.type
                x.order = order
                order = order+1
        # And then process normally
        return BlockDitem.format(self, width)
        
class ListItemDitem(BlockDitem):
    "A Ditem representing a li, dt, or dd"
    def __init__(self, type):
        BlockDitem.__init__(self, type)
        self.listtype = None
        self.order = 0
    def minwidth(self):
        if self.type == 'dt': return self.maxwidth()  # Don't wrap dt
        else: return BlockDitem.minwidth(self)
    def propagate_indents(self):
        if self.type in ('li', 'ol', 'dd'):
            self.indentlevel = self.indentlevel + 1
        BlockDitem.propagate_indents(self)
    def format(self, width):
        global unindent
        if self.type == 'li' and self.listtype == 'ol':
            unindent = ('%d. ' % (self.order)).ljust(4)
        elif self.type == 'li' and self.listtype == 'ul':
            unindent = '*   '
        return BlockDitem.format(self, width)

class RenderedColumn:
    "Width information about a column being rendered"
    def __init__(self, minwidth, maxwidth):
        self.minwidth = minwidth
        self.maxwidth = maxwidth
        self.curwidth = maxwidth
        self.fixedwidth = 0
    def logwidth(self):
        if self.maxwidth==0: return 0
        else: return math.log(self.maxwidth)
    def update(self, minwidth, maxwidth):
        "Replaces minwidth/maxwidth if greater"
        self.minwidth = minwidth>self.minwidth and minwidth or self.minwidth
        self.maxwidth = maxwidth>self.maxwidth and maxwidth or self.maxwidth
        self.curwidth = self.maxwidth

class RenderedColumns(UserList.UserList):
    "A list of RenderedColumn"
    def __init__(self, alist):
        self.data = alist
    def totalWidth(self):
        "Returns total table width"
        return reduce(lambda x,y: x+y, [z.curwidth for z in self.data]) \
            + len(self.data) + 1
    def sumLogWidth(self):
        "Returns sum of logwidth for nonfixed columns"
        return reduce(lambda x,y: x+y,
            [x.logwidth()*(1-x.fixedwidth) for x in self.data])
    def distributeWidthDifference(self, width):
        "Step 4 of w3m table rendering algorithm"
        # Note: The use of math.ceil below is because I'd rather have a
        # suboptimal width (a few characters less than requested width) rather
        # than go find what to do with rounding.
        w = self.totalWidth() - width
        assert(w>0)
        repeat_distribution = 1
        while repeat_distribution:
            repeat_distribution = 0
            for x in self.data:
                if x.fixedwidth: continue
                if x.curwidth - math.ceil(w*x.logwidth()/self.sumLogWidth()) < \
                    x.minwidth:
                        x.curwidth = x.minwidth
                        x.fixedwidth = 1
                        w = self.totalWidth() - width
                        repeat_distribution=1
                        break
        # Now that the we finished finding which columns need to be fixed to
        # their minimum width, perform the distribution once again, without
        # checking, and actually change remaining column widths
        for x in self.data:
            if x.fixedwidth: continue
            x.curwidth = x.curwidth - math.ceil(w*x.logwidth()/self.sumLogWidth())
    
def tablehrule(colwidths, rule='-'):
    "Returns a horizontal table separator for given column widths"
    result = '+'
    for x in colwidths:
        result = result + rule * x + '+'
    return result

class TableDitem(BlockDitem):
    def __init__(self, type):
        BlockDitem.__init__(self, type)
    def format(self, width):
        # Uses table rendering algorithm of w3m
        # (http://www.w3m.org/story.html), but ignoring width attribute
        # Step 1 
        columns = RenderedColumns([RenderedColumn(x.minwidth(),
            max(x.maxwidth(), 1)    # A column can't be smaller than 1 character
            ) for x in self.children[0].children])
        for x in self.children:
            for i in range(len(columns)):
                if (len(x.children)<=i): continue # Skip empty columns
                columns[i].update(x.children[i].minwidth(), x.children[i].maxwidth())
        # Step 2 (width attribute) ignored
        # Step 3 (already done - list was created with maxwidth)
        # Step 4
        if columns.totalWidth() > width: columns.distributeWidthDifference(width)
        # OK, column widths are now calculated
        colwidths = [int(x.curwidth) for x in columns]
        result = tablehrule(colwidths) + '\n'
        usedheadbodysep = False
        for tr in self.children:
            result = result + tr.format(colwidths)
            rule = '-'
            if not usedheadbodysep and tr.children[0].type == 'th' \
                                        and tr!=self.children[-1]:
                rule = '='
                usedheadbodysep = True
            result = result + tablehrule(colwidths, rule) + '\n'
        return result

class TrDitem(BlockDitem):
    def __init__(self, type):
        BlockDitem.__init__(self, type)
    def maxwidth(self):
        return reduce(lambda x,y: x+y,
            [x.maxwidth() for x in self.children]) + len(self.children) + 1
    def minwidth(self):
        return reduce(lambda x,y: x+y,
            [x.minwidth() for x in self.children]) + len(self.children) + 1
    def format(self, colwidths):
        columns = []       # List of lists of lines
        maxlinecount = 0   # Num of lines in vertically largest column
        for i in range(len(colwidths)):
            if len(self.children)<=i: lines = [ '' ]
            else: lines = self.children[i].format(colwidths[i]).split('\n')
            lines = [x + ' ' * (colwidths[i]-len(x)) for x in lines] # Pad to col len
            maxlinecount = max(maxlinecount, len(lines))
            columns.append(lines)
        # Pad vertically
        for i in range(len(columns)):
            for j in range(maxlinecount-len(columns[i])):
                columns[i].append(' ' * colwidths[i])
        result = '' 
        # Add vertical separators
        for i in range(maxlinecount):
            result = result + '|'
            for j in range(len(columns)):
                result = result + columns[j][i] + '|'
            result = result + '\n'
        return result

def handleNodeList(nodelist):
    "Processes given nodes; merges them if possible; returns ditem list"
    ditems = []
    curditem = Ditem('')
    for node in nodelist:
        aditem = handleNode(node)
        if curditem.merge(aditem): continue
        ditems.append(curditem)
        curditem = aditem
    if not curditem.empty(): ditems.append(curditem)
    return ditems

def handleNode(node):
    if node.nodeType == node.TEXT_NODE:
        return handleText(node)
    elif node.nodeName=='a':
        return handleAnchor(node)
    elif re.match('h\d', node.nodeName):
        return handleHeading(node)
    elif node.nodeName=='div' and node.getAttribute('class')=='cit':  # HARDWIRED
        return handleBlockQuote(node)
    elif node.nodeName in ('body', 'div', 'p', 'td', 'th'):
        return handleGenericBlock(node)
    elif node.nodeName in ('em', 'i'):
        return handleEmphasis(node)
    elif node.nodeName in ('strong', 'b'):
        return handleStrong(node)
    elif node.nodeName in ('ol', 'ul', 'dl'):
        return handleList(node)
    elif node.nodeName in ('li', 'dd', 'dt'):
        return handleListItem(node)
    elif node.nodeName in ('table'):
        return handleTable(node)
    elif node.nodeName in ('tr'):
        return handleTr(node)
    elif node.nodeName in ('pre'):
        return handlePre(node)
    elif node.hasChildNodes():
        contents = handleNodeList(node.childNodes)
        if len(contents) == 1: return contents[0]
        if len(contents) == 0: return Ditem('')
        result = BlockDitem(node.nodeName)
        result.children = contents
        return result
    return Ditem('')

def processChildren(node):
    if node.hasChildNodes():
        return handleNodeList(node.childNodes)
    else:
        return ()

def mergeChildren(node):
    contents = processChildren(node)
    if len(contents)>1: raise Exception('Unexpected block elements')
    if contents: return contents[0]
    else: return Ditem('')

def handleText(node):
    return Ditem(node.data)

def handleAnchor(node):
    result = mergeChildren(node)
    result.type = node.nodeName
    result.text = result.text.strip()
    if result.text == '': return result
    target = node.getAttribute('href').strip()
    if target=="" or target[0]=='#': return result  # Ignore intrnl links
    result.text = re.sub('\s+', ' ', result.text)
    key = result.text.lower()
    if hyperlinks.has_key(key) and hyperlinks[key]!=target:
        # The following try-except is a quick hack to ensure that the
        # program will not stop because of problems in the warning
        # mechanism. One such specific problem is a UnicodeEncodeError 
        # when result.text contains difficult characters.
        try:
            warnings.warn("Ignoring second appearance of anchor '" + result.text +
                                                    "' with different target")
        except:
            pass
        return result
    hyperlinks[key] = target
    result.text = '`'+result.text+'`_'
    return result

def handleHeading(node):
    contents = mergeChildren(node)
    if contents.empty(): return contents
    result = HeadingDitem(node.nodeName)
    result.children.append(contents)
    return result

def handleEmphasis(node):
    result = mergeChildren(node)
    result.type = node.nodeName
    if result.text:
        result.text = '*' + result.text + '*'
    return result

def handleStrong(node):
    result = mergeChildren(node)
    result.type = node.nodeName
    if result.text:
        result.text = '**' + result.text + '**'
    return result

def handleGenericBlock(node):
    result = BlockDitem(node.nodeName)
    result.children = processChildren(node)
    return result

def handleBlockQuote(node):
    result = BlockQuoteDitem(node.nodeName)
    result.children = processChildren(node)
    return result

def handleList(node):
    result = ListDitem(node.nodeName)
    result.children = processChildren(node)
    return result

def handleListItem(node):
    result = ListItemDitem(node.nodeName)
    result.children = processChildren(node)
    return result

def handleTable(node):
    result = TableDitem(node.nodeName)
    # Ignore table contents that are not tr
    result.children = [x
        for x in processChildren(node) if x.type=='tr']
    return result

def handleTr(node):
    result = TrDitem(node.nodeName)
    # Ignore tr contents that are not th or td
    result.children = [x
        for x in processChildren(node) if x.type in ('th', 'td')]
    return result

def handlePre(node):
    return PreDitem(mergeChildren(node).text)

dom1 = xml.dom.minidom.parse(sys.argv[1])
ditem = handleNode(dom1.getElementsByTagName("body")[0])
ditem.propagate_indents()
(utf8_encode, utf8_decode, utf8_reader, utf8_writer) = codecs.lookup('utf-8')
outf = utf8_writer(sys.stdout)
outf.write(ditem.format(79) + '\n')
for h in hyperlinks.keys():
    outf.write('\n.. _`' + h + '`:\n    ' + hyperlinks[h] + '\n')
