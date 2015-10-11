# file: Makefile
# for MemModel
# AAJ

CXX := g++
DEBUGFLAG := -g
STANDARD := -std=c++11

memModel: change.o client.o crashChk.o crc.o driver.o fileMan.o fileShifter.o \
freeList.o inodeTable.o journal.o memMan.o memoryMain.o myMemory.o pageTable.o \
simDisk.o status.o wipeList.o aJUtils.o
	$(CXX) $(DEBUGFLAG) $(STANDARD) -o memModel *.o

aJUtils.o: aJUtils.cpp aJUtils.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c aJUtils.cpp

change.o: change.cpp change.h aJTypes.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c change.cpp

client.o: client.cpp client.h change.h aJTypes.h aJUtils.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c client.cpp

crashChk.o: crashChk.cpp crashChk.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c crashChk.cpp

crc.o: crc.cpp crc.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c crc.cpp

driver.o: driver.cpp myMemory.h pageTable.h memMan.h simDisk.h \
change.h status.h driver.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c driver.cpp

fileMan.o: fileMan.cpp fileMan.h memMan.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c fileMan.cpp

fileShifter.o: fileShifter.cpp fileShifter.h aJUtils.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c fileShifter.cpp

freeList.o: freeList.cpp freeList.h arrBit.h arrBit.cpp
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c freeList.cpp

inodeTable.o: inodeTable.cpp inodeTable.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c inodeTable.cpp

journal.o: journal.cpp crashChk.h journal.h simDisk.h crc.h aJTypes.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c journal.cpp

memMan.o: memMan.cpp memMan.h myMemory.h simDisk.h change.h \
status.h crc.h aJTypes.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c memMan.cpp

memoryMain.o: memoryMain.cpp crashChk.h myMemory.h memMan.h \
simDisk.h change.h status.h driver.h fileMan.h client.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c memoryMain.cpp

myMemory.o: myMemory.cpp myMemory.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c myMemory.cpp

pageTable.o: pageTable.cpp pageTable.h simDisk.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c pageTable.cpp

simDisk.o: simDisk.cpp myMemory.h simDisk.h journal.h \
status.h crc.h inodeTable.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c simDisk.cpp

status.o: status.cpp status.h fileShifter.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c status.cpp

wipeList.o: wipeList.cpp wipeList.h aJTypes.h
	$(CXX) $(DEBUGFLAG) $(STANDARD) -c wipeList.cpp

clean:
	rm *.o

fresh:
	rm *_file output.txt status*.txt
