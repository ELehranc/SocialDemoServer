#ifndef GROUPUSER_H
#define GROUPUSER_H

#include <string>
#include "user.h"
class GroupUser : public User
{

public:
    GroupUser() : User(-1, "", "", "offline"), role_("") {}

    // 带参构造函数
    GroupUser(int id, const std::string &name, const std::string &state, const std::string &role)
        : User(id, name, "", state), role_(role) {} // 注意 password 设为空字符串

    void setRole(std::string role) { role_ = role; }
    std::string getRole() { return role_; }

private:
    std::string role_;
};

#endif