#pragma once

#include "vm.h"

// struct Point2{
//     int x;
//     int y;
// };

// std::map<std::string_view, int> _Point2_members = {
//     {"x", offsetof(Point2, x)},
//     {"y", offsetof(Point2, y)},
// };

struct CType{
    PY_CLASS(c, type_)

    const char* name;       // must be a literal
    const int size;
    const int index;
    constexpr CType(const char name[], int size, int index=-1) : name(name), size(size), index(index) {}

    static void _register(VM* vm, PyVar mod, PyVar type){
        vm->bind_static_method<-1>(type, "__new__", CPP_NOT_IMPLEMENTED());

        vm->bind_method<0>(type, "__repr__", [](VM* vm, pkpy::Args& args) {
            CType& self = vm->py_cast<CType>(args[0]);
            StrStream ss;
            ss << "<c._type '" << self.name << "' (" << self.size*8 << " bits)>";
            return vm->PyStr(ss.str());
        });
    }
};

constexpr CType kCTypes[] = {
    CType("char_", sizeof(char), 0), CType("int_", sizeof(int), 1),
    CType("float_", sizeof(float), 2), CType("double_", sizeof(double), 3),
    CType("bool_", sizeof(bool), 4), CType("void_", 1, 5),
    CType("int8_", sizeof(int8_t), 6), CType("int16_", sizeof(int16_t), 7),
    CType("int32_", sizeof(int32_t), 8), CType("int64_", sizeof(int64_t), 9), 
    CType("uint8_", sizeof(uint8_t), 10), CType("uint16_", sizeof(uint16_t), 11),
    CType("uint32_", sizeof(uint32_t), 12), CType("uint64_", sizeof(uint64_t), 13),
    // use macro here to do extension
};

const int kCTypeCount = sizeof(kCTypes) / sizeof(CType);

constexpr int ctype(const char name[]){
    for(int k=0; k<kCTypeCount; k++){
        const char* i = kCTypes[k].name;
        const char* j = name;
        while(*i && *j && *i == *j) { i++; j++;}
        if(*i == *j) return k;
    }
    UNREACHABLE();
}

#define ctype_t(x) (kCTypes[ctype(x)])

struct Pointer{
    PY_CLASS(c, ptr_)

    void* ptr;
    CType _ctype;       // base type

    Pointer(void* ptr, CType _ctype) : ptr(ptr), _ctype(_ctype) {}

    Pointer operator+(i64 offset) const {
        return Pointer((int8_t*)ptr + offset * _ctype.size, _ctype);
    }

    Pointer operator-(i64 offset) const {
        return Pointer((int8_t*)ptr - offset * _ctype.size, _ctype);
    }

    static void _register(VM* vm, PyVar mod, PyVar type){
        vm->bind_static_method<-1>(type, "__new__", CPP_NOT_IMPLEMENTED());

        vm->bind_method<0>(type, "__repr__", [](VM* vm, pkpy::Args& args) {
            Pointer& self = vm->py_cast<Pointer>(args[0]);
            StrStream ss;
            ss << "<" << self._ctype.name << "* at " << (i64)self.ptr << ">";
            return vm->PyStr(ss.str());
        });

        vm->bind_method<1>(type, "__add__", [](VM* vm, pkpy::Args& args) {
            Pointer& self = vm->py_cast<Pointer>(args[0]);
            return vm->new_object<Pointer>(self + vm->PyInt_AS_C(args[1]));
        });

        vm->bind_method<1>(type, "__sub__", [](VM* vm, pkpy::Args& args) {
            Pointer& self = vm->py_cast<Pointer>(args[0]);
            return vm->new_object<Pointer>(self - vm->PyInt_AS_C(args[1]));
        });

        vm->bind_method<1>(type, "__eq__", [](VM* vm, pkpy::Args& args) {
            Pointer& self = vm->py_cast<Pointer>(args[0]);
            Pointer& other = vm->py_cast<Pointer>(args[1]);
            return vm->PyBool(self.ptr == other.ptr);
        });

        vm->bind_method<1>(type, "__ne__", [](VM* vm, pkpy::Args& args) {
            Pointer& self = vm->py_cast<Pointer>(args[0]);
            Pointer& other = vm->py_cast<Pointer>(args[1]);
            return vm->PyBool(self.ptr != other.ptr);
        });

        // https://docs.python.org/zh-cn/3/library/ctypes.html
        vm->bind_method<1>(type, "__getitem__", [](VM* vm, pkpy::Args& args) {
            Pointer& self = vm->py_cast<Pointer>(args[0]);
            i64 index = vm->PyInt_AS_C(args[1]);
            return (self+index).get(vm);
        });

        vm->bind_method<2>(type, "__setitem__", [](VM* vm, pkpy::Args& args) {
            Pointer& self = vm->py_cast<Pointer>(args[0]);
            i64 index = vm->PyInt_AS_C(args[1]);
            (self+index).set(vm, args[2]);
            return vm->None;
        });

        vm->bind_method<1>(type, "cast", [](VM* vm, pkpy::Args& args) {
            Pointer& self = vm->py_cast<Pointer>(args[0]);
            CType& ctype = vm->py_cast<CType>(args[1]);
            return vm->new_object<Pointer>(self.ptr, ctype);
        });
    }

