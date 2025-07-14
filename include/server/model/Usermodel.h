#ifndef USER_MODEL_H
#define USER_MODEL_H

#include "user.h"

// User表的数据操作类
class UserModel{

public:
    // User表的增加方法
    bool insert(User &user);
    User query(int id);
    bool updateState(User &user);

    // 重置用户的状态信息
    void resetState();

private:
};

#endif 