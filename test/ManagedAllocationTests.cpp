// Synthetic ABI/resolver checks only; live allocation has separate acceptance.
#include "GameTypes.h"
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace CommonManagedAllocationTests {
using API = reframework::API;
static int Checks{}, Lookups{}, NativeCalls{}, ActivatorCalls{};
static bool HasAnchor{true}, HasMethod{true}, NullResult{};
static std::array<RE::RETypeDefinition, 3> Types{};
static RE::TDB Database{};
static std::array<uint8_t, 0x100> ConstructorBytes{};
static std::array<uint8_t, 64> Allocation{};
static void* Context{};
static const API::TypeDefinition* RequestedType{};
static bool ForceHeap{};

static void Check(bool valid, const char* message) {
    ++Checks;
    if (!valid) throw std::runtime_error(message);
}
template<class F> static void Throws(F&& fn, const char* message) {
    bool caught{}; try { fn(); } catch (const std::runtime_error&) { caught = true; }
    Check(caught, message);
}
static API::ManagedObject* Allocate(void* context, const API::TypeDefinition* type, bool forceHeap) {
    ++NativeCalls; Context = context; RequestedType = type; ForceHeap = forceHeap;
    return NullResult ? nullptr : reinterpret_cast<API::ManagedObject*>(Allocation.data());
}
static REFrameworkTypeDefinitionHandle FindType(REFrameworkTDBHandle, const char* name) {
    ++Lookups;
    if (std::strcmp(name, "app.EmCommonAction.cAngleBlendBase") == 0)
        return HasAnchor ? reinterpret_cast<REFrameworkTypeDefinitionHandle>(&Types[0]) : nullptr;
    if (std::strcmp(name, "Tests.First") == 0) return reinterpret_cast<REFrameworkTypeDefinitionHandle>(&Types[1]);
    if (std::strcmp(name, "Tests.Second") == 0) return reinterpret_cast<REFrameworkTypeDefinitionHandle>(&Types[2]);
    return nullptr;
}
static REFrameworkMethodHandle FindMethod(REFrameworkTypeDefinitionHandle, const char* name) {
    return HasMethod && std::strcmp(name, ".ctor()") == 0
        ? reinterpret_cast<REFrameworkMethodHandle>(ConstructorBytes.data()) : nullptr;
}
static REFrameworkMethodHandle FindDatabaseMethod(REFrameworkTDBHandle db, const char* type, const char* signature) {
    const auto definition = FindType(db, type);
    return definition ? FindMethod(definition, signature) : nullptr;
}
static void* GetFunction(REFrameworkMethodHandle method) { return reinterpret_cast<void*>(method); }
static REFrameworkTDBHandle GetDatabase() { return reinterpret_cast<REFrameworkTDBHandle>(&Database); }
static REFrameworkManagedObjectHandle Activator(REFrameworkTypeDefinitionHandle, unsigned int) {
    ++ActivatorCalls; return nullptr;
}
static const API::TypeDefinition* Type(size_t index) { return reinterpret_cast<API::TypeDefinition*>(&Types[index]); }
static void Initialize() {
    for (auto& t : Types) t.objectType = static_cast<uint32_t>(via::clr::VMObjType::Object);
    Database.version = 81;
    static const REFrameworkSDKFunctions functions{.get_tdb = &GetDatabase};
    static const REFrameworkTDB database{.find_type = &FindType, .find_method = &FindDatabaseMethod};
    static const REFrameworkTDBTypeDefinition type{.find_method = &FindMethod, .create_instance = &Activator};
    static const REFrameworkTDBMethod method{.get_function = &GetFunction};
    static const REFrameworkSDKData sdk{.functions = &functions, .tdb = &database, .type_definition = &type, .method = &method};
    static const REFrameworkPluginInitializeParam parameter{.sdk = &sdk};
    API::initialize(&parameter);
    ConstructorBytes.fill(0xcc);
}
static void Run() {
    Initialize();
    int firstContext{}, secondContext{};
    Check(!RE::create_managed_object(nullptr, Type(1)), "null context");
    Check(!RE::create_managed_object(&firstContext, static_cast<const API::TypeDefinition*>(nullptr)), "null type");
    Check(!RE::create_managed_object(&firstContext, std::string_view{}), "empty type name");
    Check(!RE::create_managed_object(&firstContext, "Tests.Absent"), "unknown type name");
    for (auto kind : {via::clr::VMObjType::Array, via::clr::VMObjType::String,
                      via::clr::VMObjType::Delegate, via::clr::VMObjType::ValType}) {
        Types[1].objectType = static_cast<uint32_t>(kind);
        Check(!RE::create_managed_object(&firstContext, Type(1)), "special allocation kind");
    }
    Types[1].objectType = static_cast<uint32_t>(via::clr::VMObjType::Object);
    for (auto flag : {System::Reflection::TypeAttributes::Interface, System::Reflection::TypeAttributes::Abstract}) {
        Types[1].typeFlags = static_cast<uint32_t>(flag);
        Check(!RE::create_managed_object(&firstContext, Type(1)), "interface or abstract type");
    }
    Types[1].typeFlags = 0;
    HasAnchor = false;
    Throws([&] { RE::create_managed_object(&firstContext, Type(1)); }, "missing native caller");
    HasAnchor = true; HasMethod = false;
    Throws([&] { RE::create_managed_object(&firstContext, Type(1)); }, "missing constructor");
    HasMethod = true;
    Throws([&] { RE::create_managed_object(&firstContext, Type(1)); }, "missing native argument contract");

    const std::array<uint8_t, 7> prefix{0x48,0x89,0xf9,0x45,0x31,0xc0,0xe8};
    std::memcpy(ConstructorBytes.data() + 0x20, prefix.data(), prefix.size());
    const auto delta = reinterpret_cast<intptr_t>(&Allocate) - reinterpret_cast<intptr_t>(ConstructorBytes.data() + 0x2b);
    Check(delta >= INT32_MIN && delta <= INT32_MAX, "fixture E8 reach");
    const auto displacement = static_cast<int32_t>(delta);
    std::memcpy(ConstructorBytes.data() + 0x27, &displacement, sizeof(displacement));
    const auto before = Types;
    auto* result = RE::create_managed_object(&firstContext, Type(1));
    Check(result == reinterpret_cast<API::ManagedObject*>(Allocation.data()), "native result preserved");
    Check(Context == &firstContext && RequestedType == Type(1) && !ForceHeap, "native arguments and default allocation mode");
    const auto resolvedLookups = Lookups;
    Check(RE::create_managed_object(&secondContext, Type(2), true) == result, "second native allocation");
    Check(Context == &secondContext && RequestedType == Type(2) && ForceHeap, "context and type are not cached");
    Check(Lookups == resolvedLookups, "successful resolver is immutable and cached");
    Check(RE::create_managed_object(&firstContext, std::string_view("Tests.FirstTrailing", 11)) == result,
          "type-name view is NUL terminated before SDK query");
    NullResult = true;
    Check(!RE::create_managed_object(&firstContext, Type(1)), "native null allocation remains failure");
    Check(NativeCalls == 4 && ActivatorCalls == 0, "no Activator or automatic constructor");
    Check(std::memcmp(before.data(), Types.data(), sizeof(Types)) == 0, "shared type metadata is untouched");
}
} // namespace CommonManagedAllocationTests
int main() {
    try {
        CommonManagedAllocationTests::Run();
        std::cout << "Managed allocation checks: " << CommonManagedAllocationTests::Checks << " passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
