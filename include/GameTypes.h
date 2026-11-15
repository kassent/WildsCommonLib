#pragma once
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <reframework/API.hpp>

namespace System::Reflection {
    // TDB 原名与数值，来自 System/Reflection/TypeAttributes.lua；列出当前使用的标志。
    enum class TypeAttributes : int32_t {
        Class = 0x00,
        Interface = 0x20,
        Abstract = 0x80,
    };
}

namespace via::clr {
    // TDB 原名与数值，来自 via/clr/VMObjType.lua；表示 VM 对象的存储分类。
    enum class VMObjType : int32_t {
        Null = 0,
        Object = 1,
        Array = 2,
        String = 3,
        Delegate = 4,
        ValType = 5,
    };

    // TDB 原名与数值，来自 via/clr/ElementType.lua；是 CLR 类型编码，不是 TDB 索引。
    enum class ElementType : int32_t {
        End = 0,
        Void = 1,
        Boolean = 2,
        Char = 3,
        I1 = 4,
        U1 = 5,
        I2 = 6,
        U2 = 7,
        I4 = 8,
        U4 = 9,
        I8 = 10,
        U8 = 11,
        R4 = 12,
        R8 = 13,
        String = 14,
        Ptr = 15,
        ByRef = 16,
        ValueType = 17,
        Class = 18,
        Var = 19,
        Array = 20,
        GenericInst = 21,
        TypedByRef = 22,
        I = 24,
        U = 25,
        FNPtr = 27,
        Object = 28,
        SzArray = 29,
        MVar = 30,
        RE_Reqd = 31,
        RE_Opt = 32,
        Internal = 33,
        Max = 34,
        Modifier = 64,
        Stencil = 65,
        Pinned = 69,
        Enum = 85,
        Any = 127,
    };
}

namespace RE {
    using API = reframework::API;
    std::string narrow(std::wstring_view str);
    // 按显式长度将 UTF-8 转为 UTF-16，保留内嵌 NUL；空输入或转换失败返回空字符串。
    // flags 使用 MultiByteToWideChar 的转换标志，默认 0 保持既有替换行为；严格调用传 MB_ERR_INVALID_CHARS。
    std::wstring widen(std::string_view str, uint32_t flags = 0);

#pragma pack(push, 4)
    // MHWILDS TDB81 ABI overlay with the game builder's packed-field
    // semantics applied.
    struct RETypeImpl {
        int32_t nameOffset;                 // +0x00
        int32_t namespaceOffset;            // +0x04
        int32_t fieldSize;                  // +0x08
        int32_t staticFieldSize;            // +0x0C
        uint64_t unknown10Low : 17;         // +0x10, bits 0..16
        uint64_t numManagedVtable : 16;     // +0x10, bits 17..32
        uint64_t numMemberFields : 24;      // +0x10, bits 33..56
        uint64_t unknown10High : 7;         // +0x10, bits 57..63
        uint16_t numMemberMethods;          // +0x18
        int16_t numNativeEntries;           // +0x1A; ObjectInfo[0] + native slots
        int16_t interfaceId;                // +0x1C
        char unknown1E[0x12];               // +0x1E
    };

    // TDB81 uses REFramework's TDB74+ type-definition layout.
    struct RETypeDefinition {
        uint64_t index : 19;                       // +0x00, bits 0..18
        uint64_t parentTypeIndex : 19;             // +0x00, bits 19..37
        uint64_t declaringTypeIndex : 19;          // +0x00, bits 38..56
        uint64_t underlyingTypeIndex : 7;          // +0x00, bits 57..63；via::clr::ElementType，不是类型索引。

        uint64_t arrayTypeIndex : 19;              // +0x08, bits 0..18
        uint64_t elementTypeIndex : 19;            // +0x08, bits 19..37
        uint64_t implIndex : 18;                   // +0x08, bits 38..55
        uint64_t systemTypeIndex : 7;              // +0x08, bits 56..62
        uint64_t unknown0F : 1;                    // +0x08, bit 63

        uint32_t typeFlags;                        // +0x10
        uint32_t instanceSize;                     // +0x14
        uint32_t fqnHash;                          // +0x18
        uint32_t typeCrc;                          // +0x1C

