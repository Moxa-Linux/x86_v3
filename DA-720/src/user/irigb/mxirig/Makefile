CXX=g++
#CXXFLAGS=-isystem../mxIrigUtil/
MACHINE=`uname -m`

all:
	# For x86_64 machine, we assume the host is x86_64 machine to build the library
	$(CXX) $(CXXFLAGS) -c mxirig.cpp
	$(AR) crv libmxirig-$(MACHINE).a mxirig.o

	# For i686 machine, we use -mi686 CXXFLAGS to build the library
	$(CXX) $(CXXFLAGS) -m32 -c mxirig.cpp -o mxirigi686.o
	$(AR) crv libmxirig-i686.a mxirigi686.o

clean:
	#rm -rf *.o
