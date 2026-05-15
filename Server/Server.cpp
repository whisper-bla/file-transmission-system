#include <iostream>
#include "FileTransmission.h"
#include <unistd.h>
#include "POSIX_Thread_Pool.h"

// 文件传输：上传文件给客户端
void FilePut(FT_SOCKET &sock, const std::string &name)
{

    // 打开文件
    FILE *_v_filep = fopen(name.c_str(), "r");

    //---------不管文件是否打开成功都需要给到客户端反馈--------------
    // 说明文件不存在
    if (_v_filep == nullptr)
    {
        // 反馈给客户端
        FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "错误:文件不存在!");
        return;
    }
    // 说明文件打开正常
    FT_Package data;

    FT_Send(sock, FT_AC_MSG, FT_DT_MSG, std::string("success"));
    //------------------------------------------------------------

    //---------等待客户端的消息，是否准备就绪--------------
    std::string msg;
    FT_Recv(sock, msg);
    if (msg != "success")
    {
        fclose(_v_filep);
        return;
    }
    //--------------------------------------------------

    // 构建数据包
    data.pa_number = 1;         // 表示是第一个包
    data.pa_action = FT_AC_PUT; // 表示这个是传输给你的数据
    data.pa_type = FT_DT_FBODY; // 表示这个包是文件内容包
    data.pa_status = FT_DS_ST;  // 表示这个包是第一个包

    int packet_num = 1; // 序列号计数器

    // 循环读取文件的内容
    int _v_count = 0;

    do
    {
        char buffer[1024] = {0};
        _v_count = fread(buffer, 1, 1024, _v_filep);

        if (_v_count == 0)
        {
            break; // 没有数据了
        }

        // 判断是否读取出现错误
        if (_v_count == -1)
        {
            // 发送错误信息给到客户端
            FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "错误:文件获取出错!");
            fclose(_v_filep);
            return;
        }

        data.pa_number = packet_num++; // 序列号增加

        // 将文件内容填充到数据包种
        data.pa_size = _v_count;
        // strcpy(data.pa_context, buffer);
        memcpy(data.pa_context, buffer, _v_count);

        // 判断是不是最后一个包
        if (_v_count != 1024)
            data.pa_status = FT_DS_FH; // 表示为最后一个包
        else
            data.pa_status = FT_DS_TT; // 中间包状态

        FT_Send(sock, data); // 发送文件的内容给到客户端

        data.pa_status = FT_DS_TT; // 表示为中间的包

        usleep(100); // 加 0.1 毫秒延时

    } while (_v_count == 1024); // 能够读满就循环

    // 读完之后
    FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "success");

    fclose(_v_filep);
}

// 文件传输：从客户端获取文件
void FileGet(FT_SOCKET sock, const std::string &name)
{
    // ===== 添加：先读取并丢弃所有残留数据 =====
    // fd_set fds;
    // struct timeval tv = {0, 0};
    // FT_Package junk;

    // FD_ZERO(&fds);
    // FD_SET(sock.socketId, &fds);

    // while (select(sock.socketId + 1, &fds, NULL, NULL, &tv) > 0)
    // {
    //     FT_Recv(sock, junk);
    //     std::cout << "[服务端] 丢弃残留包: action=" << junk.pa_action << ", type=" << junk.pa_type << std::endl;
    //     FD_ZERO(&fds);
    //     FD_SET(sock.socketId, &fds);
    // }
    // =========================================

    // 创建文件
    FILE *_v_filep = fopen(name.c_str(), "w+");
    if (_v_filep == nullptr)
    {
        // 反馈给客户端
        FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "错误:文件无法创建!");
        return;
    }

    // 告诉客户端我准备好了
    FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "success");

    int expected_num = 1;  // 期望的序列号
    int file_received = 0; // 标记是否开始接收文件

    // 循环获取数据
    int _v_count = 0;
    do
    {
        FT_Package data;
        // 从客户端中读取数据
        FT_Recv(sock, data);

        // 调试：检查数据在传输过程中是否错位
        // std::cout << "[服务端] 收到包: action=" << data.pa_action << ", type=" << data.pa_type << ", size=" << data.pa_size << std::endl;

        // ===== 序列号校验 =====
        if (data.pa_action == FT_AC_PUT && data.pa_type == FT_DT_FBODY)
        {
            if (data.pa_number != expected_num)
            {
                std::cout << "警告：包序号错误！期望 " << expected_num
                          << "，收到 " << data.pa_number << std::endl;
            }
            expected_num++;
        }
        // ===========================

        // 判断数据的行为和类型
        // 判断数据的类型
        if (data.pa_action == FT_AC_MSG && data.pa_type == FT_DT_MSG)
        {
            std::cout << "传输日志:[" << inet_ntoa(sock.address.sin_addr) << "]" << data.pa_context << std::endl;

            // 如果已经收到过文件内容，这个消息就是结束标志
            if (file_received)
            {
                break;
            }
            // 否则继续等待文件内容
            continue;
            // return;
        }
        // 判断是不是最后一个包
        else if (data.pa_action == FT_AC_PUT && data.pa_type == FT_DT_FBODY && data.pa_status == FT_DS_FH)
        {
            file_received = 1;

            // 写入文件中
            fwrite(data.pa_context, 1, data.pa_size, _v_filep);

            if (data.pa_status == FT_DS_FH)
            {
                break;
            }
        }
        fwrite(data.pa_context, 1, data.pa_size, _v_filep);

    } while (1);

    // 接收服务端的反馈
    std::string msg;
    FT_Recv(sock, msg);
    if (msg == "success")
        std::cout << "文件上传完毕" << std::endl;
    else
        std::cout << "文件上传异常" << std::endl;

    fclose(_v_filep);
}

