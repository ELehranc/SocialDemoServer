#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <atomic>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.h"
#include "user.h"
#include "public.h"

#include <sys/resource.h>

// 获取当前进程内存使用（KB）
size_t getMemoryUsage()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

using namespace std;
using json = nlohmann::json;

// 记录当前系统登录得用户信息
User g_currentUser;
// 记录当前登录用户的好友列表
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表
vector<Group> g_currentUserGroupList;

// 读取线程全局变量
std::thread g_readTask;
std::atomic<bool> g_running(false);

// 控制主菜单页面
bool isMainMenuRunning_ = false;

// 显示当前登录用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);

// 获取系统时间
string getCurrentTime();

// 平台主页面
void mainMenu(int clientfd);

int main(int argc, char **argv)
{

    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    for (;;)
    {

        cout << "==========================" << endl;
        cout << "1.login" << endl;
        cout << "2.register" << endl;
        cout << "3.quit" << endl;
        cout << "==========================" << endl;
        cout << "choice: ";
        int choice = 0;
        cin >> choice;
        cin.get();

        switch (choice)
        {
        case 1:
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get();
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error:" << request << endl;
            }
            else
            {
                char buffer[2048] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                cout << "接收消息： " << buffer << endl;
                if (len == -1)
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {

                    json responsejs = json::parse(buffer);
                    if (responsejs["errno"].get<int>() != 0)
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else
                    {
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        if (responsejs.contains("friends"))
                        {
                            g_currentUserFriendList.clear();
                            vector<string> vec = responsejs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        // 在解析 groups 前
                        std::cout << "Before parsing groups: " << getMemoryUsage() << " KB" << std::endl;

                        if (responsejs.contains("groups"))
                        {
                            try
                            {
                                g_currentUserGroupList.clear();
                                // vector<string> vec1 = responsejs["groups"];
                                for (json &groupjson : responsejs["groups"])
                                {
                                    //  json grpjs = json::parse(groupstr);
                                    Group group;
                                    group.setId(groupjson["id"].get<int>());
                                    group.setName(groupjson["groupname"]);
                                    group.setDesc(groupjson["groupdesc"]);

                                    //  vector<string> vec2 = grpjs["users"];
                                    for (auto &userjson : groupjson["users"])
                                    {

                                        GroupUser user;
                                        //  json userjs = json::parse(userstr);
                                        user.setId(userjson["id"].get<int>());
                                        user.setName(userjson["name"].get<string>());
                                        user.setState(userjson["state"].get<string>());
                                        user.setRole(userjson["role"].get<string>());
                                        group.getUsers().push_back(user);
                                    }

                                    g_currentUserGroupList.push_back(group);
                                }
                            }
                            catch (const json::exception &e)
                            {
                                cerr << "解析群组数据失败: " << e.what() << endl;
                                continue; // 跳过错误项
                            }
                        }
                        // 显示登录用户的基本信息
                        showCurrentUserData();

                        if (responsejs.contains("offlinemsg"))
                        {

                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                int msgType = js["msgid"].get<int>();
                                if (ONE_CHAT_MSG == msgType)
                                {
                                    cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                                }
                                else
                                {
                                    cout << js["time"].get<string>() << "[" << js["groupname"] << "]: " << "[" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                                }
                            }
                        }
                        // 登陆成功，启动接收线程负责接收数据;
                        g_running = true;
                        g_readTask = std::thread(readTaskHandler, clientfd);
                        g_readTask.detach();
                        isMainMenuRunning_ = true;
                        // 进主菜单页面
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};

            cout << "username: ";
            cin.getline(name, 50);
            cout << "userpassword: ";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error: " << request << endl;
            }
            else
            {

                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {

                    json responsejs = json::parse(buffer);
                    if (responsejs["errno"].get<int>() != 0)
                    {
                        cerr << name << "is already exist, register error!" << endl;
                    }
                    else
                    {
                        cout << name << "register success, userid is " << responsejs["id"] << ", do not forget it" << endl;
                    }
                }
            }
        }
        break;
        case 3:
        {
            close(clientfd);
            exit(0);
        }
        break;
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }

    return 0;
}

void showCurrentUserData()
{
    cout << "==============================login user==============================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "------------------------------friend list-----------------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "------------------------------group list-------------------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << "群消息：" << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << "------" << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "=======================================================================" << endl;
}

// 接收线程
void readTaskHandler(int clientfd)
{

    while (g_running)
    {

        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        int msgType = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgType)
        {
            cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (GROUP_CHAT_MSG == msgType)
        {
            cout << js["time"].get<string>() << "[" << js["groupname"] << "]: " << "[" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if(LOGIN_OUT_MSG == msgType){
            break;
        }
    }
    cout << "接收线程结束" << endl;
}
void help(int fd = 0, string str = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);
string getCurrentTime();

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一消息,格式chat:friendid:message"},
    {"addfriend", "添加好友,格式addfriend:friendid"},
    {"creategroup", "创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组,格式addgroup:groupid"},
    {"groupchat", "群聊,格式groupchat:groupid:message"},
    {"loginout", "注销,格式loginout"}};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主页面
void mainMenu(int clientfd)
{

    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning_)
    {

        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");
        if (idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }

        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {

            cerr << "invalid input command!" << endl;
            continue;
        }

        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

void help(int, string)
{

    cout << "show command list >>> " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

void addfriend(int clientfd, string str)
{

    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {

        cerr << "send addfriend msg error " << endl;
    }
}

void chat(int clientfd, string str)
{

    int idx = str.find(":");
    if (idx == string::npos)
    {
        cerr << "chat command invalid! ex => chat:friendid:message" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["to"] = friendid;
    js["name"] = g_currentUser.getName();
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send chat msg error => " << buffer << endl;
    }
}
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == string::npos)
    {
        cerr << "creategroup command invalid! ex => creategroup:groupname:groupdesc" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1);
    json js;
    js["msgid"] = CREAT_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send creatgroup msg error => " << buffer << endl;
    }
}
void addgroup(int clientfd, string str)
{

    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;

    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addgroup msg error => " << buffer << endl;
    }
}
void groupchat(int clientfd, string str)
{

    int idx = str.find(":");
    if (idx == string::npos)
    {

        cerr << "groupchat command invalid! ex => groupchat:groupid:message" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send groupchat msg error => " << buffer << endl;
    }
}
void loginout(int clientfd, string str)
{

    json js;
    js["msgid"] = LOGIN_OUT_MSG;
    js["id"] = g_currentUser.getId();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);

    if (len == -1)
    {
        cerr << "send groupchat msg error => " << buffer << endl;
    }
    else
    {
        isMainMenuRunning_ = false;
    }

    if (g_readTask.joinable())
    {
        g_running = false;
        g_readTask.join();
    }
}
string getCurrentTime()
{
    // 获取当前系统时间（UTC 时间戳）
    auto now = std::chrono::system_clock::now();
    // 转换为 time_t 类型（秒级精度）
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    // 转换为本地时间并格式化
    struct tm *local_time = std::localtime(&now_time);
    if (local_time == nullptr)
    {
        return ""; // 转换失败返回空
    }

    // 格式化输出为 "YYYY-MM-DD HH:MM:SS"
    char time_str[20];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);

    return std::string(time_str);
}