    template<typename T>
    inline T& ref() noexcept { return *reinterpret_cast<T*>(ptr); }

    template<typename TP>
    inline TP cast() noexcept {
        static_assert(std::is_pointer_v<TP>);
        return reinterpret_cast<TP>(ptr);
    }

    PyVar get(VM* vm){
        switch(_ctype.index){
            case ctype("char_"): return vm->PyInt(ref<char>());
            case ctype("int_"): return vm->PyInt(ref<int>());
            case ctype("float_"): return vm->PyFloat(ref<float>());
            case ctype("double_"): return vm->PyFloat(ref<double>());
            case ctype("bool_"): return vm->PyBool(ref<bool>());
            case ctype("void_"): vm->ValueError("cannot get void*"); break;
            case ctype("int8_"): return vm->PyInt(ref<int8_t>());
            case ctype("int16_"): return vm->PyInt(ref<int16_t>());
            case ctype("int32_"): return vm->PyInt(ref<int32_t>());
            case ctype("int64_"): return vm->PyInt(ref<int64_t>());
            case ctype("uint8_"): return vm->PyInt(ref<uint8_t>());
            case ctype("uint16_"): return vm->PyInt(ref<uint16_t>());
            case ctype("uint32_"): return vm->PyInt(ref<uint32_t>());
            case ctype("uint64_"): return vm->PyInt(ref<uint64_t>());
            // use macro here to do extension
            default: UNREACHABLE();
        }
        return vm->None;
    }

    void set(VM* vm, const PyVar& val){
        switch(_ctype.index){
            case ctype("char_"): ref<char>() = vm->PyInt_AS_C(val); break;
            case ctype("int_"): ref<int>() = vm->PyInt_AS_C(val); break;
            case ctype("float_"): ref<float>() = vm->PyFloat_AS_C(val); break;
            case ctype("double_"): ref<double>() = vm->PyFloat_AS_C(val); break;
            case ctype("bool_"): ref<bool>() = vm->PyBool_AS_C(val); break;
            case ctype("void_"): vm->ValueError("cannot set void*"); break;
            case ctype("int8_"): ref<int8_t>() = vm->PyInt_AS_C(val); break;
            case ctype("int16_"): ref<int16_t>() = vm->PyInt_AS_C(val); break;
            case ctype("int32_"): ref<int32_t>() = vm->PyInt_AS_C(val); break;
            case ctype("int64_"): ref<int64_t>() = vm->PyInt_AS_C(val); break;
            case ctype("uint8_"): ref<uint8_t>() = vm->PyInt_AS_C(val); break;
            case ctype("uint16_"): ref<uint16_t>() = vm->PyInt_AS_C(val); break;
            case ctype("uint32_"): ref<uint32_t>() = vm->PyInt_AS_C(val); break;
            case ctype("uint64_"): ref<uint64_t>() = vm->PyInt_AS_C(val); break;
            // use macro here to do extension
            default: UNREACHABLE();
        }
    }
};

struct StructMemberInfo {
    int offset;
    CType type;
};

struct StructMetaInfo {
    Str name;
    std::map<std::string_view, StructMemberInfo> members;
};

struct Struct {
    PY_CLASS(c, struct_)

    const StructMetaInfo* info;
    int8_t* _data;      // store any `struct`

    Struct(const StructMetaInfo* info, int8_t* data) : info(info), _data(data) {}
    ~Struct(){ delete[] _data; }

    int8_t* address(std::string_view name){
        auto it = info->members.find(name);
        if(it == info->members.end()) return nullptr;
        return _data + it->second.offset;
    }

