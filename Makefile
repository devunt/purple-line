CXX = g++
CXXFLAGS = -g -Wall -shared -fPIC \
	-DHAVE_INTTYPES_H -DHAVE_CONFIG_H -DPURPLE_PLUGINS \
	`pkg-config --cflags purple thrift`
LIBS = `pkg-config --libs purple thrift`

MAIN = libline.so

GEN_SRCS = thrift_line/line_constants.cpp thrift_line/line_types.cpp thrift_line/TalkService.cpp
REAL_SRCS = pluginmain.cpp linehttptransport.cpp thriftclient.cpp \
	purpleline.cpp purpleline_blist.cpp
SRCS += $(GEN_SRCS)
SRCS += $(REAL_SRCS)

OBJS = $(SRCS:.cpp=.o)

all: $(MAIN)

$(MAIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -Wl,-z,defs -o $(MAIN) $(OBJS) $(LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -std=c++11 -c $< -o $@

thrift_line: line.thrift
	mkdir -p thrift_line
	thrift --gen cpp -out thrift_line line.thrift

clean:
	rm -f $(MAIN)
	rm -f *.o
	rm -rf thrift_line

# TODO: Make proper install target
install:
	mkdir -p ~/.purple/plugins
	cp $(MAIN) ~/.purple/plugins

depend: .depend

.depend: thrift_line $(REAL_SRCS)
	rm -f .depend
	$(CXX) $(CXXFLAGS) -MM $(REAL_SRCS) >>.depend

-include .depend

.PHONY: clean
