# Project Name
PROJECT := pingpong

# Source, Objects and Binaries folder
SRC_DIR := src
OBJ_DIR := obj

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Compiler
CC := gcc

# Flags
CPPFLAGS := -Iinclude
CFLAGS   := -Wall

.PHONY: all clean build

all: build $(PROJECT)

$(PROJECT): $(OBJ)
	@ $(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
	@ echo '============================ Finished building ============================'

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	@ mkdir -p $@

build:
	@ echo '=============== Started building project using gcc compiler ==============='
	@ $(RM) $(OBJ_DIR)/main.o

clean:
	@ $(RM) -r $(OBJ_DIR) $(PROJECT) $(BIN_DIR)

rebuild: clean all

-include $(OBJ:.o=.d)