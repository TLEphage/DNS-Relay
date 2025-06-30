# DNS Relay Server Makefile - Cross Platform
# 项目名称
PROJECT_NAME = dnsrelay

# 检测操作系统
ifeq ($(OS),Windows_NT)
    # Windows 环境
    PLATFORM = windows
    TARGET_EXT = .exe
    LDFLAGS = -lws2_32
    MKDIR_CMD = if not exist "$(1)" mkdir "$(1)"
    RM_CMD = if exist "$(1)" rmdir /s /q "$(1)"
    RM_FILE_CMD = if exist "$(1)" del "$(1)"
    COPY_CMD = copy "$(1)" "$(2)" >nul
else
    # Linux/Unix 环境
    PLATFORM = linux
    TARGET_EXT =
    LDFLAGS =
    MKDIR_CMD = mkdir -p $(1)
    RM_CMD = rm -rf $(1)
    RM_FILE_CMD = rm -f $(1)
    COPY_CMD = cp $(1) $(2)
endif

# 编译器和选项
CC      := gcc
CFLAGS  := -Wall -O2 -Isrc -fcommon

# 目录设置
SRC_DIR = src
OBJ_DIR = obj
TARGET_DIR = .

# 源文件和目标文件
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(TARGET_DIR)/$(PROJECT_NAME)$(TARGET_EXT)

# 头文件目录已包含在CFLAGS中

# 默认目标
.PHONY: all clean rebuild help install

all: $(TARGET)

# 创建目标可执行文件
$(TARGET): $(OBJECTS) | $(TARGET_DIR)
	@echo "Linking $(PROJECT_NAME)..."
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"
	@echo ""
	@echo "===================================================================="
	@echo "|                    Build Successful!                            |"
	@echo "===================================================================="
	@echo "| Executable: $(TARGET)"
	@echo "| Usage: ./$(PROJECT_NAME).exe [options]"
	@echo "| Options:"
	@echo "|   -d    : Level 1 debugging"
	@echo "|   -dd   : Level 2 debugging" 
	@echo "|   -ddd  : Level 3 debugging"
	@echo "===================================================================="

# 编译源文件为目标文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 创建目录
$(OBJ_DIR):
	@$(call MKDIR_CMD,$(OBJ_DIR))

$(TARGET_DIR):
	@$(call MKDIR_CMD,$(TARGET_DIR))

# 清理编译文件
clean:
	@echo "Cleaning build files..."
	@$(call RM_CMD,$(OBJ_DIR))
	@$(call RM_FILE_CMD,$(TARGET))
	@$(call RM_FILE_CMD,dnsrelay.log)
	@echo "Clean complete."

# 重新构建
rebuild: clean all

# 安装（复制配置文件）
install: $(TARGET)
	@echo "Installing configuration files..."
ifeq ($(PLATFORM),windows)
	@if exist "dnsrelay.txt" $(call COPY_CMD,dnsrelay.txt,$(TARGET_DIR)\)
else
	@if [ -f "dnsrelay.txt" ]; then $(call COPY_CMD,dnsrelay.txt,$(TARGET_DIR)/); fi
endif
	@echo "Installation complete."

# 运行程序
run: $(TARGET)
	@echo "Starting DNS Relay Server..."
	@cd $(TARGET_DIR) && ./$(PROJECT_NAME)$(TARGET_EXT)

# 调试运行
debug: $(TARGET)
	@echo "Starting DNS Relay Server in debug mode..."
	@cd $(TARGET_DIR) && ./$(PROJECT_NAME)$(TARGET_EXT) -dd

# 显示帮助信息
help:
	@echo "DNS Relay Server Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all      - Build the project (default)"
	@echo "  clean    - Remove all build files"
	@echo "  rebuild  - Clean and build"
	@echo "  install  - Copy configuration files"
	@echo "  run      - Build and run the server"
	@echo "  debug    - Build and run with debug output"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Build configuration:"
	@echo "  Platform: $(PLATFORM)"
	@echo "  Compiler: $(CC)"
	@echo "  Flags:    $(CFLAGS)"
	@echo "  Linker:   $(LDFLAGS)"
	@echo "  Target:   $(TARGET)"
	@echo "  Sources:  $(words $(SOURCES)) files"

# 显示项目信息
info:
	@echo "Project Information:"
	@echo "  Name:     $(PROJECT_NAME)"
	@echo "  Sources:  $(SOURCES)"
	@echo "  Objects:  $(OBJECTS)"
	@echo "  Target:   $(TARGET)"
	@echo "  Compiler: $(CC) $(CFLAGS)"

# 依赖关系（简化版本，实际项目中可以使用更复杂的依赖生成）
$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/server.h $(SRC_DIR)/dnsStruct.h $(SRC_DIR)/log.h
$(OBJ_DIR)/server.o: $(SRC_DIR)/server.c $(SRC_DIR)/server.h $(SRC_DIR)/cache.h $(SRC_DIR)/response.h $(SRC_DIR)/dnsStruct.h $(SRC_DIR)/log.h
$(OBJ_DIR)/dnsStruct.o: $(SRC_DIR)/dnsStruct.c $(SRC_DIR)/dnsStruct.h
$(OBJ_DIR)/cache.o: $(SRC_DIR)/cache.c $(SRC_DIR)/cache.h $(SRC_DIR)/trie.h $(SRC_DIR)/dnsStruct.h
$(OBJ_DIR)/response.o: $(SRC_DIR)/response.c $(SRC_DIR)/response.h $(SRC_DIR)/dnsStruct.h $(SRC_DIR)/trie.h
$(OBJ_DIR)/log.o: $(SRC_DIR)/log.c $(SRC_DIR)/log.h
$(OBJ_DIR)/trie.o: $(SRC_DIR)/trie.c $(SRC_DIR)/trie.h
$(OBJ_DIR)/host.o: $(SRC_DIR)/host.c $(SRC_DIR)/host.h
