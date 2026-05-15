# 文件传输系统

基于 TCP 的自定义协议文件传输系统，支持文件上传/下载、远程命令执行、多客户端并发。

## 功能

- 文件上传 (PUT)
- 文件下载 (GET)  
- 远程命令执行 (MSG)
- 多客户端并发（线程池）
- 大文件传输（GB级）

## 快速开始

```bash
# 编译服务端
g++ Server.cpp FileTransmission.cpp POSIX_Thread_Pool.cpp -o server -D LINUX -pthread

# 编译客户端
g++ main.cpp FileTransmission.cpp -o client -D LINUX

# 运行
./server    # 终端1
./client    # 终端2
