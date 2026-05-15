#include <iostream>
#include "FileTransmission.h"
#include <unistd.h>

// 文件传输：上传文件给服务端
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

    //---------等待服务端的消息，是否准备就绪--------------
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

    int packet_num = 1; // ← 新增：序列号计数器

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

        data.pa_number = packet_num++; // 原来 data.pa_number = 1

        // 将文件内容填充到数据包种
        data.pa_size = _v_count;
        // strcpy(data.pa_context, buffer);
        memcpy(data.pa_context, buffer, _v_count);

        // 判断是不是最后一个包
        if (_v_count != 1024)
            data.pa_status = FT_DS_FH; // 表示为最后一个包
        else
            data.pa_status = FT_DS_TT; // 中间包状态

        // 调试问题用的
        // std::cout << "[客户端] 发送包: type=" << data.pa_type << ", size=" << _v_count << std::endl;

        FT_Send(sock, data); // 发送文件的内容给到客户端

        data.pa_status = FT_DS_TT; // 表示为中间的包

        usleep(100); // 加 0.1 毫秒延时

    } while (_v_count == 1024); // 能够读满就循环

    // 读完之后
    FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "success");

    fclose(_v_filep);
}

// 文件传输：从服务端获取文件
void FileGet(FT_SOCKET sock, const std::string &name, bool is_retry = false)
{
    // 如果是重试，先等待一下，让缓冲区稳定
    if (is_retry)
    {
        usleep(100000); // 等待100毫秒
    }

    // 创建文件
    FILE *_v_filep = fopen(name.c_str(), "w+");
    if (_v_filep == nullptr)
    {
        // 反馈给客户端
        FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "错误:文件无法创建!");
        return;
    }

    // 告诉客户端我准备好了
    FT_Package data;
    FT_Send(sock, FT_AC_MSG, FT_DT_MSG, "success");

    int expected_num = 1;  // 期望的序列号
    int file_received = 0; // 标记是否开始接收文件
    int total_bytes = 0;   // 调试

    // 循环获取数据
    int _v_count = 0;
    do
    {
        // 从客户端中读取数据
        FT_Recv(sock, data);

        // ===== 新增：序列号校验 =====
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
            // fclose(_v_filep);
            // return;
        }
        // 判断是不是最后一个包
        else if (data.pa_action == FT_AC_PUT && data.pa_type == FT_DT_FBODY, data.pa_status == FT_DS_FH)
        {
            file_received = 1;

            // 写入文件中
            fwrite(data.pa_context, 1, data.pa_size, _v_filep);

            // 调试
            total_bytes += data.pa_size;
            std::cout << "累计接收: " << total_bytes << " 字节" << std::endl;

            if (data.pa_status == FT_DS_FH)
            {
                break;
            }
            // break;
        }
        fwrite(data.pa_context, 1, data.pa_size, _v_filep);
    } while (1);

    // 接收服务端的反馈
    // std::string msg;
    // FT_Recv(sock, msg);
    FT_Package ack_data;
    FT_Recv(sock, ack_data);

    // 调试
    // std::cout << "调试: action=" << ack_data.pa_action << ", type=" << ack_data.pa_type << ", context=" << ack_data.pa_context << std::endl;

    if (ack_data.pa_action == FT_AC_MSG && ack_data.pa_type == FT_DT_MSG && strcmp(ack_data.pa_context, "success") == 0)
        std::cout << "文件上传完毕" << std::endl;
    else
        std::cout << "文件上传异常" << std::endl;
    fclose(_v_filep);
}

// windows 编译需要带 -lws2_32  网络库
int main()
{

    FT_SOCKET sock = FT_TcpClient("127.0.0.1", 8899);
    if (sock.socketId == FT_INVALID)
    {
        std::cerr << sock.socketId << std::endl;
        return -1;
    }

    while (1)
    {
        std::string cmd;
        std::cout << "Please Enter:";
        std::cin >> cmd;

        if (cmd == "GET")
        {
            std::string filename;
            std::cin >> filename;

            FT_Package data;
            data.pa_number = 1;
            data.pa_action = FT_AC_GET;
            data.pa_type = FT_DT_FNAME;
            data.pa_status = FT_DS_ST;
            data.pa_size = filename.size();
            strcpy(data.pa_context, filename.c_str());
            FT_Send(sock, data);
            FileGet(sock, filename);

            // 检查文件是否为空
            FILE *check = fopen(filename.c_str(), "r");
            if (check)
            {
                fseek(check, 0, SEEK_END);
                long size = ftell(check);
                fclose(check);

                if (size == 0)
                {
                    // 文件为空，自动重试
                    remove(filename.c_str());      // 删除空文件
                    FT_Send(sock, data);           // 重新发送文件名
                    FileGet(sock, filename, true); // 重试
                }
            }
        }
        else if (cmd == "PUT")
        {
            std::string filename;
            std::cin >> filename;

            FT_Package data;
            data.pa_number = 1;
            data.pa_action = FT_AC_PUT;
            data.pa_type = FT_DT_FNAME;
            data.pa_status = FT_DS_ST;
            data.pa_size = filename.size();
            strcpy(data.pa_context, filename.c_str());
            FT_Send(sock, data);
            FilePut(sock, filename);
        }
        else if (cmd == "MSG")
        {
            std::string command;
            std::cout << "请输入命令: ";
            std::cin.ignore();               // 清除之前的换行符
            std::getline(std::cin, command); // 获取整行命令

            std::cout << "发送命令: [" << command << "]" << std::endl; // 调试

            // 发送命令给服务端
            FT_Send(sock, FT_AC_MSG, FT_DT_CMD, command);

            // 接收并打印执行结果
            while (1)
            {
                FT_Package data;
                FT_Recv(sock, data);

                if (data.pa_action == FT_AC_MSG && data.pa_type == FT_DT_MSG)
                {
                    std::string result(data.pa_context);

                    if (result == "<-cend->")
                        break;

                    std::cout << result;
                }
            }
        }
        else if (cmd == "QUIT" || cmd == "EXIT")
        {
            FT_Package quit;
            quit.pa_number = 1;
            quit.pa_action = FT_AC_QUIT;
            quit.pa_type = FT_DT_MSG;
            quit.pa_status = FT_DS_ST;
            quit.pa_size = 0;
            FT_Send(sock, quit);
            break; // 退出客户端的 while 循环
        }
    }

    FT_ShutDown(sock);
    return 0;
}