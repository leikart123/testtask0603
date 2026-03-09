CC      := gcc
BIN_DIR := bin
OBJ_DIR := build

INC     := -Iinclude
WARN    := -Wall -Wextra -Werror

CFLAGS_COMMON := $(WARN) $(INC)
LDFLAGS_COMMON :=

ifeq ($(DEBUG),1)
  CFLAGS_MODE := -O0 -g3 -DDEBUG
else
  CFLAGS_MODE := -O2 -g
endif

ifeq ($(ASAN),1)
  CFLAGS_MODE  += -fsanitize=address,undefined -fno-omit-frame-pointer
  LDFLAGS_COMMON += -fsanitize=address,undefined
endif

CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_MODE)

LIB_SRC := \
  src/dump.c \
  src/log.c \
  src/buffer_pool.c \
  src/mpmc_queue.c \
  src/file_write.c \
  src/file_read.c \
  src/store_dump.c \
  src/load_dump.c \
  src/dump_scan.c \
  src/tuple_store.c \
  src/join_dump.c \
  src/print_dump.c \
  src/sort_dump.c
  #src/util.c

LIB_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(LIB_SRC))
LIB_DEP := $(LIB_OBJ:.o=.d)

STATGEN_SRC   := tools/statgen.c
STATMERGE_SRC := tools/statmerge.c

STATGEN_OBJ   := $(patsubst %.c,$(OBJ_DIR)/%.o,$(STATGEN_SRC))
STATMERGE_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(STATMERGE_SRC))

TEST_SRC := \
  tests/test_main.c \
  tests/test_common.c \
  tests/test_merge.c \
  tests/test_dump.c

TEST_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_SRC))
TEST_DEP := $(TEST_OBJ:.o=.d)

.PHONY: all clean test dirs

all: dirs $(BIN_DIR)/statgen $(BIN_DIR)/statmerge $(BIN_DIR)/stattest

dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)/src $(OBJ_DIR)/tools $(OBJ_DIR)/tests

$(BIN_DIR)/statgen: $(STATGEN_OBJ) $(LIB_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS_COMMON)

$(BIN_DIR)/statmerge: $(STATMERGE_OBJ) $(LIB_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS_COMMON)

$(BIN_DIR)/stattest: $(TEST_OBJ) $(LIB_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS_COMMON)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

test: $(BIN_DIR)/stattest $(BIN_DIR)/statmerge $(BIN_DIR)/statgen
	$(BIN_DIR)/stattest

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

-include $(LIB_DEP) $(TEST_DEP)