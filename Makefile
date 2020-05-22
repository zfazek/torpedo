ifneq (,)
This makefile requires GNU Make.
endif

PROGRAM = torpedo

CPP_FILES := $(shell find . -not -path "*/build/*" -a -name "*.cpp")
OBJS := $(patsubst %.cpp, %.o, $(CPP_FILES))
CXXFLAGS = -std=c++14 -Wall -Wextra -g -O0
CPPFLAGS =
LDFLAGS =
LDLIBS =

CPPFLAGS += -I/home/zfazek/git/seasocks/src/main/c
LDLIBS += /home/zfazek/git/seasocks/build/src/main/c/libseasocks.a

CXXFLAGS += $(shell pkg-config --cflags zlib)
LDLIBS += $(shell pkg-config --libs zlib)

$(PROGRAM): .depend $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(PROGRAM) $(LDLIBS)

depend: .depend

.depend: cmd = $(CXX) $(CPPFLAGS) -MM -MF depend $(var); cat depend >> .depend;
.depend:
	@echo "Generating dependencies..."
	@$(foreach var, $(CPP_FILES), $(cmd))
	@rm -f depend

-include .depend

%.o: %.cpp Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f .depend $(OBJS)

.PHONY: clean depend
