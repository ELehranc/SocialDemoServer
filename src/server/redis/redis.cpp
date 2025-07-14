#include "redis.h"
#include <iostream>

Redis::Redis() : publish_context_(nullptr), subscribe_context_(nullptr)
{
}

Redis::~Redis()
{
    if (publish_context_ != nullptr)
    {
        redisFree(publish_context_);
    }
    if (subscribe_context_ != nullptr)
    {
        redisFree(subscribe_context_);
    }
}

// 连接redis网络
bool Redis::connect()
{

    publish_context_ = redisConnect("127.0.0.1", 6379);
    if (publish_context_ == nullptr)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    subscribe_context_ = redisConnect("127.0.0.1", 6379);
    if (subscribe_context_ == nullptr)
    {
        std::cerr << "connect redis failed" << std::endl;
    }

    std::thread t([&]()
                  { observer_channel_message(); });
    t.detach();

    std::cout << "connect redis-server success!" << std::endl;
    return true;
}
// 像redis指定的通道channel发布消息
bool Redis::publish(int channel, std::string message)
{

    redisReply *reply = (redisReply *)redisCommand(publish_context_, "PUBLISH %d %s", channel, message.c_str());

    if (reply == nullptr)
    {
        std::cerr << "publish command failed!" << std::endl;
        return false;
    }

    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道unsubscribe消息取消订阅消息
bool Redis::subscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(subscribe_context_, "SUBSCRIBE %d", channel))
    {

        std::cerr << "subscribe command failed!" << std::endl;
        return false;
    }

    int done = 0;
    while (!done)
    {

        if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
        {
            std::cerr << "subscribe command failed!" << std::endl;
            return false;
        }
    }

    // redisGetReply
    return true;
}

// 向redis指定的通道unsubscribe消息取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(subscribe_context_, "UNSUBSCRIBE %d", channel))
    {

        std::cerr << "unsubscribe command failed!" << std::endl;
        return false;
    }

    int done = 0;
    while (!done)
    {

        if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
        {
            std::cerr << "unsubscribe command failed!" << std::endl;
            return false;
        }
    }

    // redisGetReply
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{

    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(subscribe_context_, (void **)&reply))
    {
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            std::cout << "Redis收到发送给id: " << atoi(reply->element[1]->str) << "的消息" << std::endl;
            notify_message_handler_(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }
    std::cerr << ">>>>>>>>>>>>>>>>>observer_channel_message quit<<<<<<<<<<<<<<<" << std::endl;
}

// 初始化像业务层上报通道消息的回调对象
void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    notify_message_handler_ = fn;
}