        uint64_t defaultCtorIndex : 22;            // +0x20, bits 0..21
        uint64_t memberMethodIndex : 22;           // +0x20, bits 22..43
        uint64_t memberFieldIndex : 20;            // +0x20, bits 44..63

        uint32_t numMemberProperties : 12;         // +0x28, bits 0..11
        uint32_t memberPropertyIndex : 19;         // +0x28, bits 12..30
        uint32_t unknown2B : 1;                    // +0x28, bit 31
        uint32_t unknown2C : 26;                   // +0x2C, bits 0..25
        uint32_t objectType : 3;                   // +0x2C, bits 26..28；via::clr::VMObjType，与 ElementType 编码不同。
        uint32_t arrayRank : 3;                    // +0x2C, bits 29..31；Array 的维数。

        int64_t interfaceListOffset : 26;          // +0x30, bits 0..25
        int64_t genericsOffset : 26;               // +0x30, bits 26..51
        int64_t interfaceIndex : 12;               // +0x30, bits 52..63

        void* typeInfo;                            // +0x38, RETypeCLR*
        void** objectInfo;                         // +0x40, REObjectInfo*/ObjectInfo
        uint64_t unknown48;                        // +0x48

        uint32_t getInterfaceListOffset() const {
            return static_cast<uint32_t>(interfaceListOffset) & 0x03FFFFFF;
        }

        int32_t getInterfaceIndex() const {
            return static_cast<int32_t>(interfaceIndex);
        }
    };

    // Mirrors sdk::tdb81::TDB. Unused table pointers stay void* because
    // callers only need selected tables; pointer-sized ABI and offsets are exact.
    struct TDB {
        uint32_t magic;                            // +0x00
        uint32_t version;                          // +0x04
        uint32_t numTypes;                         // +0x08
        uint32_t typesStartOfGenericsProbably;     // +0x0C
        uint32_t unknown10;                        // +0x10
        uint32_t numMethods;                       // +0x14
        uint32_t numFields;                        // +0x18
        uint32_t numTypeImpl;                      // +0x1C
        uint32_t numFieldImpl;                     // +0x20
        uint32_t numMethodImpl;                    // +0x24
        uint32_t numPropertyImpl;                  // +0x28
        uint32_t numProperties;                    // +0x2C
        uint32_t numEvents;                        // +0x30
        uint32_t numParams;                        // +0x34
        uint32_t numAttributes;                    // +0x38
        int32_t numInitData;                       // +0x3C
        uint32_t numAttributes2;                   // +0x40
        uint32_t numInternStrings;                 // +0x44
        uint32_t numModules;                       // +0x48
        int32_t devEntry;                          // +0x4C
        int32_t appEntry;                          // +0x50
        uint32_t numStringPool;                    // +0x54
        uint32_t numBytePool;                      // +0x58
        uint32_t padding;                          // +0x5C
        void* modules;                             // +0x60, REModule*
        RETypeDefinition* types;                   // +0x68, RETypeDefinition (*)[]
        RETypeImpl* typeImpls;                     // +0x70, RETypeImpl (*)[]
        void* methods;                             // +0x78, REMethodDefinition (*)[]
        void* methodImpls;                         // +0x80, REMethodImpl (*)[]
        void* fields;                              // +0x88, REField (*)[]
        void* fieldImpls;                          // +0x90, REFieldImpl (*)[]
        void* properties;                          // +0x98, REProperty (*)[]
        void* propertyImpls;                       // +0xA0, REPropertyImpl (*)[]
        void* events;                              // +0xA8
        void* params;                              // +0xB0, REParameterDef (*)[]
        void* attributes;                          // +0xB8, REAttributeDef (*)[]
        void* initData;                            // +0xC0, int32_t (*)[]
        void* unk;                                 // +0xC8
        void* attributes2;                         // +0xD0, int32_t (*)[]
        void* stringPool;                          // +0xD8, char (*)[]
        void* bytePool;                            // +0xE0, uint8_t (*)[]
        void* internStrings;                       // +0xE8, int32_t (*)[]
    };

    struct REInterfaceList {
        uint32_t count;                    // +0x00
        uint32_t encodedTypes[1];          // +0x04, variable-length entries
    };
#pragma pack(pop)

