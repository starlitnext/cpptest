/*
 * @Author: silentwind vipxxq@foxmail.com
 * @Date: 2022-09-29 17:46:47
 * @LastEditors: silentwind vipxxq@foxmail.com
 * @LastEditTime: 2022-09-30 17:23:13
 */

#include <iostream>
#include "EchoService.pb.h"
#include "Rpc.h"

int main()
{
    RpcChannel channel;

    echo::EchoRequest request;
    echo::EchoResponse response;
    request.set_msg("hello, rpc.");

    echo::EchoServer_Stub stub(&channel);
    stub.Echo(nullptr, &request, nullptr, nullptr);

    return 0;
}