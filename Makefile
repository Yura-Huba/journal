CXX ?= g++

CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Werror -O3 -g -fPIC -pthread
CPPFLAGS ?= -Ijournal/include -Iapp -Itests
LDFLAGS  ?= -pthread

BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
LIB_DIR   := $(BUILD_DIR)/lib
BIN_DIR   := $(BUILD_DIR)/bin

LIB_SRCS := $(wildcard journal/src/*.cpp)

TEST_SRCS := \
	tests/test_main.cpp \
	tests/test_journal.cpp \
	tests/test_console_app.cpp

LIB_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(LIB_SRCS))

APP_MAIN_OBJ    := $(OBJ_DIR)/app/main.o
APP_CONSOLE_OBJ := $(OBJ_DIR)/app/ConsoleApplication.o

TEST_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(TEST_SRCS))

LIB   := $(LIB_DIR)/libjournal.so
APP   := $(BIN_DIR)/journal_app
TESTS := $(BIN_DIR)/journal_tests

LOG_FILE  ?= journal.log
LOG_LEVEL ?= INFO

.PHONY: all lib app tests test run clean

all: lib app tests

lib: $(LIB)

app: $(APP)

tests: $(TESTS)

$(LIB): $(LIB_OBJS)
	@mkdir -p $(LIB_DIR)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

$(APP): $(APP_MAIN_OBJ) $(APP_CONSOLE_OBJ) $(LIB)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LDFLAGS) -o $@ $(APP_MAIN_OBJ) $(APP_CONSOLE_OBJ) \
		-L$(LIB_DIR) -ljournal \
		-Wl,-rpath,'$$ORIGIN/../lib'

$(TESTS): $(TEST_OBJS) $(APP_CONSOLE_OBJ) $(LIB)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LDFLAGS) -o $@ $(TEST_OBJS) $(APP_CONSOLE_OBJ) \
		-L$(LIB_DIR) -ljournal \
		-Wl,-rpath,'$$ORIGIN/../lib'

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

test: tests
	cd $(BIN_DIR) && ./journal_tests

run: app
	cd $(BIN_DIR) && ./journal_app $(LOG_FILE) $(LOG_LEVEL)

clean:
	rm -rf $(BUILD_DIR)