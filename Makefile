CPP = g++
CFLAGS = -g -Wall -shared -fPIC \
	-DHAVE_INTTYPES_H -DHAVE_CONFIG_H -DPURPLE_PLUGINS \
	`pkg-config --cflags purple thrift glib`
LIBS = `pkg-config --libs purple thrift glib`

MAIN = libline.so

SRCS = pluginmain.cpp purplehttpclient.cpp purpleline.cpp \
	thrift_line/line_constants.cpp thrift_line/line_types.cpp thrift_line/Line.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(MAIN)

$(MAIN): thrift_line $(OBJS)
	$(CPP) $(CFLAGS) $(LIBS) -o $(MAIN) $(OBJS)

.cpp.o:
	$(CPP) $(CFLAGS) -std=c++0x -c $< -o $@

thrift_line: line.thrift
	mkdir -p thrift_line
	thrift -out thrift_line --gen cpp line.thrift

clean:
	rm -f $(MAIN)
	rm -f *.o
	rm -rf thrift_line

# TODO: Make proper install target
install:
	mkdir -p ~/.purple/plugins
	cp $(MAIN) ~/.purple/plugins

.PHONY: clean
