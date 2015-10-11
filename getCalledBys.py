import sys
import re

class GetCalledBys:
    
    theClasses = []
    theFunctions = []
    funDict = {}

    def __init__(self):
        self.theArgs = sys.argv[1:]

    # scan the .h files
    def scan01(self):
        for nm in self.theArgs:
            if nm[-4:] == '.cpp':
                continue
            fileHandle = open(nm, 'r')
            lastClass = ''   # the last class name we've seen
            funName = ''
            inClass = False
            line = fileHandle.readline()
            while line:
                if not inClass:
                    # get name of class or struct
                    className = self.getClassName(line)
                    if className:
                        inClass = True
                        lastClass = className
                else:
                    # get name of function
                    funName = self.getFunName(line, lastClass)
                    if not funName:
                        # get closing brace of class/struct
                        if re.match(r'\s+};\s*', line):
                            inClass = False
                line = fileHandle.readline()
            fileHandle.close()
        # print self.funDict

    # get name of class or struct
    def getClassName(self, myLine):
        myLastClass = ''
        myClassName = re.match(r'^\s*(class|struct)\s(\w+)\s{', myLine)
        if myClassName:
            self.theClasses.append(myClassName.group(2))
            myLastClass = myClassName.group(2)
            if myLastClass not in self.funDict:
                self.funDict[myLastClass] = []
        return myLastClass

    # get name of function
    def getFunName(self, myLine, lastClass):
        myThisFun = ''
        myFunName = re.match(r'.*\s.?operator(\W{1,2})\(', myLine)
        if myFunName:
            myThisFun = 'operator' + myFunName.group(1)
        else:
            myFunName = re.match(r'\s{4}(\s{4})?(\w+\s)?(~?\w+\()', myLine)
        if myFunName:
            if not myThisFun:
                myThisFun = myFunName.group(3)[:-1]
            if myThisFun not in self.funDict[lastClass]:
                self.funDict[lastClass].append(myThisFun)
                myThisFun = lastClass + '::' + myThisFun
                if myThisFun not in self.theFunctions:
                    self.theFunctions.append(myThisFun)
        return myThisFun

    # scan the .cpp files
    def scan02(self):
        for nm in self.theArgs:
            if nm[-2:] == '.h':
                continue
            fileHandle = open(nm, 'r')
            awaitName = True
            awaitOpen = False
            awaitClose = False
            className = ''
            callerName = ''
            calleeName = ''
            line = fileHandle.readline()
            while line:
                if awaitName:
                    className, callerName = self.getClassAndCaller(line)
                    if className and callerName:
                        print 'Class is %s, Caller is %s' \
                        % (className, callerName)
                    else:
                        pass
                line = fileHandle.readline()

            fileHandle.close()

    def getClassAndCaller(self, myLine):
        # find an overloaded operator
        classAndCaller = re.match(r'(\w+)?\s(\w+)::operator(\W{1,2})\(', myLine)
        if classAndCaller:
            return classAndCaller.group(2), 'operator' + classAndCaller.group(3)
        classAndCaller = re.match(r'(\w+)?\s(\w+)::(~?)(\w+)\(', myLine)
        if classAndCaller:
            return classAndCaller.group(2), classAndCaller.group(4)
        else:
            return None, None

    def __str__(self):
        ret = ''
        self.theClasses.sort()
        for c in self.theClasses:
            ret += '%s\n' % c
        ret += '\n'
        for f in self.theFunctions:
            ret += '%s\n' % f
        ret = ret[:-1]
        return ret

if __name__ == '__main__':
    getEm = GetCalledBys()
    getEm.scan01()
    print getEm
    getEm.scan02()
