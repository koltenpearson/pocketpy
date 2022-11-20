#pragma once

#include "__stl__.h"
#include "str.h"

class PyObject;
typedef std::shared_ptr<PyObject> PyVar;
typedef PyVar PyVarOrNull;

class PyVarList: public std::vector<PyVar> {
    PyVar& at(size_t) = delete;

    inline void __checkIndex(size_t i) const {
        if (i >= size()){
            auto msg = "std::vector index out of range, " + std::to_string(i) + " not in [0, " + std::to_string(size()) + ")";
            throw std::out_of_range(msg);
        }
    }
public:
    PyVar& operator[](size_t i) {
        __checkIndex(i);
        return std::vector<PyVar>::operator[](i);
    }

    const PyVar& operator[](size_t i) const {
        __checkIndex(i);
        return std::vector<PyVar>::operator[](i);
    }

    // define constructors the same as std::vector
    using std::vector<PyVar>::vector;
};


class PyVarDict: public std::unordered_map<_Str, PyVar> {
    PyVar& at(const _Str&) = delete;

public:
    PyVar& operator[](const _Str& key) {
        return std::unordered_map<_Str, PyVar>::operator[](key);
    }

    const PyVar& operator[](const _Str& key) const {
        auto it = find(key);
        if (it == end()){
            auto msg = "std::unordered_map key not found, '" + key.str() + "'";
            throw std::out_of_range(msg);
        }
        return it->second;
    }

    // define constructors the same as std::unordered_map
    using std::unordered_map<_Str, PyVar>::unordered_map;
};


namespace pkpy {
    const size_t MAX_POOLING_N = 16;
    static std::deque<PyVar*>* _poolArgList = new std::deque<PyVar*>[MAX_POOLING_N];

    class ArgList {
        PyVar* _args = nullptr;
        size_t _size = 0;

        inline void __checkIndex(size_t i) const {
            if (i >= _size){
                auto msg = "pkpy:ArgList index out of range, " + std::to_string(i) + " not in [0, " + std::to_string(size()) + ")";
                throw std::out_of_range(msg);
            }
        }

        void __tryAlloc(size_t n){
            if(n >= MAX_POOLING_N || _poolArgList[n].empty()){
                this->_size = n;
                this->_args = new PyVar[n];
            }else{
                this->_args = _poolArgList[n].front();
                this->_size = n;
                _poolArgList[n].pop_front();
            }
        }

        void __tryRelease(){
            if(_size == 0 || _args == nullptr) return;
            if(_size >= MAX_POOLING_N){
                delete[] _args;
            }else{
                for(int i = 0; i < _size; i++) _args[i].reset();
                _poolArgList[_size].push_back(_args);
            }
        }

    public:
        ArgList(int n){
            __tryAlloc(n);
        }

        ArgList(const ArgList& other){
            __tryAlloc(other._size);
            for(int i=0; i<_size; i++){
                _args[i] = other._args[i];
            }
        }

        ArgList(ArgList&& other){
            this->_args = other._args;
            this->_size = other._size;
            other._args = nullptr;
            other._size = 0;
        }

        ArgList(PyVarList&& other){
            __tryAlloc(other.size());
            for(int i=0; i<_size; i++){
                _args[i] = std::move(other[i]);
            }
            other.clear();
        }

        ArgList(std::initializer_list<PyVar> args){
            __tryAlloc(args.size());
            int i = 0;
            for(auto& arg: args) this->_args[i++] = arg;
        }

        PyVar& operator[](size_t i){
            __checkIndex(i);
            return _args[i];
        }

        const PyVar& operator[](size_t i) const {
            __checkIndex(i);
            return _args[i];
        }

        // overload = for &&
        ArgList& operator=(ArgList&& other){
            if(this != &other){
                __tryRelease();
                this->_args = other._args;
                this->_size = other._size;
                other._args = nullptr;
                other._size = 0;
            }
            return *this;
        }

        size_t size() const {
            return _size;
        }

        ~ArgList(){
            __tryRelease();
        }
    };
}