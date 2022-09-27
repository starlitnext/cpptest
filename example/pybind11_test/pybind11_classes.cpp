// g++ -O3 -Wall -shared -std=c++14 -fPIC -I../pybind11/include -I/usr/include/python2.7 classes.cpp -o classes.so

/**

from classes import *
d = Dog()
call_go(d)
h = Husky()
call_go(h)
class Cat(Anim):
    def go(self, n_times):
        return "meow!" * n_times
c = Cat()
call_go(c)
class Dachshund(Dog):
    def __init__(self, name):
        Dog.__init__(self)
        self.name = name
    def bark(self):
        return "yap!"
d = Dachshund("Tom")
call_go(d)

 */

#include <string>
#include <pybind11/pybind11.h>

class Anim {
public:
    virtual std::string go(int n_times) = 0;
    virtual std::string name() { return "unknown"; }
};

class Dog : public Anim {
public:
    std::string go(int n_times) override {
        std::string result;
        for (int i = 0; i < n_times; ++ i)
            result += bark() + " ";
        return result;
    }

    virtual std::string bark() { return "woof!"; }
};

class Husky : public Dog {};

template <class AnimBase = Anim>
class PyAnim : public AnimBase {
public:
    using AnimBase::AnimBase;   // Inehrit constructors
    std::string go(int n_times) override { PYBIND11_OVERRIDE_PURE(std::string, AnimBase, go, n_times); }
    std::string name() override { PYBIND11_OVERRIDE(std::string, AnimBase, name, ); }
};

template <class DogBase = Dog>
class PyDog : public PyAnim<DogBase> {
public:
    using PyAnim<DogBase>::PyAnim;
    std::string go(int n_times) override { PYBIND11_OVERRIDE(std::string, DogBase, go, n_times); }
    std::string bark() override { PYBIND11_OVERLOAD(std::string, DogBase, bark, ); }
};

std::string call_go(Anim *anim) {
    return anim->go(3);
}

PYBIND11_MODULE(pybind11_classes, m) 
{
    m.doc() = "pybind11 classes plugin";

    pybind11::class_<Anim, PyAnim<>>(m, "Anim")
        .def(pybind11::init<>())
        .def("go", &Anim::go);
    pybind11::class_<Dog, Anim, PyDog<>>(m, "Dog")
        .def(pybind11::init<>())
        .def("bark", &Dog::bark);
    pybind11::class_<Husky, Dog, PyDog<Husky>>(m, "Husky")
        .def(pybind11::init<>());
    
    m.def("call_go", &call_go);
}
