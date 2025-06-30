# Cross-Platform Test Makefile

# 检测操作系统
ifeq ($(OS),Windows_NT)
    # Windows 环境
    PLATFORM = windows
    TARGET_EXT = .exe
    LDFLAGS = -lws2_32
    RM_CMD = if exist "$(1)" del "$(1)"
else
    # Linux/Unix 环境
    PLATFORM = linux
    TARGET_EXT = 
    LDFLAGS = 
    RM_CMD = rm -f $(1)
endif

# 编译器和选项
CC = gcc
CFLAGS = -Wall -O2 -I../src -fcommon

# 源文件
TEST_SOURCES = test_crossplatform.c
BENCHMARK_SOURCES = benchmark.c

# 目标文件
TARGET = test_crossplatform$(TARGET_EXT)
BENCHMARK_TARGET = benchmark$(TARGET_EXT)

# 默认目标
all: $(TARGET) $(BENCHMARK_TARGET)

# 编译测试程序
$(TARGET): $(TEST_SOURCES)
	@echo "Building cross-platform test for $(PLATFORM)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Test build complete: $@"

# 编译基准测试程序
$(BENCHMARK_TARGET): $(BENCHMARK_SOURCES)
	@echo "Building benchmark test for $(PLATFORM)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Benchmark build complete: $@"

# 运行测试
test: $(TARGET)
	@echo "Running cross-platform tests..."
	@./$(TARGET)

# 运行基准测试
benchmark: $(BENCHMARK_TARGET)
	@echo "Running performance benchmark..."
	@echo "Note: Make sure DNS relay server is running on 127.0.0.1:53"
	@echo "Starting benchmark in 3 seconds..."
ifeq ($(PLATFORM),windows)
	@timeout /t 3 /nobreak >nul
else
	@sleep 3
endif
	@./$(BENCHMARK_TARGET)

# 清理
clean:
	@echo "Cleaning test files..."
	@$(call RM_CMD,$(TARGET))
	@$(call RM_CMD,$(BENCHMARK_TARGET))
	@echo "Clean complete."

# 帮助
help:
	@echo "Cross-Platform Test Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all   - Build the test program"
	@echo "  test  - Build and run tests"
	@echo "  clean - Remove test files"
	@echo "  help  - Show this help"
	@echo ""
	@echo "Platform: $(PLATFORM)"

.PHONY: all test clean help
