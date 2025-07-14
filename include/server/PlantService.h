#ifndef PlantService_H
#define PlantService_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <mutex>
#include <muduo/base/Logging.h>

#include "json.hpp"
#include "Usermodel.h"
#include "Offlinemessagemodel.h"
#include "Friendmodel.h"
#include "Groupmodel.h"

#include "redis.h"

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 回调路由的类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp &time)>;

// 服务器服务类
class PlantService
{
public:
    // 获取单例接口
    static PlantService *getInstance();

    // 处理登陆业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp &time);
    // 处理注册业务
    void regist(const TcpConnectionPtr &conn, json &js, Timestamp &time);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    void rest();

    // 一对一消息业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp &time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp &time);

    // 创建群组
    bool createGroup(const TcpConnectionPtr &conn, json &js, Timestamp &time);
    // 加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp &time);
    // 群消息
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp &time);
    // 登出
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp &time);

    void handleRedisSubscribeMessage(int userid, std::string msg);

    void defaultHandler(const TcpConnectionPtr &conn, json &js, Timestamp &time);
    // 获取消息对应处理器
    MsgHandler getHandler(int msgId);

private:
    PlantService();

    // 路由
    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    // 数据操作类
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;

    // 存储在线用户的通信连接
    std::unordered_map<int, TcpConnectionPtr> userConnMap_;
    // 保证连接map的线程安全
    std::mutex connMutex_;

    Redis redis_;
};

#endif
