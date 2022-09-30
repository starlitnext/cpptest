
#include <iostream>
#include "EchoService.pb.h"
#include "Rpc.h"

int main()
{
    RpcChannel channel;
    channel.init("127.0.0.1", 8000);

    echo::EchoRequest request;
    echo::EchoResponse response;
    request.set_msg("hello, rpc.");

    echo::EchoServer_Stub stub(&channel);
    RpcController controller;
    stub.Echo(&controller, &request, &response, NULL);
    std::cout << "resp:" << response.msg() << std::endl;

    return 0;
}