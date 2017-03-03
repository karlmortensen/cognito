PROGRAM=read_alfred

CC?=gcc
CXX?=g++

CFLAGS=-O2
CXXFLAGS=
LDFLAGS=

OBJECTS=$(PROGRAM).o
LIBS=
DEPS=


all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(CXXFLAGS) $^ -o $@ $(LIBS)

debug: CFLAGS += -ggdb -O0 -DDEBUG
debug: $(PROGRAM)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	@rm -rf $(OBJECTS) $(PROGRAM)
