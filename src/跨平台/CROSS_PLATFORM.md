# DNS Relay Server 跨平台指南

本项目已经实现了Windows和Linux的跨平台兼容性，同样的代码可以在两个平台上编译和运行。

## 跨平台实现原理

### 1. 平台检测
- 使用 `#ifdef _WIN32` 来检测Windows平台
- 使用 `#else` 分支处理Linux/Unix平台

### 2. 统一的API抽象
创建了 `src/platform.h` 和 `src/platform.c` 文件，提供统一的跨平台接口：

#### 网络API统一
- **Windows**: 使用 WinSock2 API (WSAStartup, closesocket, WSAPoll等)
- **Linux**: 使用 标准socket API (socket, close, poll等)
- **统一接口**: create_udp_socket(), close_socket_safe(), poll_sockets()等

#### 数据类型统一
- **socket_t**: Windows下为SOCKET，Linux下为int
- **socklen_t**: Windows下需要定义，Linux下原生支持
- **pollfd_t**: Windows下为WSAPOLLFD，Linux下为struct pollfd

#### 错误处理统一
- **Windows**: WSAGetLastError() + FormatMessage()
- **Linux**: errno + strerror()
- **统一接口**: get_error_string()

### 3. 编译系统适配
Makefile自动检测操作系统并使用相应的编译选项：

#### Windows平台
```makefile
CC      := gcc 
CFLAGS  := -Wall -O2 -Isrc -fcommon -D_WIN32
LDFLAGS := -lws2_32
TARGET_EXT := .exe
```

#### Linux平台
```makefile
CC      := gcc 
CFLAGS  := -Wall -O2 -Isrc -fcommon
LDFLAGS := 
TARGET_EXT := 
```

## 编译和运行

### Windows平台
```bash
# 编译
make all

# 运行
make run
# 或者
./dnsrelay.exe

# 调试模式
make debug
# 或者
./dnsrelay.exe -dd
```

### Linux平台
```bash
# 编译
make all

# 运行
make run
# 或者
./dnsrelay

# 调试模式
make debug
# 或者
./dnsrelay -dd
```

## 主要修改的文件

### 新增文件
- `src/platform.h` - 跨平台头文件，统一API定义
- `src/platform.c` - 跨平台实现文件

### 修改的文件
- `src/server.h` - 使用跨平台类型定义
- `src/server.c` - 使用跨平台网络API
- `src/host.h` - 移除Windows特定头文件
- `src/main.c` - 使用跨平台清理函数
- `Makefile` - 添加平台检测和相应编译选项

## 跨平台兼容性特性

### ✅ 已实现
- [x] 网络socket操作 (创建、绑定、发送、接收)
- [x] 非阻塞IO和poll机制
- [x] 错误处理和日志记录
- [x] 文件操作 (配置文件读取)
- [x] 内存管理
- [x] 编译系统自动适配

### 🔧 注意事项
1. **编译器要求**: 需要支持C11标准的gcc编译器
2. **依赖库**: 
   - Windows: 需要ws2_32库 (通常系统自带)
   - Linux: 无额外依赖
3. **配置文件**: dnsrelay.txt和dnsrelay_ipv6.txt在两个平台上格式相同

### 📝 使用建议
1. 在Windows上开发时，建议使用MinGW或MSYS2环境
2. 在Linux上编译前，确保安装了build-essential包
3. 两个平台的配置文件可以直接复制使用
4. 日志文件格式在两个平台上保持一致

## 测试验证

### 功能测试
```bash
# 编译测试
make clean && make all

# 基本功能测试
make run

# 调试模式测试
make debug
```

### 平台特定测试
- **Windows**: 在PowerShell或CMD中运行
- **Linux**: 在bash shell中运行
- 验证DNS解析功能在两个平台上行为一致

这样的跨平台设计确保了代码的可移植性，同时保持了在各自平台上的最佳性能。
