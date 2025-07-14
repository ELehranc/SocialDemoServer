#include "PlantService.h"
#include "public.h"

#include <vector>

// 获取单例接口
PlantService *PlantService::getInstance()
{
    static PlantService instance;
    return &instance;
}
// 路由注册
PlantService::PlantService()
{
    // 登陆业务
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&PlantService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    // 注册业务
    msgHandlerMap_.insert({REG_MSG, std::bind(&PlantService::regist, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    msgHandlerMap_.insert({DEFAULT, std::bind(&PlantService::defaultHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    // 一对一消息业务
    msgHandlerMap_.insert({ONE_CHAT_MSG, std::bind(&PlantService::oneChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    // 添加好友
    msgHandlerMap_.insert({ADD_FRIEND_MSG, std::bind(&PlantService::addFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    // 创建群组
    msgHandlerMap_.insert({CREAT_GROUP_MSG, std::bind(&PlantService::createGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    // 加入群
    msgHandlerMap_.insert({ADD_GROUP_MSG, std::bind(&PlantService::addGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    // 群消息
    msgHandlerMap_.insert({GROUP_CHAT_MSG, std::bind(&PlantService::groupChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    // 登出
    msgHandlerMap_.insert({LOGIN_OUT_MSG, std::bind(&PlantService::loginout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    if (redis_.connect())
    {

        redis_.init_notify_handler(std::bind(&PlantService::handleRedisSubscribeMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
}

// 服务器异常，业务重置方法
void PlantService::rest()
{
    // 所有onlien用户集体下线
    userModel_.resetState();
}

void PlantService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp &time)
{
    int userid = js["id"].get<int>();

    auto it = userConnMap_.find(userid);
    {
        std::lock_guard<std::mutex> lock(connMutex_);
        if (it != userConnMap_.end())
        {
            userConnMap_.erase(it);
        }
    }
    User user(userid, "", "", "offline");
    userModel_.updateState(user);

    // 退出id之后，在redis中取消订阅该用户的消息，消息会直接进入离线消息
    redis_.unsubscribe(userid);

    json response;
    response["msgid"] = LOGIN_OUT_MSG;

    conn->send(response.dump());
}

MsgHandler PlantService::getHandler(int msgId)
{
    auto it = msgHandlerMap_.find(msgId);
    if (it == msgHandlerMap_.end())
    {
        return msgHandlerMap_[DEFAULT];
    }
    else
    {
        return it->second;
    }
}
// 处理登陆业务
void PlantService::login(const TcpConnectionPtr &conn, json &js, Timestamp &time)
{

    int id = js["id"];
    std::string pwd = js["password"];

    User user = userModel_.query(id);

    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            {
                std::lock_guard<std::mutex> lock(connMutex_);
                userConnMap_.insert({id, conn});
            }
            // 该用户登录，不允许重复登陆
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "用户已登录，不允许重复登陆";
            conn->send(response.dump());
        }
        else
        {
            // 登陆成功
            // 更新用户账号信息
            user.setState("online");
            userModel_.updateState(user);
            {
                std::lock_guard<std::mutex> lock(connMutex_);
                userConnMap_.insert({id, conn});
            }

            // 对所有向该id用户的消息，我都需要进行订阅channel(id)
            redis_.subscribe(id);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息，如果有的话集中发送
            std::vector<std::string> vec = offlineMsgModel_.query(user.getId());

            if (!vec.empty())
            {
                LOG_INFO << "存在离线消息，现在发送";
                response["offlinemsg"] = vec;
                // 读取完全部离线消息后，删除全部离线消息
                offlineMsgModel_.remove(user.getId());
            }
            else
            {
                LOG_INFO << "不存在离线消息";
            }

            // 查询该用户的好友信息并返回
            std::vector<User> friendsVec = friendModel_.query(user.getId());
            if (!friendsVec.empty())
            {

                std::vector<std::string> vec2;
                for (auto &user : friendsVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            std::vector<Group> groupVec = groupModel_.queryGroups(user.getId());
            if (!groupVec.empty())
            {

                std::vector<json> groupV;
                for (Group &group : groupVec)
                {
                    json gropjson;
                    gropjson["id"] = group.getId();
                    gropjson["groupname"] = group.getName();
                    gropjson["groupdesc"] = group.getDesc();

                    std::vector<json> userV;
                    std::vector<GroupUser> groupuserVec = group.getUsers();

                    for (GroupUser &guser : groupuserVec)
                    {
                        json userjson;
                        userjson["id"] = guser.getId();
                        userjson["name"] = guser.getName();
                        userjson["state"] = guser.getState();
                        userjson["role"] = guser.getRole();
                        userV.push_back(userjson);
                    }

                    gropjson["users"] = userV;
                    groupV.push_back(gropjson);
                }

                response["groups"] = groupV;
            }
            LOG_INFO << "登录消息： " << response.dump();
            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在，登陆失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
    }

    LOG_INFO << "do login service !!!";
}
// 处理注册业务
void PlantService::regist(const TcpConnectionPtr &conn, json &js, Timestamp &time)
{

    std::string name = js["name"];
    std::string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = userModel_.insert(user);

    if (state)
    {

        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "REGIST_FAILD";
        conn->send(response.dump());
    }

    LOG_INFO << " do rigist service !!!";
}

// 处理客户端异常退出
void PlantService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {

        std::lock_guard<std::mutex> lock(connMutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); it++)
        {

            if (it->second == conn)
            {
                user.setId(it->first);
                userConnMap_.erase(it);
                break;
            }
        }
    }

    redis_.unsubscribe(user.getId());

    if (user.getId() != -1)
    {
        user.setState("offline");
        userModel_.updateState(user);
    }
}

// 一对一消息业务
void PlantService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp &time)
{
    int toid = js["to"].get<int>();

    {
        std::lock_guard<std::mutex> lock(connMutex_);
        auto it = userConnMap_.find(toid);
        if (it != userConnMap_.end())
        {
            // toid在线
            LOG_INFO << "SENDING SUCCESS";
            it->second->send(js.dump());
            return;
        }

        User user = userModel_.query(toid);
        if (user.getState() == "online")
        {
            redis_.publish(toid, js.dump());
            return;
        }

        offlineMsgModel_.insert(toid, js.dump());
    }
}

// 添加好友业务
void PlantService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp &time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息;
    friendModel_.insert(userid, friendid);
}

bool PlantService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp &time)
{
    int userid = js["id"].get<int>();

    std::string groupname = js["groupname"];
    std::string groupdesc = js["groupdesc"];
    Group group(-1, groupname, groupdesc);

    if (groupModel_.createGroup(group))
    {
        LOG_INFO << "群组创建成功";
        groupModel_.addGroup(userid, group.getId(), "creator");

        json response;
        char message[50];
        sprintf(message, "群组创建成功,群id: %d ", group.getId());
        response["msgid"] = ONE_CHAT_MSG;
        response["id"] = 0;
        response["name"] = "root";
        response["time"] = time.toString();
        response["msg"] = message;
        conn->send(response.dump());
        return true;
    }
    else
    {
        LOG_INFO << "群组创建失败";
        return false;
    }
}

// 加入群组
void PlantService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp &time)
{

    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    groupModel_.addGroup(userid, groupid, "normal");
}
// 群消息
void PlantService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp &time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    std::vector<Group> Groups = groupModel_.queryGroups(userid);
    for (auto &group : Groups)
    {
        if (group.getId() == groupid)
        {
            js["groupname"] = group.getName();
            break;
        }
    }

    std::vector<int> groupUsers = groupModel_.queryGroupUsers(userid, groupid);

    {
        std::lock_guard<std::mutex> lock(connMutex_);
        for (auto &uid : groupUsers)
        {
            if (uid == userid)
                continue;
            auto it = userConnMap_.find(uid);
            if (it != userConnMap_.end())
            {
                // 对方在线，直接发送消息
                it->second->send(js.dump());
            }
            else
            {

                User user = userModel_.query(uid);
                if (user.getState() == "online")
                {

                    redis_.publish(uid, js.dump());
                    return;
                }
                else
                {
                    // 对方不在线，存储离线消息
                    offlineMsgModel_.insert(uid, js.dump());
                }
            }
        }
    }
}
void PlantService::handleRedisSubscribeMessage(int userid, std::string msg)
{

    {
        std::lock_guard<std::mutex> lock(connMutex_);

        auto it = userConnMap_.find(userid);
        if (it != userConnMap_.end())
        {
            it->second->send(msg);
            return;
        }
    }

    offlineMsgModel_.insert(userid, msg);
}

void PlantService::defaultHandler(const TcpConnectionPtr &conn, json &js, Timestamp &time)
{
    LOG_ERROR << "msgid" << " not find handler! ";
}