    static_assert(sizeof(RETypeImpl) == 0x30);
    static_assert(offsetof(RETypeImpl, numMemberMethods) == 0x18);
    static_assert(offsetof(RETypeImpl, numNativeEntries) == 0x1A);
    static_assert(sizeof(RETypeDefinition) == 0x50);
    static_assert(offsetof(RETypeDefinition, typeInfo) == 0x38);
    static_assert(offsetof(RETypeDefinition, objectInfo) == 0x40);
    static_assert(sizeof(TDB) == 0xF0);
    static_assert(offsetof(TDB, typeImpls) == 0x70);
    static_assert(offsetof(REInterfaceList, encodedTypes) == 0x04);

    // MHWILDS VM partial layout; only the known prefix is represented.
    // This is the engine VM, not the per-call VMContext.
    struct VM {
        uint8_t unknown0000[0x35C0];
        void** runtimeTypes;               // +0x35C0, System.RuntimeType* table
        uint64_t runtimeTypeCount;          // +0x35C8, not TDB::numTypes
        uint8_t unknown35D0[0x48];
        TDB* tdb;                          // +0x3618
    };

    static_assert(offsetof(VM, runtimeTypes) == 0x35C0);
    static_assert(offsetof(VM, runtimeTypeCount) == 0x35C8);
    static_assert(offsetof(VM, tdb) == 0x3618);

    struct REObjectInfoLayout {
        uint32_t    tdbVersion{};
        uint32_t    typeImplIndex{};
        size_t      numInterfacePrefixEntries{};
        size_t      numNativeEntries{}; // ObjectInfo[0] + native functions
        size_t      numNativeSlots{};   // native functions only
        size_t      numManagedSlots{};
        size_t      numBlockQwords{};
        size_t      blockSize{};
        void**      blockStart{};
        void**      blockEnd{};
    };

#pragma pack(push, 1)
    class REObject
    {
    public:
        class REObjectInfo* info; //0x0000
    }; //Size: 0x0008
    static_assert(sizeof(REObject) == 0x8);

    class REManagedObject : public REObject
    {
    public:
        uint32_t referenceCount; //0x0008
        int16_t N000071AE; //0x000C
        char pad_000E[2]; //0x000E
    }; //Size: 0x0010
    static_assert(sizeof(REManagedObject) == 0x10);

    class REComponent : public REManagedObject
    {
    public:
        class REGameObject* ownerGameObject; //0x0010
        class REComponent* childComponent; //0x0018
        class REComponent* prevComponent; //0x0020
        class REComponent* nextComponent; //0x0028
    }; //Size: 0x0030
    static_assert(sizeof(REComponent) == 0x30);

    class SystemString : public REManagedObject
    {
    public:
        int32_t size; //0x0010
        wchar_t data[256]; //0x0014

        [[nodiscard]] std::string to_str() const { return narrow(data); }
        [[nodiscard]] std::wstring to_wstr() const { return data; }
        [[nodiscard]] std::wstring_view to_view() const { return data; }
        [[nodiscard]] API::ManagedObject* to_managed_object()  { return reinterpret_cast<API::ManagedObject*>(this); }
    }; //Size: 0x0214
    static_assert(sizeof(SystemString) == 0x214);

    // via.Prefab（布局 EnhancedOuterItem/prefab_creation_chain_analysis.md 二，
    // 尺寸 = TDB get_size 40 live 实证）：+0x10 原生 Resource*（createResource 产物）、
    // +0x18 System.String* 路径（引用计数自管：存入前需 add_ref，+0x18 非 GC 扫描字段）、
    // +0x20 standby（set_Standby 读写；原版 catalog 条目实际为 false——懒加载）
    class REPrefab : public REManagedObject
    {
    public:
        void* resource; //0x0010 原生 Resource*（非托管）
        SystemString* path; //0x0018 pfb 路径
        bool standby; //0x0020 资源常驻
        char pad_0021[7]; //0x0021
    }; //Size: 0x0028
    static_assert(sizeof(REPrefab) == 0x28);

