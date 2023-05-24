EXEC=mxIrigUtil

CXX=g++
ifeq ("$(CONFIG_PRODUCT)","DA-682A")
CXXFLAGS=-D__DA682A__
else
ifeq ("$(CONFIG_PRODUCT)","DA-820")
CXXFLAGS=-D__DA820__
endif
endif
CXXFLAGS+= -Wno-write-strings
LDFLAGS = -L../mxirig -lmxirig-$(shell uname -m) -lrt -lm

all: $(EXEC).o
	$(CXX) $(EXEC).o -o $(EXEC) $(LDFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) $? -c

clean:
	rm -rf $(EXEC).o $(EXEC)
