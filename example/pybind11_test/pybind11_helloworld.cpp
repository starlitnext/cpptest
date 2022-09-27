// g++ -O3 -Wall -shared -std=c++14 -fPIC -I../pybind11/include -I/usr/include/python2.7 helloworld.cpp -o helloworld.so

#include <string>
#include <pybind11/pybind11.h>

int add(int i, int j)
{
    return i + j;
}

class Pet {
public:
    Pet(const std::string &name) : name_(name) { }
    void setName(const std::string &name) { name_ = name; }
    const std::string& getName() const { return name_; }

    std::string signature;
private:
    std::string name_;
};

struct PolymorphicPet {
    virtual ~PolymorphicPet() = default;
};

struct PolymorphicDog : PolymorphicPet {
    std::string bark() const { return "woof!"; }
};

struct Overloaded {
    Overloaded(const std::string& name, int age) { }
    void set(int age) { age_ = age; }
    void set(const std::string& name) { name_ = name; }

    std::string name_;
    int age_;
};

struct Pet1
{
    enum Kind {
        Dog = 0,
        Cat
    };

    struct Attributes {
        float age = 0;
    };

    Pet1(const std::string& name, Kind type) : name(name), type(type) { }

    std::string name;
    Kind type;
    Attributes attr;
};

PYBIND11_MODULE(pybind11_helloworld, m) 
{
    m.doc() = "pybind11 helloworld plugin";

    // = 1 为函数默认值，可以不设置
    m.def("add", &add, "A function that adds two numbers",
        pybind11::arg("i") = 1, pybind11::arg("j") = 2);

    m.def("add1", &add, pybind11::arg("i") = 1, pybind11::arg("j") = 2);

    using namespace pybind11::literals;
    m.def("add2", &add, "i"_a = 1, "j"_a = 2);

    // 隐式转换
    m.attr("the_answer") = 42;
    // 显式转换
    pybind11::object world = pybind11::cast("World");
    m.attr("what") = world;

    // 只有设置了 dynamic_attr 才可以在脚本中添加一个动态属性
    pybind11::class_<Pet>(m, "Pet", pybind11::dynamic_attr())
        .def(pybind11::init<const std::string&>())
        .def_readwrite("signature", &Pet::signature)
        .def_property("name", &Pet::getName, &Pet::setName)
        // .def("setName", &Pet::setName)
        // .def("getName", &Pet::getName)
        .def("__repr__",
            [](const Pet& a) {
                return "<example.Pet named <'" + a.getName() + "'>";
            }
        );

    // 继承多态
    pybind11::class_<PolymorphicPet> pypet(m, "PolymorphicPet");
    // 和 pybind11::class_<PolymorphicDog>(m, "PolymorphicDog", pypet) 等效
    pybind11::class_<PolymorphicDog, PolymorphicPet>(m, "PolymorphicDog")
        .def(pybind11::init<>())
        .def("bark", &PolymorphicDog::bark);
    // 返回一个指向子类对象的父类指针
    // 父类至少需要有一个virtual函数，才能产生多态
    m.def("pet_store", [](){ return std::unique_ptr<PolymorphicPet>(new PolymorphicDog); });

    // 函数重载
    pybind11::class_<Overloaded>(m, "Overloaded")
        .def(pybind11::init<const std::string&, int>())
        // .def("set", static_cast<void (Overloaded::*)(int)>(&Overloaded::set), "Set age")
        // .def("set", static_cast<void (Overloaded::*)(const std::string&)>(&Overloaded::set), "Set name");
        .def("set", pybind11::overload_cast<int>(&Overloaded::set), "Set age")
        .def("set", pybind11::overload_cast<const std::string&>(&Overloaded::set), "Set name");

    // Enumerations and internal types
    pybind11::class_<Pet1> pet1(m, "Pet1");
    pet1.def(pybind11::init<const std::string &, Pet1::Kind>())
        .def_readwrite("name", &Pet1::name)
        .def_readwrite("type", &Pet1::type)
        .def_readwrite("attr", &Pet1::attr);
    pybind11::enum_<Pet1::Kind>(pet1, "Kind")
        .value("Dog", Pet1::Kind::Dog)
        .value("Cat", Pet1::Kind::Cat)
        .export_values();
    pybind11::class_<Pet1::Attributes>(pet1, "Attributes")
        .def(pybind11::init<>())
        .def_readwrite("age", &Pet1::Attributes::age);
}