    class PhysicsFilterInfo : public REManagedObject
    {
    public:
        uint32_t layer; //0x0010
        uint32_t group; //0x0014
        uint32_t subGroup; //0x0018
        uint32_t ignoreSubGroup; //0x001C
        uint32_t maskBits; //0x0020
        char pad_0024[12]; //0x0024
    }; //Size: 0x0030
    static_assert(sizeof(PhysicsFilterInfo) == 0x30);

    template <class T> class SystemArray {
    public:
        void* object_info;      // 0x0000
        uint32_t ref_count;     // 0x0008
        int32_t _0;             // 0x000C
        void* contained_type;   // 0x0010
        uint32_t _1;            // 0x0018
        uint32_t count;         // 0x001C
        T elements[1];          // 0x0020
    };

    class SystemGuid {
        public:
        uint32_t mData1; // 0x0
        uint16_t mData2; // 0x4
        uint16_t mData3; // 0x6
        union {
            uint64_t mData4L; // 0x8
            struct {
                uint8_t mData4_0; // 0x8
                uint8_t mData4_1; // 0x9
                uint8_t mData4_2; // 0xA
                uint8_t mData4_3; // 0xB
                uint8_t mData4_4; // 0xC
                uint8_t mData4_5; // 0xD
                uint8_t mData4_6; // 0xE
                uint8_t mData4_7; // 0xF
            };
        } mData4;

        SystemGuid() {
            std::memset(this, 0, sizeof(SystemGuid));
        }

        explicit SystemGuid(std::string_view str) {
            if (!from_str(str)) {
                std::memset(this, 0, sizeof(SystemGuid));
            }
        }
        [[nodiscard]] std::string to_str() const;
        [[nodiscard]] bool from_str(std::string_view str);
        [[nodiscard]] bool operator==(const SystemGuid& other) const;

    };
    static_assert(sizeof(SystemGuid) == 0x10);
#pragma pack(pop)


    struct SystemAction {
        struct DelegateInvocation {
            API::ManagedObject* object;
            void* func;
            API::Method* method;
        };
        void* object_info;      // 0x0000
        uint32_t ref_count;     // 0x0008
        int32_t _0;             // 0x000C
        uint32_t num_methods { 0 }; // 0x0010
        int32_t _1;             // 0x0014
        DelegateInvocation methods[1]; // 0x0018
    };
    static_assert(sizeof(SystemAction) == 0x30);


    template <class T> class REArray;

    template <class T> REArray<T>* to_re_array(API::ManagedObject* obj) {
        return reinterpret_cast<REArray<T>*>(obj);
    }

    template <class T> const REArray<T>* to_re_array(const API::ManagedObject* obj) {
        return reinterpret_cast<const REArray<T>*>(obj);
    }

    template <class T> API::ManagedObject* to_managed_object(T *obj) {
        return reinterpret_cast<API::ManagedObject*>(obj);
    }

    template <class T = API::ManagedObject*, class... Args>
    T call(API::ManagedObject* obj, std::string_view name, Args... args) {
        return obj->call<T>(name, API::get()->get_vm_context(), obj, args...);
    }

    template <class T = API::ManagedObject*, class... Args> T call(API::Method* method, Args... args) {
        return method->call<T>(API::get()->get_vm_context(), args...);
    }

    template <class... Args> std::string str_call(API::ManagedObject* obj, std::string_view name, Args... args) {
        const SystemString* res = obj->call<SystemString*>(name, API::get()->get_vm_context(), obj, args...);
        return res->to_str();
    }

    template <class... Args> std::string str_call(API::Method* method, Args... args) {
        const SystemString* res = method->call<SystemString*>(API::get()->get_vm_context(), args...);
        return res->to_str();
    }

    template <class T = uint32_t> std::string get_enum_name(std::string_view name, T value) {
        const auto& api = API::get();
        const auto type = api->tdb()->find_type(name);
        const auto inst = type->create_instance();

        *inst->get_field<T>("value__") = value;
        return call<SystemString*>(inst, "ToString()")->to_str();
    }

    template <class T = uint32_t> std::string get_enum_name(API::TypeDefinition* type, T value) {
        const auto inst = type->create_instance();
        *inst->get_field<T>("value__") = value;

        return call<SystemString*>(inst, "ToString()")->to_str();
    }

