MAINSOURCE := main.cpp 
# MAINOBJS := $(patsubst %.cpp,%.o,$(MAINSOURCE))
SOURCE  := $(wildcard *.cpp )
override SOURCE := $(filter-out $(MAINSOURCE),$(SOURCE))
OBJS    := $(patsubst %.cpp,%.o,$(SOURCE))



TARGET  := webserver   
CC      := g++
LIBS    := -lpthread
INCLUDE:= -I./usr/local/lib
CFLAGS  := -std=c++11 -g -Wall -O3 -D_PTHREADS
CXXFLAGS:= $(CFLAGS)




.PHONY : objs clean veryclean rebuild all tests debug
all : $(TARGET) 
objs : $(OBJS)
rebuild: veryclean all

test:client.o
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

client.o:client/client.cpp
	$(CC) -c -g $^ 

clean :
	find . -name '*.o' | xargs rm -f
veryclean :
	find . -name '*.o' | xargs rm -f
	find . -name $(TARGET) | xargs rm -f
debug:
	@echo $(SOURCE)

$(TARGET) : $(OBJS) main.o
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
