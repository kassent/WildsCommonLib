#include "GameTypes.h"
#include <Windows.h>
#include <climits>
#include <cstdint>
#include <format>
#include <intrin.h>
#include "HookUtils.h"
#include "LogUtils.h"
namespace RE {

    using API = reframework::API;

    int32_t get_managed_virtual_index(std::string_view baseClassName, std::string_view methodSignature) {
        auto* tdb = API::get()->tdb();
        auto* base = tdb ? tdb->find_type(baseClassName) : nullptr;
        if (!base)
            throw std::runtime_error("Managed virtual declaring type is absent: " + std::string(baseClassName));
        // Interface slots use ObjectInfo's interface prefix, not managedVtable.
        if ((reinterpret_cast<const RETypeDefinition*>(base)->typeFlags &
             static_cast<uint32_t>(System::Reflection::TypeAttributes::Interface)) != 0)
            throw std::invalid_argument("Managed class virtual call received an interface: " + std::string(baseClassName));
        auto* declaration = base->find_method(methodSignature);
        if (!declaration || declaration->is_static())
            throw std::runtime_error("Managed virtual instance declaration is absent: " +
                                     std::string(baseClassName) + "." + std::string(methodSignature));
        const auto index = declaration->get_virtual_index();
        if (index < 0)
            throw std::runtime_error("Method has no managed virtual index: " +
                                     std::string(baseClassName) + "." + std::string(methodSignature));
        return index;
    }

    void* get_managed_virtual_function(API::ManagedObject* object, int32_t virtualIndex) {
        if (!object)
            throw std::invalid_argument("Managed virtual receiver is null");
        auto** info = *reinterpret_cast<void***>(object);
        if (!info || !info[0] || !info[-2])
            throw std::runtime_error("Managed virtual receiver has no ObjectInfo table");
        const auto* definition = static_cast<const RETypeDefinition*>(info[0]);
        const auto* tdb = reinterpret_cast<const TDB*>(API::get()->tdb());
        if (!tdb || tdb->version != 81 || !tdb->typeImpls || definition->implIndex >= tdb->numTypeImpl)
            throw std::runtime_error("Managed virtual receiver requires the verified TDB81 type implementation");
        const auto count = tdb->typeImpls[definition->implIndex].numManagedVtable;
        if (virtualIndex < 0 || static_cast<uint32_t>(virtualIndex) >= count)
            throw std::out_of_range("Managed virtual index is outside the receiver's table");
        auto** table = static_cast<void**>(info[-2]);
        auto* entry = table[virtualIndex];
        if (!entry)
            throw std::runtime_error("Managed virtual slot has no implementation");
        return entry;
    }

