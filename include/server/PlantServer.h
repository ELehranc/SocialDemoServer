#ifndef __PlantServer_H__
#define __PlantServer_H__

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

using namespace muduo;
using namespace muduo::net;

class PlantServer
{

public:
    PlantServer(EventLoop *loop, const InetAddress &listenAddr, const string &name);

    void start();

private:
    // 连接回调
    void onConnection(const TcpConnectionPtr &conn);
    // 消息读写回调
    void onMessage(const TcpConnectionPtr &, Buffer *, Timestamp);

    TcpServer server_;
    EventLoop *loop_;
};
#endif