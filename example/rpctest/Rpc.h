/*
 * @Author: silentwind vipxxq@foxmail.com
 * @Date: 2022-09-29 17:03:07
 * @LastEditors: silentwind vipxxq@foxmail.com
 * @LastEditTime: 2022-09-30 18:01:30
 * @FilePath: /cpptest/example/rpctest/Rpc.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "RpcMeta.pb.h"
#include "google/protobuf/service.h"
#include "google/protobuf/stubs/callback.h"
#include "asio.hpp"
#include "MemoryBitStream.hpp"

struct RpcHeader
{
    std::string service_name;
    std::string method_name;
    int32_t data_size = 3;
};

class RpcChannel
    : public ::google::protobuf::RpcChannel
{
public:
    RpcChannel(OutputMemoryBitStream& ostream) :
        ostream_(ostream)
    {
    }

    virtual void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                          ::google::protobuf::RpcController* /*controller*/, 
                          const ::google::protobuf::Message* request,
                          ::google::protobuf::Message* /*response*/, 
                          ::google::protobuf::Closure*)
    {
        request->SerializeToString(&serialize_data_);
        RpcHeader header{ method->service()->name(), method->name(), serialize_data_.size() };

        // TODO 需要发送出去的数据包
        // rpc_meta.size() + rpc_meta + request
        // 读取流程为先读取rpc_meta.size()
    }

private:
    // std::shared_ptr<asio::ip::tcp::socket> sock_;
    std::string serialize_data_;
    std::string serialize_header_;
    OutputMemoryBitStream& ostream_;
};

class RpcServer
{
public:
    void add(::google::protobuf::Service* service)
    {
        ServiceInfo service_info;
        service_info.service = service;
        service_info.sd = service->GetDescriptor();
        for (int i = 0; i < service_info.sd->method_count(); ++i)
        {
            service_info.mds[service_info.sd->method(i)->name()] = service_info.sd->method(i);
        }
        services_[service_info.sd->name()] = service_info;
    }

    // 数据包输入
    void input_data(const char* in_data, size_t size)
    {
        rpc::RpcMeta meta;
        meta.ParseFromArray(in_data, size);

        dispatch_msg(
            meta.service_name(),
            meta.method_name(),
            ,
        );
    }

private:
    void dispatch_msg(
        const std::string& service_name,
        const std::string& method_name,
        const char* serialized_data
    )
    {
        auto service = services_[service_name].service;
        auto md = services_[service_name].mds[method_name];

        auto recv_msg = service->GetRequestPrototype(md).New();
        recv_msg->ParseFromString(serialized_data);

        // 调用service真正的方法
        service->CallMethod(md, nullptr, recv_msg, nullptr, nullptr);
    }

private:
    struct ServiceInfo {
        ::google::protobuf::Service* service;
        const ::google::protobuf::ServiceDescriptor* sd;
        std::map<std::string, const ::google::protobuf::MethodDescriptor*> mds;
    };

    std::map<std::string, ServiceInfo> services_;
};