void Command(FT_SOCKET sock, const std::string &cmd)
{
    // std::cout << "执行命令: " << cmd << std::endl; // 调试

    // 打开一个程序的管道
    FILE *_v_pfilep = popen(cmd.c_str(), "r");

    if (_v_pfilep == nullptr)
    {
        perror("popen failed"); // 打印错误原因
        FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "命令执行失败");
        FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "<-cend->");
        return;
    }

    char buffer[1024] = {0};
    size_t bytes_read = 0;

    // 读取命令输出
    while ((bytes_read = fread(buffer, 1, sizeof(buffer) - 1, _v_pfilep)) > 0)
    {
        buffer[bytes_read] = '\0'; // 字符串结束符
        FT_Send(sock, FT_AC_MSG, FT_DT_MSG, buffer);
    }

    // 检查是否有错误
    if (ferror(_v_pfilep))
    {
        FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "读取命令输出时出错");
    }

    // 告诉客户端指令执行信息结束了
    FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "<-cend->");

    pclose(_v_pfilep);
}

// 客户端处理函数（给线程池用）
void handle_client(void *arg)
{
    FT_SOCKET clientSock = *(FT_SOCKET *)arg;
    delete (FT_SOCKET *)arg; // 释放内存

    std::cout << "线程 " << pthread_self() << " 开始处理客户端" << std::endl;

    while (1)
    {
        FT_Package data;
        if (!FT_Recv(clientSock, data))
        {
            std::cout << "客户端断开连接" << std::endl;
            break;
        }

        if (FT_AC_QUIT == data.pa_action)
            break;

        switch (data.pa_action)
        {
        case FT_AC_GET:
            std::cout << "客户端请求获取文件:[" << data.pa_context << "]" << std::endl;
            FilePut(clientSock, std::string(data.pa_context));
            break;
        case FT_AC_PUT:
            std::cout << "客户端请求上传文件:[" << data.pa_context << "]" << std::endl;
            FileGet(clientSock, (char *)data.pa_context);
            break;
        case FT_AC_MSG:
            std::cout << "收到消息: type=" << data.pa_type << ", context=" << data.pa_context << std::endl;
            if (data.pa_type == FT_DT_CMD)
            {
                std::cout << "客户端请求执行指令:" << data.pa_context << std::endl;
                Command(clientSock, (char *)data.pa_context);
            }
            else if (data.pa_type == FT_DT_MSG)
            {
                std::cout << "收到消息: " << data.pa_context << std::endl;
            }
            break;
        default:
            std::cout << "客户端未知指令请求:" << "[" << data.pa_context << "]" << std::endl;
            break;
        }
    }

    FT_ShutDown(clientSock);
    std::cout << "线程 " << pthread_self() << " 结束处理客户端" << std::endl;
}

int main()
{

    FT_SOCKET sock = FT_TcpServer("0.0.0.0", 8899);
    if (sock.socketId == -1)
    {
        perror("");
        return -1;
    }

    // 创建线程池（10个线程）
    ptp_t *thread_pool = ptp_init(10);
    std::cout << "服务端启动，线程池已创建（10个工作线程）..." << std::endl;
    std::cout << "等待客户端连接..." << std::endl;

    // 主要逻辑
    // while (1)
    // {
    //     FT_SOCKET clientSock = FT_Accept(sock);
    //     std::cout << "客户端连接" << std::endl;
    //     // 与客户端进行通信
    //     while (1)
    //     {
    //         // 准备一个数据包
    //         FT_Package data;

    //         // 接收客户端发送过来的指令
    //         FT_Recv(clientSock, data);

    //         if (FT_AC_QUIT == data.pa_action)
    //             break;

    //         // // 判断客户端发送过来的指令是什么行为
    //         switch (data.pa_action)
    //         {
    //         case FT_AC_GET:
    //             std::cout << "客户端请求获取文件:" << "[" << data.pa_context << "]" << std::endl;
    //             FilePut(clientSock, std::string(data.pa_context));
    //             break;
    //         case FT_AC_PUT:
    //             std::cout << "客户端请求上传文件:" << "[" << data.pa_context << "]" << std::endl;
    //             FileGet(clientSock, (char *)data.pa_context);
    //             break;
    //         case FT_AC_MSG:
    //             if (data.pa_type == FT_DT_CMD)
    //             {
    //                 // 是命令，执行
    //                 std::cout << "客户端请求执行指令:" << data.pa_context << std::endl;
    //                 Command(clientSock, (char *)data.pa_context);
    //             }
    //             else if (data.pa_type == FT_DT_MSG)
    //             {
    //                 // 是普通消息，打印日志
    //                 std::cout << "收到消息: " << data.pa_context << std::endl;
    //             }
    //             break;
    //         default:
    //             std::cout << "客户端未知指令请求:" << "[" << data.pa_context << "]" << std::endl;
    //             // FT_Send(clientSock,FT_AC_MSG,FT_DT_MSG,"未知指令");
    //             // FT_Send(clientSock,FT_AC_MSG,FT_DT_MSG,"<-cend->");
    //             break;
    //         }
    //     }
    //}

    while (1)
    {
        FT_SOCKET *clientSock = new FT_SOCKET;
        *clientSock = FT_Accept(sock);
        std::cout << "新客户端连接，交给线程池处理" << std::endl;

        // 把客户端处理任务添加到线程池
        ptp_add_task(thread_pool, handle_client, clientSock);
    }

    std::cout << "客户端断开连接" << std::endl;
    // FT_ShutDown(clientSock); // 端口客户端的连接

    FT_ShutDown(sock);
    ptp_destroy(thread_pool);
    return 0;
}