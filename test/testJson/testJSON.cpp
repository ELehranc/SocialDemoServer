#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;


string func1(){

    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "lcb";
    js["msg"]["zhang san"] = "hello world";
   
    string sendBuf = js.dump();
    return sendBuf;
}

void func2()
{
    json js;
    js["id"] = {1, 2, 3, 4, 5, 6};
    js["name"] = "zhang san";
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};
    cout << js << endl;
}

void func3(){

    json js;

    vector<int> res;
    res.push_back(1);
    res.push_back(2);
    res.push_back(3);

    js["list"] = res;

    unordered_map<string,int> map_;

    map_["美男子"] = 5;
    map_["小可爱"] = 6;

    js["人们"] = map_;

    cout << js << endl;
}

int main(){
    string recvBuf = func1();

    json jsbuf = json::parse(recvBuf);

    cout << jsbuf["msg_type"] << endl;
    cout << jsbuf["from"] << endl;
    cout << jsbuf["to"] << endl;
    auto msgjs  = jsbuf["msg"];
    cout << msgjs["zhang san"] << endl;
    return 0;
}