#pragma once

#include <functional>
#include <thread>
#include <utility>

using std::function;
using std::thread;

namespace Shared{ namespace Util{

template<typename T>
class CallBack {
    

private:
    function<void()> _onThen = [](){};
    function<T()> _mainFunc = [](){};
    T _result;
    
public:
    CallBack(function<T()> func){
       _mainFunc = func;
    };
    
    ~CallBack() {
        //printf("CallBack::~CallBack p [%p]\n",this);
    };
    
    void then(function<void()> onThen){
        _onThen = onThen;
    };
    
    T getResult(){
        return _result;
    }
    
    void run(){
        _result = _mainFunc();
        _onThen();
    };
};

template<>
class CallBack<void> {

private:
    function<void()> _onThen = [](){};
    function<void()> _mainFunc = [](){};
    
public:
    CallBack(function<void()> func){
       _mainFunc = func;
    };
    
    ~CallBack() {
        //printf("CallBack::~CallBack p [%p]\n",this);
    };
    
    void then(function<void()> onThen){
        _onThen = onThen;
    };
    
    void run(){
        _mainFunc();
        _onThen();
    };

};

}}
