# 文件传输系统

基于 TCP 的自定义协议文件传输系统，支持文件上传/下载、远程命令执行、多客户端并发。

## 功能

- 文件上传 (PUT)
- 文件下载 (GET)
- 远程命令执行 (MSG)
- 多客户端并发（线程池）
- 大文件传输支持

## 快速开始

### 编译

```bash
# 服务端
g++ Server.cpp FileTransmission.cpp POSIX_Thread_Pool.cpp -o server -D LINUX -pthread

# 客户端
g++ main.cpp FileTransmission.cpp -o client -D LINUX

运行
bash
./server   # 终端1
./client   # 终端2

使用
text
PUT test.txt      # 上传
GET test.txt      # 下载
MSG ls -la        # 命令
QUIT              # 退出

协议设计
每个数据包固定 1044 字节：
字段	       大小	   说明
pa_number	     4	   包序号
pa_action	     4	   GET/PUT/MSG/QUIT
pa_type	       4	   文件名/文件内容/消息
pa_status	     4	   开始/中间/结束
pa_size	       4	   数据长度
pa_context	1024	   数据内容

技术栈
C++ / Socket / TCP

多线程（pthread）

作者
whisper-bla
