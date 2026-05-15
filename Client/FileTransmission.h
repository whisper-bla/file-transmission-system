#pragma once

// 这个文件传输的API组可以进行跨平台的开发

// 需要的头文件
#ifdef LINUX  // 如果定义了LINUX宏
#ifndef WIN32 // 没有定义WIN32宏,说明这个是在Linux平台使用
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define FT_INVALID -1
typedef int FT_SOCK_ID;
#define FT_SOCKLEN socklen_t
#endif
#endif

#ifdef WIN32  // 如果定义了WIN32宏
#ifndef LINUX // 没有定义LINUX宏,说明这个是在Windows平台使用
#include <winsock2.h>
#include <windows.h>

#define FT_INVALID INVALID_SOCKET
typedef SOCKET FT_SOCK_ID;
#define FT_SOCKLEN int
#endif
#endif

#include <iostream>
#include <string.h>

/*
文件传输的API组说明：
    包含数据封装：
        FT_SOCKET        套接字类型
        FT_INVALID       套接字错误
        FT_Action        网络数据的行为
        FT_DataType      网络数据的类型
        FT_DataStatus    网络数据包的状态
        FT_Package       网络数据包的封装
    包含函数封装：
        FT_TcpServer     获取一个Tcp的服务端套接字
        FT_TcpClient     获取一个Tcp的客户端套接字
        FT_UdpSocket     获取一个Udp的套接字
        FT_Accept        等待一个客户端连接
        FT_ShutDown      关闭一个网络通道
        FT_Send          通过指定的套接字发送一个网络数据包
        FT_Recv          通过指定的套接字获取一个网络数据包
*/

typedef enum
{
    FT_TCP,
    FT_UDP
} FT_INET_TYPE;
// 网络通道类型
struct FT_SOCKET
{
    bool status;
    FT_INET_TYPE inetType;
    FT_SOCK_ID socketId;
    struct sockaddr_in address;
    FT_SOCKET() {}
    FT_SOCKET(int) {}
};

// 数据包内容的最大值
#define PACKAGESIZE 1024
#define FT_FAILED ((FT_SOCKET)0)

/*
网络数据的行为:
    FT_AC_GET (filetransmission_Action_get)     获取行为
    FT_AC_PUT (filetransmission_Action_put)     上传行为
    FT_AC_MSG (filetransmission_Action_message) 消息行为
    FT_AC_QUIT(filetransmission_Action_quit)    退出行为
*/
typedef enum
{
    FT_AC_GET,
    FT_AC_PUT,
    FT_AC_MSG,
    FT_AC_QUIT
} FT_Action;

/*
网络数据的类型:
    FT_DT_FNAME (filetransmission_DataType_file name)    文件名
    FT_DT_FBODY (filetransmission_DataType_file body)    文件内容
    FT_DT_MSG   (filetransmission_DataType_message)      消息
    FT_DT_CMD  (filetransmission_DataType_command)      系统命令
*/
typedef enum
{
    FT_DT_FNAME,
    FT_DT_FBODY,
    FT_DT_MSG,
    FT_DT_CMD
} FT_DataType;

/*
网络数据包的状态:
    FT_DS_ST (filetransmission_DataStatus_Start)     开始（第一个数据包）
    FT_DS_TT (filetransmission_DataStatus_Transport) 传输中（中间数据包）
    FT_DS_FH (filetransmission_DataStatus_Finish)    结束（最后一个包）
*/
typedef enum
{
    FT_DS_ST,
    FT_DS_TT,
    FT_DS_FH
} FT_DataStatus;

typedef struct
{
    int pa_number;                // 本次数据的序列
    FT_Action pa_action;          // 本次数据的行为
    FT_DataType pa_type;          // 本次数据的类型
    FT_DataStatus pa_status;      // 本次数据的状态
    int pa_size;                  // 本次数据的大小
    char pa_context[PACKAGESIZE]; // 本次数据的内容
} FT_Package;

// API
// 获取一个Tcp的服务端套接字
/*
    @brief 生成一个TCP服务端套接字
    @param ipv4 需要绑定的ipv4地址，可以为空，为空默认绑定INADDR_ANY
    @param port 需要绑定的端口号，可以为空，为空默认绑定8899
    @return FT_SOCKET 成功返回一个创建好的套接字，失败返回FT_INVALID
*/
FT_SOCKET FT_TcpServer(std::string ipv4 = "", int port = 8899);

// 获取一个Tcp的客户端套接字
/*
    @brief 生成一个TCP客户端端套接字
    @param ipv4 需要连接 服务端的ipv4地址
    @param port 需要连接 服务端的端口号
    @return FT_SOCKET 成功返回一个创建好的套接字，失败返回FT_INVALID
*/
FT_SOCKET FT_TcpClient(std::string ipv4, int port);

// 获取一个Udp的套接字
/*
    @brief 生成一个TCP客户端端套接字
    @param ipv4 数据包需要发送到的目标ipv4地址
    @param port 数据包需要发送到的目标端口号
    @return FT_SOCKET 成功返回一个创建好的套接字，失败返回FT_INVALID
*/
FT_SOCKET FT_UdpSocket(std::string ipv4, int port);

// 等待一个客户端连接
/*
    @brief 等待一个客户端连接
    @param socketId 需要等待客户端连接的服务端
    @return 成功返回一个客户端网络通道，失败返回FT_FAILED
*/
FT_SOCKET FT_Accept(FT_SOCKET &socketId);

// 关闭网络通道
/*
    @brief 关闭一个网络通道
    @param socket 需要关闭的网络通
*/
void FT_ShutDown(FT_SOCKET &socketId);

// 通过指定的套接字发送一个网络数据包
bool FT_Send(FT_SOCKET &socketId, FT_Package &data);
bool FT_Send(FT_SOCKET &socketId, FT_Action action, FT_DataType type, std::string data);

// 通过指定的套接字获取一个网络数据包
bool FT_Recv(FT_SOCKET &socketId, FT_Package &data);
bool FT_Recv(FT_SOCKET &socketId, std::string &data);