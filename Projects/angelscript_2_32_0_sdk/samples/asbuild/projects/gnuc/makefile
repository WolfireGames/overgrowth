# GCC Makefile
CXX = g++
CXXFLAGS = -g -I../../../../angelscript/include -D_LINUX_
SRCDIR = ../../source
OBJDIR = obj

SRCNAMES = \
  main.cpp

OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRCNAMES:.cpp=.o))) obj/asbuild.o
BIN = ../../bin/asbuild
DELETER = rm -f

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) -o $(BIN) $(OBJ) -langelscript -lpthread -L ../../../../angelscript/lib
	@echo -------------------------------------------------------------------
	@echo Done.

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/asbuild.o: ../../../../add_on/scriptbuilder/scriptbuilder.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	$(DELETER) $(OBJ) $(BIN)

.PHONY: all clean