    template <class T = uint32_t> std::string get_enum_name(API::ManagedObject* value) {
        return call<SystemString*>(value, "ToString()")->to_str();
    }

    API::ManagedObject* get_component(API::ManagedObject* gameObject, std::string_view componentName);

    API::ManagedObject* get_child_object(API::ManagedObject* object, std::initializer_list<std::string_view> children);

    SystemString* create_managed_string(std::string_view string);

    API::ManagedObject* create_managed_array(
        std::string_view type,
        size_t length
    );

    // SDK/Activator 创建路径，保留现有默认构造语义。
    // SIMPLIFY 会临时改写共享类型分类，不用于并行原生构造中的裸分配。
    API::ManagedObject* create_managed_object(std::string_view type, int32_t flags = 0);

    // Wilds 原生 VM 分配：不执行托管构造、不改类型分类、不额外 retain。
    // 调用方须已在原生托管调用作用域内，使用本次调用的 vmctx，随后显式
    // 构造一次并通过真实 owner 发布引用。不是字符串/数组/委托的专用工厂。
    API::ManagedObject* create_managed_object(
        void* vmctx, const API::TypeDefinition* type, bool forceHeapAllocation = false);
    API::ManagedObject* create_managed_object(
        void* vmctx, std::string_view type, bool forceHeapAllocation = false);
    
    API::ManagedObject* create_managed_array(
        const API::TypeDefinition* type, 
        size_t length
    );

    SystemAction* create_managed_action(
        void *vmctx, 
        API::TypeDefinition *actionType, 
        API::ManagedObject *target, 
        API::Method *method, 
        bool a_create
    );

    // Fixed-layout callers already have the actual field slot. This overload
    // preserves reference ownership without a reflective field-name lookup.
    bool set_managed_field(API::ManagedObject* obj, API::ManagedObject** targetField, API::ManagedObject* newValue);
    // 早期原生加载期尚无RF SDK，允许传入已解析的同一原生retain/release入口。
    bool set_managed_field(API::ManagedObject* obj, API::ManagedObject** targetField, API::ManagedObject* newValue,
                           void (*retain)(API::ManagedObject*), void (*release)(API::ManagedObject*));

    bool set_managed_field(
        API::ManagedObject* obj, 
        std::string_view field_name, 
        API::ManagedObject* value
    );


    template <class T> class REArray : SystemArray<T> {
    public:
        using iterator = T*;
        using const_iterator = const T*;

        [[nodiscard]] size_t size() const { return SystemArray<T>::count; }
        const T& at(size_t idx) const { return SystemArray<T>::elements[idx]; }
        T& at(size_t idx) { return SystemArray<T>::elements[idx]; }

        T get_item(uint32_t idx) const { return call<T>(to_managed_object(this), "get_Item", idx); }
        void set_item(uint32_t idx, const T& value) { call(to_managed_object(this), "set_Item", idx, value); }

        iterator begin() { return &SystemArray<T>::elements[0]; }
        const_iterator begin() const { return &SystemArray<T>::elements[0]; }
        iterator end() { return SystemArray<T>::elements + SystemArray<T>::count; }
        const_iterator end() const { return SystemArray<T>::elements + SystemArray<T>::count; }

        [[nodiscard]] bool contains(const T& value) { return call<bool>(to_managed_object(this), "Contains", value); }

        const T& operator[](size_t idx) const { return at(idx); }
        T& operator[](size_t idx) { return at(idx); }
    };

    template<typename T>
    concept Integral = std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_pointer_v<T>;

    template <typename T>
    concept FunctionPointer = std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>;

    template <Integral T>
    T& read_offset_data(void* location, int32_t relOffset) {
        return *reinterpret_cast<T*>(static_cast<std::byte*>(location) + relOffset);
    }

    template <Integral T>
    void write_offset_data(void* location, int32_t relOffset, T value) {
        *reinterpret_cast<T*>(static_cast<std::byte*>(location) + relOffset) = value;
    }

