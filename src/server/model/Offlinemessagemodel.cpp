#include "Offlinemessagemodel.h"
#include "db.h"

// 存储用户的离线消息;
void OfflineMsgModel::insert(int userid, std::string msg)
{

    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO OfflineMessage (userid,message) values(%d, '%s')", userid, msg.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
    return;
}
// 移除用户的离线消息
void OfflineMsgModel::remove(int userid)
{

    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "DELETE FROM OfflineMessage WHERE userid = %d", userid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
    return;
}

std::vector<std::string> OfflineMsgModel::query(int userid)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "SELECT message FROM OfflineMessage WHERE userid = %d", userid);

    std::vector<std::string> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;

            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);
            }

            mysql_free_result(res);
        }
    }
    return vec;
}
