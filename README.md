# DNS-Relay
BUPT-计算机网络课程设计-DNS 中继服务器

[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue.svg)](https://github.com/your-repo/DNS-Relay)
## 🌟 项目特色

- **跨平台支持**：同时支持 Windows 和 Linux 系统，保持功能和性能一致性
- **高性能**：使用 LRU 缓存机制和 Trie 树优化查询性能
- **域名拦截**：支持不良网站拦截功能
- **易于配置**：简单的配置文件格式，支持多种调试级别
- **并发处理**：支持多客户端并发查询，具备完善的超时处理机制

## 📊 要求

1. 读入“域名-IP 地址”对照表，当客户端查询域名对应的 IP 地址时，用域名检索该对照表，可能有三种检索结果：
   1. 检索结果为 IP 地址 `0.0.0.0`，则向客户端返回“域名不存在”的报错消息，而不是返回 IP 地址为 `0.0.0.0`，实现不良网站拦截功能。
   2. 检索结果为普通 IP 地址，则向客户返回这个地址，实现 DNS 服务器功能。
   3. 表中未检到该域名，则向因特网 DNS 服务器发出查询，并将结果返给客户端，实现DNS 中继功能。
2. 根据 DNS 协议文本，按照协议要求，实现与 Windows 或其他系统的互联互通。
3. 多客户端并发：允许多个客户端（可能会位于不同的多个计算机）的并发查询，
即：允许第一个查询尚未得到答案前就启动处理另外一个客户端查询请求（DNS 协议头中 ID 字段的作用），需要进行消息 ID 的转换
4. 超时处理：由于 UDP 的不可靠性，考虑求助外部 DNS 服务器（中继）却不能得到应答或者收到迟到应答的情形
5. 实现 LRU 机制的 Cache 缓存
6. 使用 Trie 树实现字典查询算法的优化
7. 实现 Windows/Linux 源程序的一致性能

## 🚀 快速开始

### 系统要求

| 平台 | 最低要求 | 推荐配置 |
|------|----------|----------|
| **Windows** | Windows 7 SP1, MinGW/GCC 4.8+ | Windows 10, GCC 7.0+ |
| **Linux** | Linux 2.6.32+, GCC 4.8+ | Ubuntu 18.04+, GCC 7.0+ |

### 编译和安装

#### Windows 系统

```bash
# 克隆项目
git clone <repository-url>
cd DNS-Relay

# 编译
make all

# 运行
./dnsrelay.exe
```

#### Linux 系统

```bash
# 安装依赖 (Ubuntu/Debian)
sudo apt update
sudo apt install build-essential

# 克隆项目
git clone <repository-url>
cd DNS-Relay

# 编译
make all

# 运行 (需要root权限绑定53端口)
sudo ./dnsrelay
```

详细的Linux编译指南请参考：[Linux编译指南](docs/linux-build-guide.md)

### 基本使用

1. **配置系统DNS**：将 DNS 设置为 `127.0.0.1`
2. **启动服务器**：运行 `dnsrelay` 程序 (默认使用上游DNS服务器 `10.3.9.6`)
3. **测试功能**：正常使用 `ping`、`nslookup`、浏览器等，名字解析工作正常
4. **局域网使用**：其他计算机将DNS服务器指向本机IP地址即可使用

## 📖 使用说明

### 命令行参数

```bash
# 基本语法
dnsrelay [-d | -dd | -ddd] [dns-server-ipaddr] [filename]

# 参数说明
-d      # Level 1 调试：输出基本信息（时间、序号、客户端IP、查询域名）
-dd     # Level 2 调试：输出详细调试信息
-ddd    # Level 3 调试：输出字节级详细信息

# 可选参数
[dns-server-ipaddr]  # 上游DNS服务器地址，默认为 10.3.9.6
[filename]           # 配置文件路径，默认为 dnsrelay.txt
```

### 使用示例

```bash
# 基本运行
./dnsrelay

# 调试模式运行
./dnsrelay -dd

# 指定上游DNS服务器
./dnsrelay 8.8.8.8

# 指定配置文件
./dnsrelay custom_hosts.txt

# 完整参数示例
./dnsrelay -d 8.8.8.8 custom_hosts.txt
```

### 配置文件格式

创建 `dnsrelay.txt` 文件，格式如下：

```
# DNS中继器配置文件
# 格式：IP地址 域名
# 使用 0.0.0.0 表示拦截该域名

192.168.1.100 example.com
10.0.0.50 test.local
0.0.0.0 blocked-site.com
```

### 测试命令

```bash
# 测试DNS解析
nslookup www.bupt.edu.cn 127.0.0.1

# 查看DNS缓存 (Windows)
ipconfig /displaydns

# 清除DNS缓存 (Windows)
ipconfig /flushdns

# Linux下的测试命令
dig @127.0.0.1 www.bupt.edu.cn
```

## 🔧 高级功能

### 跨平台特性

- **统一API**：使用条件编译实现跨平台网络API统一
- **性能一致**：Windows和Linux版本保持相同的性能表现
- **配置兼容**：配置文件格式在两个平台上完全兼容

### 性能优化

- **LRU缓存**：智能缓存机制，提高查询响应速度
- **Trie树索引**：优化域名查找算法，支持大规模域名表
- **并发处理**：支持多客户端同时查询，具备事务ID管理
- **超时处理**：完善的超时重试机制，提高服务可靠性

### 安全功能

- **域名拦截**：支持黑名单功能，可拦截恶意或不良网站
- **日志记录**：详细的访问日志，便于监控和分析
- **错误处理**：完善的错误处理机制，提高系统稳定性

## 📚 文档

- [跨平台实现原理](docs/cross-platform-implementation.md) - 详细的技术实现文档
- [Linux编译指南](docs/linux-build-guide.md) - Linux系统专用编译和配置指南
- [API参考](docs/api-reference.md) - 开发者API文档

## 🧪 测试

### 运行测试套件

```bash
# 编译并运行跨平台测试
cd test
make test

# 运行性能测试
make benchmark
```

### 功能验证

项目包含完整的测试套件，验证：
- 网络初始化和清理
- 套接字创建和配置
- 非阻塞模式设置
- IP地址转换功能
- 跨平台兼容性

---

**注意**：在生产环境中使用时，请确保：
1. 使用root权限运行以绑定53端口
2. 配置适当的防火墙规则
3. 定期更新域名拦截列表
4. 监控服务运行状态和性能指标