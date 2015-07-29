import re
import sys

def scanFile(fileName, lookFor):
    ctr = 0
    inComment = False

    for myLine in open(fileName):
        myLine = myLine.strip()
        if not inComment and re.search("/\\*", myLine):
            inComment = True
        if not inComment and not re.match("^\s*//", myLine) \
           and re.match("\S", myLine) and lookFor in myLine:
            ctr += 1
            # print myLine
        if inComment and re.search("\\*/", myLine):
            inComment = False
                    
    return ctr

if __name__ == '__main__':
    ttl = 0
    lookFor = sys.argv[1]
    for j in sys.argv[2:]:
        ttl += scanFile(j, lookFor)
    print "ttl is ", ttl
