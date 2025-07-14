#include "PlantServer.h"
#include "json.hpp"
#include "PlantService.h"

#include <functional>

using json = nlohmann::json;

PlantServer::PlantServer(EventLoop *loop, const InetAddress &listenAddr, const string &name) : server_(loop, listenAddr, name), loop_(loop)
{

    server_.setMessageCallback(std::bind(&PlantServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    server_.setConnectionCallback(std::bind(&PlantServer::onConnection, this, std::placeholders::_1));

    server_.setThreadNum(4);
}

void PlantServer::start()
{
    server_.start();
}
// 连接回调
void PlantServer::onConnection(const TcpConnectionPtr &conn)
{
    // 用户断开连接
    if (!conn->connected())
    {
        PlantService::getInstance()->clientCloseException(conn);
        conn->shutdown();
    }
}
// 消息读写回调
void PlantServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{

    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化

    try{
        json js = json::parse(buf);

        if(!js.contains("msgid") || !js["msgid"].is_number()){
            LOG_ERROR << "Invalid message format: missing or invalid 'msgid'";
            conn->shutdown();
            return;
        }

        // 目的：解耦网络模块的代码和业务模块的代码
        // 通过js["msgid"] 获取一个业务处理器handler -> conn js time
        auto msghandler = PlantService::getInstance()->getHandler(js["msgid"].get<int>());
        msghandler(conn, js, time);
    }catch(const json::parse_error& e){
        LOG_ERROR << "JSON parse error: "<< e.what();
        LOG_ERROR << "iNVALID JSON: " << buf;
        conn->shutdown();
    }
}
