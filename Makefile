PGCMP_HOME    := $(shell pwd)
INST_DIR       := /usr/local/bin
INCLUDE        := -I$(PGCMP_HOME)/include -I$(shell pg_config --includedir) -I$(shell pg_config --includedir-server)

CC      := gcc
CFLAGS  :=  $(shell pg_config --cflags) $(INCLUDE)
LD      := gcc
LDFLAGS :=  $(shell pg_config --ldflags) 
LIBS    := -lrt -L$(shell pg_config --libdir) -lpq -lm -lpthread 

SRC := $(patsubst %.c,$(PGCMP_HOME)/%.c,$(wildcard src/*.c)) 
OBJ := $(patsubst %.c,%.o,$(wildcard $(PGCMP_HOME)/src/*.c))

TARGET      := $(PGCMP_HOME)/bin/pgcmp
TARGET_DEPS := $(PGCMP_HOME)/Makefile $(wildcard $(PGCMP_HOME)/include/*.h)

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(PGCMP_HOME)/bin
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJ) $(LIBS) 

$(OBJ):  $(TARGET_DEPS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

install:
	cp $(TARGET) $(INST_DIR)

clean:
	rm -f  $(PGCMP_HOME)/src/*.o  $(TARGET) core core.* 
