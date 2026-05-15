#include "FileTransmission.h"

// 获取一个Tcp的服务端套接字
/*
    @brief 生成一个TCP服务端套接字
    @param ipv4 需要绑定的ipv4地址，可以为空，为空默认绑定INADDR_ANY
    @param port 需要绑定的端口号，可以为空，为空默认绑定8899
    @return FT_SOCKET 成功返回一个创建好的套接字，失败返回FT_INVALID
*/
FT_SOCKET FT_TcpServer(std::string ipv4, int port)
{
#ifdef WIN32  // 如果定义了WIN32宏
#ifndef LINUX // 没有定义LINUX宏,说明这个是在Windows平台使用
    // 初始化网络库
    WSADATA wd;
    WSAStartup(MAKEWORD(2, 2), &wd);
#endif
#endif
    FT_SOCKET sock;
    sock.socketId = socket(AF_INET, SOCK_STREAM, 0);
    if (sock.socketId == FT_INVALID)
        return FT_FAILED;

    sock.address.sin_family = AF_INET;
    sock.address.sin_port = htons(port);
    sock.address.sin_addr.s_addr = ipv4 == "" ? INADDR_ANY : inet_addr(ipv4.c_str());

    if (bind(sock.socketId, (struct sockaddr *)&sock.address, sizeof(sock.address)) == -1)
    {
        FT_ShutDown(sock);
        return sock;
    }

    if (listen(sock.socketId, 10) == -1)
    {
        FT_ShutDown(sock);
        return sock;
    }

    const char opt = 1;
    setsockopt(sock.socketId, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sock.inetType = FT_TCP;
    sock.status = true;

    return sock;
}

// 获取一个Tcp的客户端套接字
/*
    @brief 生成一个TCP客户端端套接字
    @param ipv4 需要连接 服务端的ipv4地址
    @param port 需要连接 服务端的端口号
    @return FT_SOCKET 成功返回一个创建好的套接字，失败返回FT_INVALID
*/
FT_SOCKET FT_TcpClient(std::string ipv4, int port)
{
#ifdef WIN32  // 如果定义了WIN32宏
#ifndef LINUX // 没有定义LINUX宏,说明这个是在Windows平台使用
    // 初始化网络库
    WSADATA wd;
    int ret = WSAStartup(MAKEWORD(2, 2), &wd);
    if (0 != ret)
    {
        std::cerr << ret << std::endl;
    }
#endif
#endif
    FT_SOCKET sock;
    sock.socketId = socket(AF_INET, SOCK_STREAM, 0);
    if (sock.socketId == FT_INVALID)
        return sock;

    //  bind实现客户端绑定固定端口的功能。服务端固定端口，客户端不固定端口，这是网络编程的标准做法。
    //  sock.address.sin_family = AF_INET;
    //  sock.address.sin_port = htons(port);
    //  sock.address.sin_addr.s_addr = INADDR_ANY;

    // if (bind(sock.socketId, (struct sockaddr *)&sock.address, sizeof(sock.address)) == -1)
    // {
    //     FT_ShutDown(sock);
    //     return sock;
    // }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ipv4.c_str());

    if (::connect(sock.socketId, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("connect failed");
        FT_ShutDown(sock);
        return sock;
    }

    sock.inetType = FT_TCP;
    sock.status = true;

    return sock;
}

// 获取一个Udp的套接字
/*
    @brief 生成一个TCP客户端端套接字
    @param ipv4 数据包需要发送到的目标ipv4地址
    @param port 数据包需要发送到的目标端口号
    @return FT_SOCKET 成功返回一个创建好的套接字，失败返回FT_INVALID
*/
FT_SOCKET FT_UdpSocket(std::string ipv4, int port)
{
#ifdef WIN32  // 如果定义了WIN32宏
#ifndef LINUX // 没有定义LINUX宏,说明这个是在Windows平台使用
    // 初始化网络库
    WSADATA wd;
    WSAStartup(MAKEWORD(2, 2), &wd);
#endif
#endif
    FT_SOCKET sock;
    sock.socketId = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock.socketId == FT_INVALID)
        return FT_FAILED;

    sock.address.sin_family = AF_INET;
    sock.address.sin_port = htons(port);
    sock.address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock.socketId, (struct sockaddr *)&sock.address, sizeof(sock.address)) == -1)
    {
        FT_ShutDown(sock);
        return FT_FAILED;
    }

    sock.inetType = FT_UDP;
    sock.status = true;

    return sock;
}

