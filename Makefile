.PHONY: clean all pybind11_example asio_example

TOP := $(PWD)
BUILD_DIR := $(TOP)/build
EXTERNAL := $(TOP)/3rd
EXAMPLE := $(TOP)/example
PYTHON_INCLUDE := /usr/include/python2.7

CXX14 := g++-10 -std=c++14
CXX20 := g++-10 -std=c++20
CXXFLAGS := -g3 -O2 -Wall -Wno-unused-result
SHARED_FLAGS = -fPIC --shared
PYBIND_CFLAGS = -I$(EXTERNAL)/pybind11/include -I$(PYTHON_INCLUDE)
ASIO_CFLAGS = -fcoroutines -DASIO_STANDALONE -I$(EXTERNAL)/asio/asio/include
ASIO_LDFLAGS = -lpthread
PROTOC := $(EXTERNAL)/protobuf_install/bin/protoc
PROTOBUF_CFLAGS = -I$(EXTERNAL)/protobuf_install/include
PROTOBUF_LDFLAGS = -L$(EXTERNAL)/protobuf_install/lib -Wl,-rpath=$(EXTERNAL)/protobuf_install/lib -lpthread -lprotobuf

all: build_dir pybind11_example asio_example

build_dir:
	@echo "create build dir"
	@mkdir -p $(BUILD_DIR)

# =================== pybind11 examples start =================== 
pybind11_example: $(BUILD_DIR)/pybind11_classes.so $(BUILD_DIR)/pybind11_helloworld.so

$(BUILD_DIR)/pybind11_classes.so: $(EXAMPLE)/pybind11_test/pybind11_classes.cpp
	$(CXX14) $(CXXFLAGS) $(SHARED_FLAGS) $(PYBIND_CFLAGS) $^ -o $@

$(BUILD_DIR)/pybind11_helloworld.so: $(EXAMPLE)/pybind11_test/pybind11_helloworld.cpp
	$(CXX14) $(CXXFLAGS) $(SHARED_FLAGS) $(PYBIND_CFLAGS) $^ -o $@

# =================== pybind11 examples end =================== 


# =================== asio examples start =================== 
# asio_example: $(BUILD_DIR)/http_server $(BUILD_DIR)/co_echo_srv
# asio_example: $(BUILD_DIR)/echo_cli

# $(BUILD_DIR)/http_server: $(EXAMPLE)/asiotest/http_server.cpp
# 	$(CXX20) $(CXXFLAGS) $(ASIO_CFLAGS) $^ -o $@ $(ASIO_LDFLAGS)

# $(BUILD_DIR)/echo_cli: $(EXAMPLE)/asiotest/echo_cli.cpp
# 	$(CXX20) $(CXXFLAGS) $(ASIO_CFLAGS) $^ -o $@ $(ASIO_LDFLAGS)

# $(BUILD_DIR)/co_echo_srv: $(EXAMPLE)/asiotest/co_echo_srv.cpp
# 	$(CXX20) $(CXXFLAGS) $(ASIO_CFLAGS) -DASIO_ENABLE_HANDLER_TRACKING $^ -o $@ $(ASIO_LDFLAGS)

ASIOTEST = $(TOP)/example/asiotest
ASIO_PROGRAMS := $(patsubst $(ASIOTEST)/%.cpp,%,$(wildcard $(ASIOTEST)/*.cpp))
asio_example: $(ASIO_PROGRAMS)
$(ASIO_PROGRAMS):
	@$(MAKE) build_dir
	$(CXX20) $(CXXFLAGS) $(ASIO_CFLAGS) $(ASIOTEST)/$@.cpp -o $(BUILD_DIR)/$@ $(ASIO_LDFLAGS)

# =================== asio examples end =================== 

# =================== protobuf examples begin =================== 

# 跑protobuf的例子时需要先 build protobuf，跑一次就行，不会提交git
build_protobuf:
	cd $(EXTERNAL)/protobuf && git submodule update --init --recursive && ./autogen.sh
	cd $(EXTERNAL) && mkdir -p protobuf_install
	cd $(EXTERNAL)/protobuf && ./configure --prefix=$(EXTERNAL)/protobuf_install
	cd $(EXTERNAL)/protobuf && make check && make install

# 使用动态链接
addressbook_write: $(EXAMPLE)/protobuf/addressbook_write.cpp $(EXAMPLE)/protobuf/addressbook.pb.cc
	$(PROTOC) -I=$(EXAMPLE)/protobuf --cpp_out=$(EXAMPLE)/protobuf $(EXAMPLE)/protobuf/addressbook.proto
	$(CXX20) $(CXXFLAGS) $(PROTOBUF_CFLAGS) $^ -o $(BUILD_DIR)/$@ $(PROTOBUF_LDFLAGS)

# 使用静态链接
addressbook_read: $(EXAMPLE)/protobuf/addressbook_read.cpp $(EXAMPLE)/protobuf/addressbook.pb.cc $(EXTERNAL)/protobuf_install/lib/libprotobuf.a
	$(PROTOC) -I=$(EXAMPLE)/protobuf --cpp_out=$(EXAMPLE)/protobuf $(EXAMPLE)/protobuf/addressbook.proto
	$(CXX20) $(CXXFLAGS) $(PROTOBUF_CFLAGS) $^ -o $(BUILD_DIR)/$@ -lpthread

gen_rpc_proto:
	$(PROTOC) -I=$(EXAMPLE)/rpctest/proto --cpp_out=$(EXAMPLE)/rpctest $(EXAMPLE)/rpctest/proto/EchoService.proto
	$(PROTOC) -I=$(EXAMPLE)/rpctest/proto --cpp_out=$(EXAMPLE)/rpctest $(EXAMPLE)/rpctest/proto/RpcMeta.proto

rpc_echo_cli: $(EXAMPLE)/rpctest/EchoClient.cpp $(EXAMPLE)/rpctest/RpcMeta.pb.cc $(EXAMPLE)/rpctest/EchoService.pb.cc $(EXTERNAL)/protobuf_install/lib/libprotobuf.a
	$(CXX20) $(CXXFLAGS) $(PROTOBUF_CFLAGS) $(ASIO_CFLAGS) $^ -o $(BUILD_DIR)/$@ -lpthread

rpc_echo_srv: $(EXAMPLE)/rpctest/EchoServer.cpp $(EXAMPLE)/rpctest/RpcMeta.pb.cc $(EXAMPLE)/rpctest/EchoService.pb.cc $(EXTERNAL)/protobuf_install/lib/libprotobuf.a
	$(CXX20) $(CXXFLAGS) $(PROTOBUF_CFLAGS) $(ASIO_CFLAGS) $^ -o $(BUILD_DIR)/$@ -lpthread

# =================== protobuf examples end =================== 


clean:
	rm -rf $(BUILD_DIR)