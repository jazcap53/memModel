import re
import sys

def scanFile(fileName):
    ctr = 0
    inComment = False

    for myLine in open(fileName):
        myLine = myLine.strip()
        if not inComment and re.search("/\\*", myLine):
            inComment = True
        if not inComment and not re.match("^\s*//", myLine) \
           and re.match("\S", myLine):
            ctr += 1
            # print myLine
        if inComment and re.search("\\*/", myLine):
            inComment = False
                    
    return ctr

if __name__ == '__main__':
    ttl = 0
    for j in sys.argv[1:]:
        ttl += scanFile(j)
    print "ttl is ", ttl