    template <FunctionPointer T>
    T get_method(std::string_view className, std::string_view name) {
        const auto& tdb = API::get()->tdb();
        if (!tdb) {
            return nullptr;
        }
        auto* method = tdb->find_method(className, name);
        if (!method) {
            return nullptr;
        }
        auto* func = method->get_function<T>();
        if (!func) {
            return nullptr;
        }
        return func;
    }

    template <typename T>
    concept is_managed_object_v = std::is_pointer_v<T> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, API::ManagedObject>;

    template <typename Ret, typename... Args>
    requires std::is_void_v<Ret>
    Ret invoke_re_call(API::Method* method, API::ManagedObject* object, Args&&... args) {
        RE::call<Ret>(method, object, std::forward<Args>(args)...);
    }

    template <typename Ret, typename... Args>
    requires is_managed_object_v<Ret>
    Ret invoke_re_call(API::Method* method, API::ManagedObject* object, Args&&... args) {
        auto result = RE::call<Ret>(method, object, std::forward<Args>(args)...);
        return reinterpret_cast<Ret>(reinterpret_cast<uintptr_t>(result) & 0x7fffffffffffffffULL);
    }

    template <typename Ret, typename... Args>
    requires (!std::is_void_v<Ret> && !is_managed_object_v<Ret>)
    Ret invoke_re_call(API::Method* method, API::ManagedObject* object, Args&&... args) {
        return RE::call<Ret>(method, object, std::forward<Args>(args)...);
    }

    // Resolve a class declaration once; dispatch always uses the current receiver.
    int32_t get_managed_virtual_index(std::string_view baseClassName, std::string_view methodSignature);
    void* get_managed_virtual_function(API::ManagedObject* object, int32_t virtualIndex);

    template <typename Ret, typename... Args>
    Ret invoke_re_virtual_call(API::ManagedObject* object, int32_t virtualIndex, Args&&... args) {
        static_assert(std::is_void_v<Ret> || Integral<Ret>,
                      "Managed virtual calls require a scalar or pointer result; verify aggregate ABI separately");
        static_assert((Integral<std::decay_t<Args>> && ...),
                      "Managed virtual arguments must be ABI scalars or explicit pointers");
        using Fn = Ret (*)(void*, API::ManagedObject*, std::decay_t<Args>...);
        const auto function = reinterpret_cast<Fn>(get_managed_virtual_function(object, virtualIndex));
        auto* context = API::get()->get_vm_context();
        if constexpr (is_managed_object_v<Ret>) {
            const auto result = function(context, object, std::forward<Args>(args)...);
            return reinterpret_cast<Ret>(reinterpret_cast<uintptr_t>(result) & 0x7fffffffffffffffULL);
        } else {
            return function(context, object, std::forward<Args>(args)...);
        }
    }

}

// Each call site names one fixed class declaration and signature. Cache its slot,
// never the first receiver's override entry or per-thread VM context.
#define RE_VIRTUAL_CALL []<typename Ret, typename... Args> \
    (RE::API::ManagedObject* object, std::string_view baseClassName, \
     std::string_view methodSignature, Args&&... args) -> Ret { \
        static const int32_t virtualIndex = \
            RE::get_managed_virtual_index(baseClassName, methodSignature); \
        return RE::invoke_re_virtual_call<Ret>(object, virtualIndex, std::forward<Args>(args)...); \
    }.operator()

#define RE_CALL []<typename Ret, typename... Args> \
    (API::ManagedObject* object, std::string_view methodName, Args&&... args) -> Ret { \
        if (!object) { \
            if constexpr (std::is_void_v<Ret>) { return; } \
            else { return Ret{}; } \
        } \
        static auto *method = object->get_type_definition()->find_method(methodName); \
        if (!method) { \
            if constexpr (std::is_void_v<Ret>) { return; } \
            else { return Ret{}; } \
        } \
        return RE::invoke_re_call<Ret>(method, object, std::forward<Args>(args)...); \
    }.operator()

#define RE_FIELD []<typename Ret> \
    (API::ManagedObject* object, std::string_view fieldName) -> Ret* { \
        if (!object) { return nullptr; } \
        static auto *field = object->get_type_definition()->find_field(fieldName); \
        if (!field) { return nullptr; } \
        return (Ret*)field->get_data_raw((void*)object, false); \
    }.operator()
