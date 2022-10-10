/*
 * @Author: silentwind vipxxq@foxmail.com
 * @Date: 2022-09-29 17:04:09
 * @LastEditors: silentwind vipxxq@foxmail.com
 * @LastEditTime: 2022-10-08 16:03:07
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
        std::cout << "Echo Called: " << request->msg() << std::endl;
        // msg 参数可以做进一步分发       
    }
};

#define ASIO_ENABLE_HANDLER_TRACKING

#include <array>
#include <iostream>
#include <memory>
#include <asio.hpp>
#include <asio/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>

using asio::buffer;
using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
namespace this_coro = asio::this_coro;
using namespace asio::experimental::awaitable_operators;
using std::chrono::steady_clock;
using namespace std::literals::chrono_literals;

constexpr auto use_no_throw_awaitable = asio::as_tuple(asio::use_awaitable);

RpcServer rpc_server;
EchoService echo_service;

awaitable<void> timeout(steady_clock::duration duration)
{
    asio::steady_timer timer(co_await this_coro::executor);
    timer.expires_after(duration);
    co_await timer.async_wait(use_no_throw_awaitable);
}

awaitable<void> proxy(tcp::socket client)
{
    std::array<char, 1024> data;

    for (;;)
    {
        auto result1 = co_await (
            client.async_read_some(buffer(data), use_no_throw_awaitable) ||
            timeout(5s)
        );
        if (result1.index() == 1)
            co_return;  // timed out

        auto [e1, n1] = std::get<0>(result1);
        if (e1)
            break;

        // dispatch data, n1
        rpc_server.input_data(data.data(), n1);
    }
}

awaitable<void> listen(tcp::acceptor& acceptor)
{
    for (;;)
    {
        auto [e1, client] = co_await acceptor.async_accept(use_no_throw_awaitable);
        if (e1)
            break;

        auto ex = client.get_executor();
        co_spawn(ex, proxy(std::move(client)), detached);
    }
}

int main(int argc, char const *argv[]) 
{
    rpc_server.add(&echo_service);

    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: EchoServer";
            std::cerr << " <listen_address> <listen_port>";
            return 1;
        }

        asio::io_context ctx;

        auto listen_endpoint = 
            *tcp::resolver(ctx).resolve(
                argv[1],
                argv[2],
                tcp::resolver::passive
            );
        
        tcp::acceptor acceptor(ctx, listen_endpoint);

        co_spawn(ctx, listen(acceptor), detached);

        ctx.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;

    return 0;
}