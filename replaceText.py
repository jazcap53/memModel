# usage: python replaceText.py filenames... <original text> <replacement text>
# replaces 'original text' with 'replacement text' wherever it occurs in 
#     a file
# copies filename into filename.tmp, deletes filename, and renames filename.tmp to
#     filename

# *** this version untested ***

import sys
import re
import os

clas = sys.argv[1:]

repl = clas.pop()
orig = clas.pop()

print "the original is", orig
print "the replacement is", repl

ct = 0

for a in clas:
    b = a + '.tmp'
    myInFile = open(a, 'r')
    myOutFile = open(b, 'w')
    myLine = myInFile.readline()
    while myLine:
        myNewLine = re.sub(orig, repl, myLine)
        if myNewLine != myLine:
            ct += 1
        myLine = myNewLine
        myOutFile.write(myLine)
        myLine = myInFile.readline()
    myOutFile.close()
    myInFile.close()
    os.remove(a)
    os.rename(b, a)

print ct, "lines altered"
