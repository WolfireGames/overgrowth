# Tutoral GCC Makefile

CXX = g++
CXXFLAGS = -ggdb -I../../../../angelscript/include -D_LINUX_
SRCDIR = ../../source
OBJDIR = obj

SRCNAMES =   main.cpp

OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRCNAMES:.cpp=.o))) obj/scriptstdstring.o obj/contextmgr.o
BIN = ../../bin/concurrent
DELETER = rm -f

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) -o $(BIN) $(OBJ) -langelscript  -L../../../../angelscript/lib -lpthread
	@echo -------------------------------------------------------------------
	@echo Done.

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptstdstring.o: ../../../../add_on/scriptstdstring/scriptstdstring.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/contextmgr.o: ../../../../add_on/contextmgr/contextmgr.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	$(DELETER) $(OBJ) $(BIN)

.PHONY: all clean
