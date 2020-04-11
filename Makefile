CC = g++
CFLAGS = -lstdc++fs -std=c++17 -Wall
LFLAGS = -lstdc++fs
OBJS = src/net2mat.o src/endian.o src/io.o src/mat4.o src/pugixml.o src/read_data.o src/mat.o

net2mat : net2mat.o endian.o io.o mat4.o \
       pugixml.o read_data.o mat.o
	$(CC) $(LFLAGS) -o net2mat net2mat.o endian.o io.o mat4.o \
               pugixml.o read_data.o mat.o

net2mat.o : src/net2mat.cpp # srt/pugixml.hpp src/matio.h
	$(CC) $(CFLAGS) -c src/net2mat.cpp
endian.o : src/endian.c # src/matio_private.h
	$(CC) $(CFLAGS) -c src/endian.c
io.o : src/io.c # src/matio_private.h
	$(CC) $(CFLAGS) -c src/io.c
mat.o : src/mat.c # src/safe_math.h
	$(CC) $(CFLAGS) -c src/mat.c
mat4.o : src/mat4.c # src/mat4.h src/matio_private.h
	$(CC) $(CFLAGS) -c src/mat4.c
read_data.o : src/read_data.c # src/matio_private.h
	$(CC) $(CFLAGS) -c src/read_data.c
pugixml.o : src/pugixml.cpp # srt/pugixml.hpp
	$(CC) $(CFLAGS) -c src/pugixml.cpp
clean :
	rm net2mat net2mat.o endian.o io.o mat4.o \
               pugixml.o read_data.o mat.o