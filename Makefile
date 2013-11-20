CC = gcc
CPP = g++
CFLAGS = -Wall -shared -fPIC \
	-DHAVE_INTTYPES_H -DHAVE_CONFIG_H -DPURPLE_PLUGINS \
	`pkg-config --cflags purple thrift glib openssl`
LIBS = `pkg-config --libs purple thrift glib openssl`

MAIN = libline.so

CPP_SRCS = purplehttpclient.cpp purpleline.cpp wrappers.cpp \
	thrift_line/line_constants.cpp thrift_line/line_types.cpp thrift_line/Line.cpp
C_SRCS = pluginmain.c

CPP_OBJS = $(CPP_SRCS:.cpp=.o)
C_OBJS = $(C_SRCS:.c=.o)

all: $(MAIN)

$(MAIN): thrift $(CPP_OBJS) $(C_OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o $(MAIN) $(CPP_OBJS) $(C_OBJS)

.cpp.o:
	$(CPP) $(CFLAGS) -std=c++0x -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

thrift: line.thrift
	mkdir -p thrift_line
	thrift -out thrift_line --gen cpp line.thrift

clean:
	rm -f $(MAIN)
	rm -f *.o
	rm -rf thrift

# TODO: Make proper install target
install:
	mkdir -p ~/.purple/plugins
	cp $(MAIN) ~/.purple/plugins

.PHONY: clean
