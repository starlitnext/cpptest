/*
 * @Author: silentwind vipxxq@foxmail.com
 * @Date: 2022-09-29 17:03:07
 * @LastEditors: silentwind vipxxq@foxmail.com
 * @LastEditTime: 2022-10-08 15:48:19
 */

#ifndef RPC_HPP
#define RPC_HPP

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

    void Serialize(OutputMemoryBitStream& ostream)
    {
        ostream.Write(service_name);
        ostream.Write(method_name);
        ostream.Write(data_size);
    }

    void Deserialize(InputMemoryBitStream& istream)
    {
        istream.Read(service_name);
        istream.Read(method_name);
        istream.Read(data_size);
    }
};

class RpcChannel
    : public ::google::protobuf::RpcChannel
{
public:
    RpcChannel()
    {
    }

    virtual void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                          ::google::protobuf::RpcController* /*controller*/, 
                          const ::google::protobuf::Message* request,
                          ::google::protobuf::Message* /*response*/, 
                          ::google::protobuf::Closure*)
    {
        ostream_.Reset();
        // 预留4个字节写包的长度
        ostream_.Skip(4);

        request->SerializeToString(&serialize_data_);
        RpcHeader header{ method->service()->name(), method->name(), serialize_data_.size() };
        header.Serialize(ostream_);
        ostream_.Write(serialize_data_);

        // 数据包格式：
        // 4 bytes len + x bytes payload
        // 包体大小写在包的最前面
        auto head = ostream_.GetHead();
        ostream_.SetHead(0);
        ostream_.Write(ostream_.GetByteLength(), 4 << 3);
        ostream_.SetHead(head);

        // TODO 网络发送
        ostream_.GetBufferPtr();
        // 这里应该拿到对应的connection，发送数据
        // 一个RpcChannel 与一个connection相对应
    }

private:
    std::string serialize_data_;
    OutputMemoryBitStream ostream_;
};

class RpcServer
{
public:
    RpcServer() :
        recv_buff(nullptr),
        capacity_(0),
        recv_size(0),
        need_size(4)
    {
        realloc_buffer(1024);
    }
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

    // 数据包输入，rpc分发的入口
    void input_data(const char* in_data, size_t size)
    {
        recv_size += size;
        if (recv_size > capacity_)
            realloc_buffer(std::max(recv_size, capacity_ * 2));
        ::memcpy(recv_buff + recv_size - size, in_data, size);

        istream.Reset(recv_buff, recv_size << 3);
        istream.Read(need_size, 4 << 3);
        if (recv_size - 4 < need_size)
            return;

        RpcHeader header;
        header.Deserialize(istream);
        std::string serialized_data;
        istream.Read(serialized_data);

        dispatch_msg(
            header.service_name,
            header.method_name,
            serialized_data
        );
    }

private:

    void dispatch_msg(
        const std::string& service_name,
        const std::string& method_name,
        const std::string& serialized_data
    )
    {
        auto service = services_[service_name].service;
        auto md = services_[service_name].mds[method_name];

        auto recv_msg = service->GetRequestPrototype(md).New();
        recv_msg->ParseFromString(serialized_data);

        // 调用service真正的方法
        service->CallMethod(md, nullptr, recv_msg, nullptr, nullptr);
    }

    void realloc_buffer(uint32_t in_capacity)
    {
        if (recv_buff == nullptr)
        {
            recv_buff = static_cast<char*>(std::malloc(in_capacity));
            ::memset(recv_buff, 0, in_capacity);
        }
        else
        {
            char* tmp_buffer = static_cast<char*>(std::malloc(in_capacity));
            ::memset(tmp_buffer, 0, in_capacity);
            ::memcpy(tmp_buffer, recv_buff, in_capacity);
            std::free(recv_buff);
            recv_buff = tmp_buffer;
        }

        capacity_ = in_capacity;
    }

private:
    struct ServiceInfo {
        ::google::protobuf::Service* service;
        const ::google::protobuf::ServiceDescriptor* sd;
        std::map<std::string, const ::google::protobuf::MethodDescriptor*> mds;
    };

    std::map<std::string, ServiceInfo> services_;
    InputMemoryBitStream istream;
    char *recv_buff;
    size_t capacity_;
    size_t recv_size;
    size_t need_size;
};

#endif // !RPC_HPP