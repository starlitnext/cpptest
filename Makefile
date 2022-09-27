.PHONY: clean all pybind11_example asio_example

TOP := $(PWD)
BUILD_DIR := $(TOP)/build
EXTERNAL := $(TOP)/3rd
EXAMPLE := $(TOP)/example
PYTHON_INCLUDE := /usr/include/python2.7

CXX14 := g++-10 -std=c++14
CXX20 := g++-10 -std=c++20
CXXFLAGS := -g3 -O2 -Wall
SHARED_FLAGS = -fPIC --shared
PYBIND_CFLAGS = -I$(EXTERNAL)/pybind11/include -I$(PYTHON_INCLUDE)
ASIO_CFLAGS = -fcoroutines -DASIO_STANDALONE -I$(EXTERNAL)/asio/asio/include
ASIO_LDFLAGS = -lpthread

all: build_dir pybind11_example asio_example

build_dir:
	-mkdir $(BUILD_DIR)

# =================== pybind11 examples start =================== 
pybind11_example: $(BUILD_DIR)/pybind11_classes.so $(BUILD_DIR)/pybind11_helloworld.so

$(BUILD_DIR)/pybind11_classes.so: $(EXAMPLE)/pybind11_test/pybind11_classes.cpp
	$(CXX14) $(CXXFLAGS) $(SHARED_FLAGS) $(PYBIND_CFLAGS) $^ -o $@

$(BUILD_DIR)/pybind11_helloworld.so: $(EXAMPLE)/pybind11_test/pybind11_helloworld.cpp
	$(CXX14) $(CXXFLAGS) $(SHARED_FLAGS) $(PYBIND_CFLAGS) $^ -o $@

# =================== pybind11 examples end =================== 


# =================== asio examples start =================== 
asio_example: $(BUILD_DIR)/http_server $(BUILD_DIR)/co_echo_srv
asio_example: $(BUILD_DIR)/echo_cli

$(BUILD_DIR)/http_server: $(EXAMPLE)/asiotest/http_server.cpp
	$(CXX20) $(CXXFLAGS) $(ASIO_CFLAGS) $^ -o $@ $(ASIO_LDFLAGS)

$(BUILD_DIR)/echo_cli: $(EXAMPLE)/asiotest/echo_cli.cpp
	$(CXX20) $(CXXFLAGS) $(ASIO_CFLAGS) $^ -o $@ $(ASIO_LDFLAGS)

$(BUILD_DIR)/co_echo_srv: $(EXAMPLE)/asiotest/co_echo_srv.cpp
	$(CXX20) $(CXXFLAGS) $(ASIO_CFLAGS) -DASIO_ENABLE_HANDLER_TRACKING $^ -o $@ $(ASIO_LDFLAGS)

# =================== asio examples end =================== 

clean:
	rm -rf $(BUILD_DIR)