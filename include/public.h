#ifndef __PUBLIC_H__
#define __PUBLIC_H__

enum EnMsgType
{
    LOGIN_MSG = 1,  // 登陆消息 1
    LOGIN_MSG_ACK,  // 登陆响应
    REG_MSG,        // 注册消息 3
    REG_MSG_ACK,    // 注册响应
    ONE_CHAT_MSG,   // 一对一消息 5
    ADD_FRIEND_MSG, // 添加好友 6

    CREAT_GROUP_MSG, // 创建群组 7
    ADD_GROUP_MSG,   // 加入群组 8
    GROUP_CHAT_MSG,  // 多对多 9

    LOGIN_OUT_MSG, // 登出消息 10

    DEFAULT
};

#endif