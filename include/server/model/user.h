#ifndef USER_H
#define USER_H

#include <string>

// 匹配User表的ORM类
class User
{

public:
    // 添加无参构造函数
    User() : id_(-1), name_(""), password_(""), state_("offline") {}

    User(int id , std::string name , std::string pwd , std::string state) : id_(id), name_(name), password_(pwd), state_(state) {}

    void setId(int id) { id_ = id; }
    void setName(std::string name) { name_ = name; }
    void setPwd(std::string pwd) { password_ = pwd; }
    void setState(std::string state) { state_ = state; }

    int getId() { return id_; }
    std::string getName() { return name_; }
    std::string getPwd() { return password_; }
    std::string getState() { return state_; }

protected:
    int id_;
    std::string name_;
    std::string password_;
    std::string state_;
};

#endif