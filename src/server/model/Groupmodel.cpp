#include "Groupmodel.h"
#include "db.h"
// 创建群组
bool GroupModel::createGroup(Group &group)
{

    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT into AllGroup(groupname, groupdesc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取到主键id,设置到group当中;
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}
// 加入群组
void GroupModel::addGroup(int userid, int groupid, std::string role)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT into GroupUser(groupid,userid,grouprole) values(%d, %d, '%s')",
            groupid, userid, role.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 查询用户所在群组信息
std::vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT a.id,a.groupname,a.groupdesc FROM AllGroup a INNER JOIN GroupUser b ON a.id = b.groupid WHERE b.userid = %d", userid);

    MySQL mysql;
    std::vector<Group> vec;
    if (mysql.connect())
    {

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                vec.push_back(group);
            }

            mysql_free_result(res);
        }
    }

    for (auto &group : vec)
    {

        sprintf(sql, "SELECT a.id, a.name ,a.state ,b.grouprole FROM User a INNER JOIN GroupUser b ON a.id = b.userid WHERE b.groupid = %d", group.getId());

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {

                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }

            mysql_free_result(res);
        }
    }

    return vec;
}

// 根据指定的groupid查询群组用户id列表，除userid自己
std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};

    sprintf(sql, "SELECT userid FROM GroupUser WHERE groupid = %d AND userid !=%d", groupid, userid);

    MySQL mysql;
    std::vector<int> vec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);

        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                int userid = atoi(row[0]);
                vec.push_back(userid);
            }
            mysql_free_result(res);
        }
    }

    return vec;
}