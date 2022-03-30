# Test Framework GNUC makefile

CXX = g++
CXXFLAGS += -ggdb -I../../../../angelscript/include -Wno-missing-field-initializers
SRCDIR = ../../source
OBJDIR = obj

ifdef DONT_WAIT
    CXXFLAGS += -DDONT_WAIT
endif


SRCNAMES = \
  main.cpp \
  gamemgr.cpp \
  gameobj.cpp \
  scriptmgr.cpp \


OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRCNAMES:.cpp=.o))) \
  obj/scripthandle.o \
  obj/scriptstdstring.o \
  obj/scriptbuilder.o \
  obj/weakref.o \
  

BIN = ../../bin/game
DELETER = rm -f

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) -o $(BIN) $(OBJ) -langelscript -lpthread -L../../../../angelscript/lib
	@echo -------------------------------------------------------------------
	@echo Done.

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scripthandle.o: ../../../../add_on/scripthandle/scripthandle.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<
	
obj/scriptstdstring.o: ../../../../add_on/scriptstdstring/scriptstdstring.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptbuilder.o: ../../../../add_on/scriptbuilder/scriptbuilder.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/weakref.o: ../../../../add_on/weakref/weakref.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	$(DELETER) $(OBJ) $(BIN)

.PHONY: all clean