// 关闭网络通道
/*
    @brief 关闭一个网络通道
    @param socket 需要关闭的网络通
*/
void FT_ShutDown(FT_SOCKET &socketId)
{
#ifdef LINUX
#ifndef WIN32
    close(socketId.socketId);
#endif
#endif

#ifdef WIN32
#ifndef LINUX
    WSACleanup(); // 解除网络库的初始化操作
    closesocket(socketId.socketId);
#endif
#endif

    socketId.socketId = FT_INVALID;
    socketId.status = false;
}

// 等待一个客户端连接
/*
    @brief 等待一个客户端连接
    @param socketId 需要等待客户端连接的服务端
    @return 成功返回一个客户端网络通道，失败返回FT_FAILED
*/
FT_SOCKET FT_Accept(FT_SOCKET &socketId)
{
    FT_SOCKET newSockId;
    while (1)
    {
        FT_SOCKLEN length = sizeof(newSockId.address);
        newSockId.socketId = accept(socketId.socketId, (struct sockaddr *)&newSockId.address, &length);
        if (newSockId.socketId == -1)
            return newSockId;
        break;
    }
    newSockId.inetType = FT_TCP;
    return newSockId;
}

// 通过指定的套接字发送一个网络数据包
// bool FT_Send(FT_SOCKET &socketId, FT_Package &data)
// {
//     if (socketId.inetType == FT_TCP)
//         if (send(socketId.socketId, data.pa_context, data.pa_size, 0) == -1)
//             return false;
//         else if (socketId.inetType == FT_UDP)
//             if (sendto(socketId.socketId, data.pa_context, data.pa_size, 0, (struct sockaddr *)&socketId.address, sizeof(socketId.address)) == -1)
//                 return false;
//     return true;
// }
// 第一种丢失协议头，而接收方需要通过协议头来解析

bool FT_Send(FT_SOCKET &socketId, FT_Package &data)
{
    if (socketId.inetType == FT_TCP)
    {
        int ret = send(socketId.socketId, (const char *)&data, sizeof(FT_Package), 0);
        return (ret == sizeof(FT_Package));
    }
    else if (socketId.inetType == FT_UDP)
    {
        int ret = sendto(socketId.socketId, (const char *)&data, sizeof(FT_Package), 0, (struct sockaddr *)&socketId.address, sizeof(socketId.address));
        return (ret == sizeof(FT_Package));
    }
    return false;
}

bool FT_Send(FT_SOCKET &socketId, FT_Action action, FT_DataType type, std::string data)
{
    FT_Package inetData;
    inetData.pa_number = 1;
    inetData.pa_action = action;
    inetData.pa_type = type;
    inetData.pa_status = FT_DS_ST;
    inetData.pa_size = data.length();
    strcpy(inetData.pa_context, data.c_str());

    return FT_Send(socketId, inetData);
}

// 通过指定的套接字获取一个网络数据包
// bool FT_Recv(FT_SOCKET &socketId, FT_Package &data)
// {
//     if (socketId.inetType == FT_TCP)
//         if (recv(socketId.socketId, (char *)&data, sizeof(data), 0) == -1)
//             return false;
//         else if (socketId.inetType == FT_UDP)
//             if (recvfrom(socketId.socketId, data.pa_context, data.pa_size, 0, nullptr, nullptr) == -1)
//                 return false;
//     return true;
// }

// 改完后：保证收发一致，都是整个结构体，协议头完整保留，能正确解析包的类型
bool FT_Recv(FT_SOCKET &socketId, FT_Package &data)
{
    if (socketId.inetType == FT_TCP)
    {
        int ret = recv(socketId.socketId, (char *)&data, sizeof(FT_Package), 0);
        return (ret == sizeof(FT_Package));
    }
    else if (socketId.inetType == FT_UDP)
    {
        FT_SOCKLEN len = sizeof(socketId.address);
        int ret = recvfrom(socketId.socketId, (char *)&data, sizeof(FT_Package), 0,
                           (struct sockaddr *)&socketId.address, &len);
        return (ret == sizeof(FT_Package));
    }
    return false;
}

bool FT_Recv(FT_SOCKET &socketId, std::string &data)
{
    FT_Package inetData;
    if (!FT_Recv(socketId, inetData))
        return false;
    data = inetData.pa_context;
    return true;
}