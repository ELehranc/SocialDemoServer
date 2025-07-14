#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo::net;
using namespace muduo;
class PlantServer
{

public:
    PlantServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg) : server_(loop, listenAddr, nameArg), loop_(loop)
    {
        server_.setMessageCallback(bind(&PlantServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        server_.setConnectionCallback(bind(&PlantServer::onConnection, this, std::placeholders::_1));

        server_.setThreadNum(4);
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " STATE:online " << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " STATE:offline " << endl;
            conn->shutdown();
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp timestamp)
    {
        string buff = buf->retrieveAllAsString();
        cout << "recv data : " << buff << " time: " << timestamp.toString() << endl;
        conn->send(buff);
    }

    TcpServer server_;
    EventLoop *loop_;
};

int main()
{

    EventLoop loop;
    InetAddress addr("127.0.0.1", 8080);
    PlantServer server(&loop, addr, "charserver");

    server.start();
    loop.loop();

    return 0;
}
