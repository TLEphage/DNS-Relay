# DNS中继器 Linux 编译和使用指南

## 概述

本指南详细介绍如何在Linux系统上编译、配置和运行DNS中继器。该项目已经过跨平台改造，支持在Linux和Windows系统上运行，并保持功能和性能的一致性。

## 系统要求

### 最低要求
- Linux内核版本：2.6.32 或更高
- GCC编译器：4.8 或更高版本
- GNU Make：3.81 或更高版本
- 内存：至少 64MB 可用内存
- 磁盘空间：至少 10MB 可用空间

### 推荐配置
- Ubuntu 18.04 LTS 或更高版本
- CentOS 7 或更高版本
- Debian 9 或更高版本
- GCC 7.0 或更高版本

## 依赖安装

### Ubuntu/Debian 系统

```bash
# 更新包管理器
sudo apt update

# 安装编译工具
sudo apt install build-essential

# 安装必要的开发库
sudo apt install libc6-dev

# 验证安装
gcc --version
make --version
```

### CentOS/RHEL 系统

```bash
# 安装开发工具组
sudo yum groupinstall "Development Tools"

# 或者在较新版本中使用 dnf
sudo dnf groupinstall "Development Tools"

# 验证安装
gcc --version
make --version
```

### Arch Linux 系统

```bash
# 安装基础开发工具
sudo pacman -S base-devel

# 验证安装
gcc --version
make --version
```

## 编译步骤

### 1. 获取源代码

```bash
# 克隆或下载项目源代码
git clone <repository-url>
cd DNS-Relay

# 或者解压源代码包
tar -xzf DNS-Relay.tar.gz
cd DNS-Relay
```

### 2. 检查编译环境

```bash
# 查看Makefile配置
make help

# 输出示例：
# DNS Relay Server Makefile
# 
# Available targets:
#   all      - Build the project (default)
#   clean    - Remove all build files
#   rebuild  - Clean and build
#   install  - Copy configuration files
#   run      - Build and run the server
#   debug    - Build and run with debug output
#   help     - Show this help message
# 
# Build configuration:
#   Platform: linux
#   Compiler: gcc
#   Flags:    -Wall -O2 -Isrc -fcommon
#   Linker:   
#   Target:   ./dnsrelay
#   Sources:  9 files
```

### 3. 编译项目

```bash
# 清理之前的编译文件（可选）
make clean

# 编译项目
make all

# 编译成功后会显示：
# ====================================================================
# |                    Build Successful!                            |
# ====================================================================
# | Executable: ./dnsrelay
# | Usage: ./dnsrelay [options]
# | Options:
# |   -d    : Level 1 debugging
# |   -dd   : Level 2 debugging
# |   -ddd  : Level 3 debugging
# ====================================================================
```

### 4. 验证编译结果

```bash
# 检查可执行文件
ls -la dnsrelay

# 运行跨平台测试
cd test
make test

# 测试输出示例：
# === DNS Relay Cross-Platform Test Suite ===
# 
# Running on Linux platform
# 
# Testing network initialization and cleanup...
# ✓ Network initialization successful
# ✓ Network cleanup successful
# 
# Testing socket creation...
# ✓ Socket creation successful
# ✓ Socket close successful
# 
# === All tests passed! ===
```

## 配置和运行

### 1. 配置文件

创建或编辑配置文件：

```bash
# 复制示例配置文件
cp dnsrelay.txt.example dnsrelay.txt

# 编辑配置文件
nano dnsrelay.txt
```

配置文件格式示例：
```
# DNS中继器配置文件
# 格式：域名 IP地址
# 0.0.0.0 表示拦截该域名

example.com 192.168.1.100
blocked-site.com 0.0.0.0
local-server.lan 192.168.1.50
```

### 2. 运行DNS中继器

#### 基本运行

```bash
# 以root权限运行（DNS服务需要绑定53端口）
sudo ./dnsrelay

# 或者使用其他端口（测试用）
./dnsrelay -p 5353
```

#### 调试模式运行

```bash
# Level 1 调试（基本信息）
sudo ./dnsrelay -d

# Level 2 调试（详细信息）
sudo ./dnsrelay -dd

# Level 3 调试（字节级信息）
sudo ./dnsrelay -ddd
```

#### 后台运行

```bash
# 使用 nohup 后台运行
sudo nohup ./dnsrelay > dnsrelay.log 2>&1 &

# 使用 systemd 服务（推荐）
# 创建服务文件
sudo nano /etc/systemd/system/dnsrelay.service
```

systemd 服务文件示例：
```ini
[Unit]
Description=DNS Relay Server
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/path/to/DNS-Relay
ExecStart=/path/to/DNS-Relay/dnsrelay -d
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

启用和管理服务：
```bash
# 重新加载systemd配置
sudo systemctl daemon-reload

# 启用服务
sudo systemctl enable dnsrelay

# 启动服务
sudo systemctl start dnsrelay

# 查看服务状态
sudo systemctl status dnsrelay

# 查看日志
sudo journalctl -u dnsrelay -f
```

### 3. 网络配置

#### 配置系统DNS

```bash
# 备份原始DNS配置
sudo cp /etc/resolv.conf /etc/resolv.conf.backup

# 配置使用本地DNS中继器
echo "nameserver 127.0.0.1" | sudo tee /etc/resolv.conf

# 或者编辑配置文件
sudo nano /etc/resolv.conf
```

#### 防火墙配置

```bash
# UFW防火墙
sudo ufw allow 53/udp

# iptables防火墙
sudo iptables -A INPUT -p udp --dport 53 -j ACCEPT

# firewalld防火墙
sudo firewall-cmd --permanent --add-port=53/udp
sudo firewall-cmd --reload
```

## 测试验证

### 1. 功能测试

```bash
# 测试DNS解析
nslookup google.com 127.0.0.1
dig @127.0.0.1 google.com

# 测试域名拦截（如果配置了拦截规则）
nslookup blocked-site.com 127.0.0.1
```

### 2. 性能测试

```bash
# 使用dig进行批量测试
for i in {1..100}; do
    dig @127.0.0.1 google.com > /dev/null
done

# 使用dnsperf工具（需要安装）
sudo apt install dnsperf
echo "google.com A" > test_queries.txt
dnsperf -s 127.0.0.1 -d test_queries.txt
```

## 故障排除

### 常见问题

1. **权限不足错误**
   ```bash
   # 错误：Permission denied (bind)
   # 解决：使用sudo运行或更改端口
   sudo ./dnsrelay
   ```

2. **端口被占用**
   ```bash
   # 检查端口占用
   sudo netstat -tulpn | grep :53
   
   # 停止其他DNS服务
   sudo systemctl stop systemd-resolved
   ```

3. **编译错误**
   ```bash
   # 检查GCC版本
   gcc --version
   
   # 安装缺失的开发包
   sudo apt install build-essential
   ```

### 日志分析

```bash
# 查看实时日志
tail -f dnsrelay.log

# 搜索错误信息
grep "ERROR" dnsrelay.log

# 分析DNS查询统计
grep "DNS packet" dnsrelay.log | wc -l
```

## 性能优化

### 系统级优化

```bash
# 增加文件描述符限制
echo "* soft nofile 65536" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 65536" | sudo tee -a /etc/security/limits.conf

# 优化网络参数
echo "net.core.rmem_max = 16777216" | sudo tee -a /etc/sysctl.conf
echo "net.core.wmem_max = 16777216" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

### 应用级优化

1. **调整缓存大小**：修改源代码中的缓存容量
2. **优化上游DNS**：选择响应速度快的上游DNS服务器
3. **配置预加载**：在配置文件中添加常用域名

## 卸载

```bash
# 停止服务
sudo systemctl stop dnsrelay
sudo systemctl disable dnsrelay

# 删除服务文件
sudo rm /etc/systemd/system/dnsrelay.service
sudo systemctl daemon-reload

# 恢复DNS配置
sudo cp /etc/resolv.conf.backup /etc/resolv.conf

# 删除程序文件
rm -rf /path/to/DNS-Relay
```

## 技术支持

如果遇到问题，请：

1. 查看日志文件获取详细错误信息
2. 运行跨平台测试验证基本功能
3. 检查网络配置和防火墙设置
4. 参考跨平台实现原理文档

更多技术细节请参考：`docs/cross-platform-implementation.md`
