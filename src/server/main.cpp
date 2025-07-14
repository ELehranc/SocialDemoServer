#include "PlantServer.h"
#include "PlantService.h"
#include <iostream>

#include <signal.h>

// 处理服务器Ctrl+C结束服务后，重置User的登录状态
void restHandler(int)
{

    PlantService::getInstance()->rest();
    exit(0);
}

int main(int argc, char **argv)
{

    if (argc < 3)
    {
        std::cerr << "Error ex: ./PlantServer 127.0.0.1 6000" << std::endl;
        exit(-1);
    }

    const char *ip = argv[1];
    int port = std::stoi(argv[2]);

    signal(SIGINT, restHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    PlantServer server(&loop, addr, "PlantServer");
    server.start();
    loop.loop();

    return 0;
}