SRCPATH = src/
INCPATH = include/
OBJPATH = obj/

CXX = g++
CXXFLAGS = -Wall -g -I $(INCPATH)

SRCMODULES = main.cpp socket.cpp chat.cpp
OBJMODULES = $(SRCMODULES:.cpp=.o)

$(addprefix $(OBJPATH),%.o): $(addprefix $(SRCPATH),%.cpp)
	if ! test -d obj; then mkdir obj; fi;
	$(CXX) $(CXXFLAGS) -c $< -o $@

build: $(addprefix $(OBJPATH),$(OBJMODULES))
	$(CXX) $(CXXFLAGS) $^ -o main

run:
	make build
	./main

clean:
	rm -rf $(OBJPATH) main deps.mk

rebuild:
	make clean
	make build

deps.mk: $(addprefix $(SRCPATH),$(SRCMODULES))
	$(CXX) -I $(INCPATH) -MM $^ > $@

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif
