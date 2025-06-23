# DNS-Relay
BUPT-计算机网络课程设计-DNS 中继服务器

## 要求

1. 读入“域名-IP 地址”对照表，当客户端查询域名对应的 IP 地址时，用域名检索该对照表，可能有三种检索结果：
   1. 检索结果为 ip 地址 0.0.0.0，则向客户端返回“域名不存在”的报错消息，而不是返回 IP 地址为 0.0.0.0，实现不良网站拦截功能。
   2. 检索结果为普通 IP 地址，则向客户返回这个地址，实现 DNS 服务器功能。
   3. 表中未检到该域名，则向因特网 DNS 服务器发出查询，并将结果返给客户端，实现DNS 中继功能。
2. 根据 DNS 协议文本，按照协议要求，实现与 Windows 或其他系统的互联互通。
3. 多客户端并发：允许多个客户端（可能会位于不同的多个计算机）的并发查询，
即：允许第一个查询尚未得到答案前就启动处理另外一个客户端查询请求（DNS 协议头中 ID 字段的作用），需要进行消息 ID 的转换
4. 超时处理：由于 UDP 的不可靠性，考虑求助外部 DNS 服务器（中继）却不能得到应答或者收到迟到应答的情形
5. 实现 LRU 机制的 Cache 缓存
6. 使用 Trie 树实现字典查询算法的优化
7. 实现 Windows/Linux 源程序的一致性能

## 运行步骤

1. 将 DNS 设置为 `127.0.0.1`
2. 运行 `dnsrelay` 程序 (在程序中把外部 dns 服务器设为 `202.106.0.20`)
3. 正常使用 ping，ftp，IE 等，名字解析工作正常
4. 局域网上的其他计算机（Windows 或 Linux）将域名服务器指向 DNS 中继服务器的 IP 地址，ftp, IE 等均能正常工作

- `dnsrelay` 程序运行命令：`dnsrelay [-d | -dd] [dns-server-ipaddr] [filename]`
  - `dnsrelay` : 无调试信息输出
  - `dnsrelay –d` : 输出调试信息(仅输出时间坐标，序号，客户端 IP 地址，查询的域名)
  - `dnsrelay –dd` : 输出详细调试信息
  - `[filename]` 留空则使用默认配置文件`dnsrelay.txt`
  - `[dns-server-ipaddr]` 留空则使用默认名字服务器`202.106.0.20`

**其他命令：**
- nslookup www.bupt.edu.cn：向名字服务器询问名字 www.bupt.edu.cn 的地址
- ipconfig/displaydns: 察看当前 dns cache 的内容以确认程序执行结果的正确性
- ipconfig/flushdns: 清除 dns cache 中缓存的所有 DNS 记录