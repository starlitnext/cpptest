/*
 * @Author: silentwind vipxxq@foxmail.com
 * @Date: 2022-09-29 17:04:09
 * @LastEditors: silentwind vipxxq@foxmail.com
 * @LastEditTime: 2022-09-30 11:36:25
 * @FilePath: /cpptest/example/rpctest/EchoServer.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <iostream>
#include <string>

#include "EchoService.pb.h"
#include "Rpc.h"

class EchoService 
    : public echo::EchoServer 
{
public:
    virtual void Echo(::google::protobuf::RpcController* controller,
                       const ::echo::EchoRequest* request,
                       ::echo::EchoResponse* response,
                       ::google::protobuf::Closure* done)
    {
        std::cout << request->msg() << std::endl;
        response->set_msg(
            std::string("I have received '") + request->msg() + std::string("'")
        );
        done->Run();
    }
};

int main() 
{
    RpcServer server;
    EchoService echo_service;
    server.add(&echo_service);
    server.start("127.0.0.1", 8000);

    return 0;
}