    std::string narrow(std::wstring_view str) {
        const auto length = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.length()), nullptr, 0, nullptr, nullptr);
        std::string narrowStr{};

        narrowStr.resize(length);
        WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.length()), (LPSTR)narrowStr.c_str(), length, nullptr, nullptr);

        return narrowStr;
    }

    // 先计算 UTF-16 长度，再写入自有可写缓冲；失败不向调用方交出半成品。
    std::wstring widen(std::string_view str, uint32_t flags) {
        if (str.empty() || str.size() > INT_MAX)
            return {};
        const auto bytes = static_cast<int>(str.size());
        const auto length = MultiByteToWideChar(CP_UTF8, flags, str.data(), bytes, nullptr, 0);
        if (!length)
            return {};
        std::wstring wideStr(size_t(length), L'\0');
        if (MultiByteToWideChar(CP_UTF8, flags, str.data(), bytes, wideStr.data(), length) != length)
            return {};
        return wideStr;
    }

    API::ManagedObject* get_component(API::ManagedObject* gameObject, std::string_view componentName) {
        auto *type = API::get()->tdb()->find_type(componentName);
        if (!type) {
            return nullptr;
        }
        return RE_CALL<API::ManagedObject*>(gameObject, "getComponent(System.Type)", type->get_runtime_type());
    }

    API::ManagedObject* get_child_object(API::ManagedObject* object, std::initializer_list<std::string_view> children) {
        auto* currentObject = object;
        for (const auto childName : children) {
            if (!currentObject) {
                return nullptr;
            }

            auto* transform = RE_CALL<API::ManagedObject*>(currentObject, "get_Transform()");
            if (!transform) {
                return nullptr;
            }

            const auto wideChildName = widen(childName);
            auto* childTransform = RE_CALL<API::ManagedObject*>(transform, "get_Child()");
            currentObject = nullptr;
            while (childTransform) {
                auto* childObject = RE_CALL<API::ManagedObject*>(childTransform, "get_GameObject()");
                auto* name = RE_CALL<SystemString*>(childObject, "get_Name()");
                if (name && std::wstring_view(name->data, name->size) == wideChildName) {
                    currentObject = childObject;
                    break;
                }
                childTransform = RE_CALL<API::ManagedObject*>(childTransform, "get_Next()");
            }

            if (!currentObject) {
                return nullptr;
            }
        }
        return currentObject;
    }

    SystemString* create_managed_string(std::string_view content) {
        auto wContent = widen(content);
        // ---@field FastAllocateString fun(arg0: System.Int32|integer): System.String|string private static / id: 807288
        using namespace reframework;
        const auto& api = API::get();
        // if (wContent.length() <= 1024) {
        //     return reinterpret_cast<SystemString*>(api->sdk()->functions->create_managed_string(wContent.data()));
        // }
        static auto *fastAllocateStringMethod = api->tdb()->find_method("System.String", "FastAllocateString(System.Int32)");
        auto *managedString = RE::call<SystemString*>(fastAllocateStringMethod, wContent.length());
        if (managedString != nullptr && managedString->size >= wContent.length()) {
            std::memcpy(managedString->data, wContent.data(), wContent.length() * sizeof(wchar_t));
        }
        return managedString;
    }

    bool set_managed_field(API::ManagedObject* obj, std::string_view field_name, API::ManagedObject* newValue) {
        return obj && set_managed_field(obj,obj->get_field<API::ManagedObject*>(field_name),newValue);
    }

    bool set_managed_field(API::ManagedObject* obj, API::ManagedObject** targetField, API::ManagedObject* newValue) {
        return set_managed_field(
            obj, targetField, newValue, +[](API::ManagedObject* value) { value->add_ref(); },
            +[](API::ManagedObject* value) { value->release(); });
    }

    bool set_managed_field(API::ManagedObject* obj, API::ManagedObject** targetField, API::ManagedObject* newValue,
                           void (*retain)(API::ManagedObject*), void (*release)(API::ManagedObject*)) {
        using ManagedObject = API::ManagedObject;
        if (obj == nullptr || targetField == nullptr) {
            return false;
        }

        constexpr int32_t MANAGED_OBJECT_REF_COUNT_OFFSET = 0x8;
        if (read_offset_data<int32_t>(obj, MANAGED_OBJECT_REF_COUNT_OFFSET) < 0) {
            *targetField = newValue;
        } else if (*targetField != newValue) {
            if (newValue) {
                retain(newValue);
            }

            ManagedObject* previous = nullptr;
            ManagedObject* observed = nullptr;
            do {
                previous = *targetField;
                observed = static_cast<ManagedObject*>(
                    _InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(targetField), newValue, previous));
            } while (observed != previous);

            if (previous) {
                release(previous);
            }
        }
        return true;
    }

    API::ManagedObject* create_managed_array(std::string_view type, size_t length) {
        using namespace reframework;
        const auto& api = API::get();
        auto *type_definition = api->tdb()->find_type(type.data());
        return create_managed_array(type_definition, length);
    }

    API::ManagedObject* create_managed_object(std::string_view type, int32_t flags) {
        using namespace reframework;
        const auto& api = API::get();
        auto* type_definition = api->tdb()->find_type(type.data());
        if (!type_definition) {
            _MESSAGE("[create_managed_object] type not found: %s", std::string(type).c_str());
            return nullptr;
        }
        auto* instance = type_definition->create_instance(flags);
        if (!instance) {
            _MESSAGE("[create_managed_object] create_instance failed: %s (flags=%d)", std::string(type).c_str(), flags);
        }
        return instance;
    }

    using FnAllocateManagedObject = API::ManagedObject* (*)(void*, const API::TypeDefinition*, bool);

    static FnAllocateManagedObject resolve_managed_object_allocator() {
        using FnConstructor = void (*)(void*, API::ManagedObject*);
        const auto ctor = get_method<FnConstructor>("app.EmCommonAction.cAngleBlendBase", ".ctor()");
        if (!ctor)
            throw std::runtime_error("VM object allocator: cAngleBlendBase constructor is absent");
        // Original new YawBlend: RCX=the constructor's VM context, RDX=TDB type,
        // R8B=false. The following call is the VM allocator, not the bool ctor.
        const auto callSetup = HookUtils::LookupFunctionPattern(
            reinterpret_cast<void*>(ctor), "48 89 F9 45 31 C0 E8", 0x100);
        if (!callSetup)
            throw std::runtime_error("VM object allocator: native allocation call setup is absent");
        const auto entry = HookUtils::ReadOffsetData(callSetup + 6, 1, 5);
        if (!entry)
            throw std::runtime_error("VM object allocator: allocation callee is absent");
        _MESSAGE("[create_managed_object] resolved from cAngleBlendBase.ctor: caller=%p call=%p entry=%p",
                 ctor, reinterpret_cast<void*>(callSetup + 6), reinterpret_cast<void*>(entry));
        return reinterpret_cast<FnAllocateManagedObject>(entry);
    }

    API::ManagedObject* create_managed_object(void* vmctx, const API::TypeDefinition* type, bool forceHeapAllocation) {
        if (!vmctx || !type)
            return nullptr;
        const auto* definition = reinterpret_cast<const RETypeDefinition*>(type);
        // These need their own variable-size or binding factory contracts.
        if (definition->objectType != static_cast<uint32_t>(via::clr::VMObjType::Object)
            || (definition->typeFlags &
                (static_cast<uint32_t>(System::Reflection::TypeAttributes::Interface) |
                 static_cast<uint32_t>(System::Reflection::TypeAttributes::Abstract))) != 0) {
            _MESSAGE("[create_managed_object] ordinary concrete class required: type=%p kind=%u flags=%08X",
                     type, static_cast<unsigned>(definition->objectType), definition->typeFlags);
            return nullptr;
        }
        // Immutable after successful resolution. A throwing initializer is not
        // cached as a permanent null entry; vmctx and type are never cached.
        static const auto allocate = resolve_managed_object_allocator();
        auto* object = allocate(vmctx, type, forceHeapAllocation);
        if (!object)
            _MESSAGE("[create_managed_object] native allocation returned null: context=%p type=%p heap=%d",
                     vmctx, type, forceHeapAllocation);
        return object;
    }

    API::ManagedObject* create_managed_object(void* vmctx, std::string_view name, bool forceHeapAllocation) {
        if (!vmctx || name.empty())
            return nullptr;
        const auto ownedName = std::string(name);
        auto* type = API::get()->tdb()->find_type(ownedName);
        if (!type) {
            _MESSAGE("[create_managed_object] type not found: %s", ownedName.c_str());
            return nullptr;
        }
        return create_managed_object(vmctx, type, forceHeapAllocation);
    }

    API::ManagedObject* create_managed_array(const API::TypeDefinition* type, size_t length) {
        using namespace reframework;
        const auto& api = API::get();
        return reinterpret_cast<API::ManagedObject*>(api->sdk()->functions->create_managed_array(*type, length));
    }


    std::string SystemGuid::to_str() const {
        // b12f11b6-a65b-4799-ba2b-42d833e884b6
        return std::format("{:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
            mData1,
            mData2,
            mData3,
            mData4.mData4_0, mData4.mData4_1, mData4.mData4_2, mData4.mData4_3, mData4.mData4_4, mData4.mData4_5, mData4.mData4_6, mData4.mData4_7);
    }

    bool SystemGuid::from_str(std::string_view str) {
        if (str.empty() || str.length() != 36) {
            return false;
        }
        // d9f8dedd-550d-4c77-908e-70a893d3dc56
        uint32_t data1, data2, data3, data4_0, data4_1, data4_2, data4_3, data4_4, data4_5, data4_6, data4_7;
        bool success = sscanf_s(str.data(), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            &data1, &data2, &data3, &data4_0, &data4_1, &data4_2, &data4_3, &data4_4, &data4_5, &data4_6, &data4_7) == 11;
        if (success) {
            mData1 = static_cast<uint32_t>(data1);
            mData2 = static_cast<uint16_t>(data2);
            mData3 = static_cast<uint16_t>(data3);
            mData4.mData4_0 = static_cast<uint8_t>(data4_0);
            mData4.mData4_1 = static_cast<uint8_t>(data4_1);
            mData4.mData4_2 = static_cast<uint8_t>(data4_2);
            mData4.mData4_3 = static_cast<uint8_t>(data4_3);
            mData4.mData4_4 = static_cast<uint8_t>(data4_4);
            mData4.mData4_5 = static_cast<uint8_t>(data4_5);
            mData4.mData4_6 = static_cast<uint8_t>(data4_6);
            mData4.mData4_7 = static_cast<uint8_t>(data4_7);
        }
        return success;
    }

    bool SystemGuid::operator==(const SystemGuid& other) const {
        return mData1 == other.mData1 && mData2 == other.mData2 && mData3 == other.mData3 && mData4.mData4L == other.mData4.mData4L;
    }

    RE::SystemAction* create_managed_action(void *vmctx, API::TypeDefinition *actionType, API::ManagedObject *target, API::Method *method, bool a_create) {
        static auto *setupControlMethod = API::get()->tdb()->find_method("app.GUI030205PartsBox", "setupControl()");
        if (!setupControlMethod) {
            _MESSAGE("Failed to get app.GUI030205PartsBox.setupControl");
            return nullptr;
        }
        static auto *func = setupControlMethod->get_function_raw();
        if (!func) {
            _MESSAGE("Failed to get app.GUI030205PartsBox.setupControl");
            return nullptr;
        }
        // E8 ?? ?? ?? ?? C5 7B 11 40 10 C5 FA 11 78 18
        static auto addr = HookUtils::LookupFunctionPattern(func, "E8 ?? ?? ?? ?? 4D 8B B6", 0x100);
        if (!addr) {
            _MESSAGE("Failed to get addr for app.GUI030205PartsBox.setupControl");
            return nullptr;
        }
        static auto *functor = (decltype(create_managed_action)*)HookUtils::ReadOffsetData(addr, 0x1, 5);
        return functor(vmctx, actionType, target, method, a_create);
    }
}
