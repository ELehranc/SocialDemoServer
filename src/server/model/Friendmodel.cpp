#include "Friendmodel.h"

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{

    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO Friend(userid, friendid) values(%d, %d)", userid, friendid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 返回用户好友列表
std::vector<User> FriendModel::query(int userid)
{

    // 组装sql语句
    char sql[1024] = {0};

    sprintf(sql, "SELECT a.id,a.name,a.state FROM User a INNER JOIN Friend b ON b.friendid = a.id WHERE b.userid = %d", userid);
    std::vector<User> friends;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;

            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);

                friends.push_back(user);
            }
            // 注意释放
            mysql_free_result(res);
        }
    }
    return friends;
}