    static void _register(VM* vm, PyVar mod, PyVar type){
        vm->bind_static_method<-1>(type, "__new__", CPP_NOT_IMPLEMENTED());

        vm->bind_method<0>(type, "__repr__", [](VM* vm, pkpy::Args& args) {
            Struct& self = vm->py_cast<Struct>(args[0]);
            StrStream ss;
            ss << "<c._struct '" << self.info->name << "'>";
            return vm->PyStr(ss.str());
        });

#define MEMBER_LOOKUP() \
    Struct& self = vm->py_cast<Struct>(args[0]);            \
    std::string_view name = vm->PyStr_AS_C(args[1]);        \
    auto it = self.info->members.find(name);                \
    if(it == self.info->members.end()){                     \
        vm->AttributeError(args[0], name.data());           \
        return vm->None;                                    \
    }                                                       \
    const StructMemberInfo& info = it->second;              \
    Pointer p = Pointer(self._data+info.offset, info.type); \

        vm->bind_method<1>(type, "__getattr__", [](VM* vm, pkpy::Args& args) {
            MEMBER_LOOKUP()
            return p.get(vm);
        });

        vm->bind_method<2>(type, "__setattr__", [](VM* vm, pkpy::Args& args) {
            MEMBER_LOOKUP()
            p.set(vm, args[2]);
            return vm->None;
        });

#undef MEMBER_LOOKUP
    }
};

void add_module_c(VM* vm){
    PyVar mod = vm->new_module("c");
    PyVar ptr_t = vm->register_class<Pointer>(mod);
    vm->register_class<CType>(mod);
    vm->register_class<Struct>(mod);

    for(int i=0; i<kCTypeCount; i++){
        vm->setattr(mod, kCTypes[i].name, vm->new_object<CType>(kCTypes[i]));
    }
    vm->setattr(mod, "nullptr", vm->new_object<Pointer>(nullptr, ctype_t("void_")));

    vm->bind_func<1>(mod, "malloc", [](VM* vm, pkpy::Args& args) {
        i64 size = vm->PyInt_AS_C(args[0]);
        return vm->new_object<Pointer>(malloc(size), ctype_t("void_"));
    });

    vm->bind_func<1>(mod, "free", [](VM* vm, pkpy::Args& args) {
        Pointer& self = vm->py_cast<Pointer>(args[0]);
        free(self.ptr);
        return vm->None;
    });

    vm->bind_func<1>(mod, "sizeof", [](VM* vm, pkpy::Args& args) {
        CType& ctype = vm->py_cast<CType>(args[0]);
        return vm->PyInt(ctype.size);
    });

    vm->bind_func<3>(mod, "memcpy", [](VM* vm, pkpy::Args& args) {
        Pointer& dst = vm->py_cast<Pointer>(args[0]);
        Pointer& src = vm->py_cast<Pointer>(args[1]);
        i64 size = vm->PyInt_AS_C(args[2]);
        memcpy(dst.ptr, src.ptr, size);
        return vm->None;
    });

    vm->bind_func<3>(mod, "memset", [](VM* vm, pkpy::Args& args) {
        Pointer& dst = vm->py_cast<Pointer>(args[0]);
        i64 val = vm->PyInt_AS_C(args[1]);
        i64 size = vm->PyInt_AS_C(args[2]);
        memset(dst.ptr, (int)val, size);
        return vm->None;
    });

    vm->bind_func<1>(mod, "strdup", [ptr_t](VM* vm, pkpy::Args& args) {
        if(is_type(args[0], vm->tp_str)){
            const Str& s = vm->PyStr_AS_C(args[0]);
            return vm->new_object<Pointer>(strdup(s.c_str()), ctype_t("char_"));
        }else if(is_type(args[0], OBJ_GET(Type, ptr_t))){
            Pointer& p = vm->py_cast<Pointer>(args[0]);
            return vm->new_object<Pointer>(strdup(p.cast<char*>()), ctype_t("char_"));
        }else{
            vm->TypeError("strdup() argument must be 'str' or 'c._ptr'");
            return vm->None;
        }
    });

    vm->bind_func<2>(mod, "strcmp", [](VM* vm, pkpy::Args& args) {
        Pointer& p1 = vm->py_cast<Pointer>(args[0]);
        Pointer& p2 = vm->py_cast<Pointer>(args[1]);
        return vm->PyInt(strcmp(p1.cast<char*>(), p2.cast<char*>()));
    });

    vm->bind_func<1>(mod, "strlen", [](VM* vm, pkpy::Args& args) {
        Pointer& p = vm->py_cast<Pointer>(args[0]);
        return vm->PyInt(strlen(p.cast<char*>()));
    });
}