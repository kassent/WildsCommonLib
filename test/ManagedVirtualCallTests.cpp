#include "GameTypes.h"
#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace CommonManagedVirtualCallTests {
using API = reframework::API;
static int Checks{};
static int TypeLookups{};
static int MethodLookups{};
static int IndexLookups{};
static int FunctionLookups{};
static int ContextCalls{};
static int FirstContext{}, SecondContext{};
static void* CurrentContext{&FirstContext};

static void Check(bool valid, const char* message) {
    ++Checks;
    if (!valid)
        throw std::runtime_error(message);
}

template <class F> static void Rejects(F&& operation, const char* message) {
    bool rejected = false;
    try {
        operation();
    } catch (const std::exception&) {
        rejected = true;
    }
    Check(rejected, message);
}

template <class T> static void CheckBits(T value, T expected, const char* message) {
    using Bits = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
    Check(std::bit_cast<Bits>(value) == std::bit_cast<Bits>(expected), message);
}

// These receivers are synthetic ABI fixtures, not VM-created managed objects.
struct Receiver {
    void** Info;
    uint64_t HeaderPadding{};
    int32_t Value{};
    float Single{};
    double Real{};
};

static Receiver& ReadReceiver(void* context, API::ManagedObject* object) {
    Check(context == CurrentContext, "VM context was cached or omitted");
    ++ContextCalls;
    return *reinterpret_cast<Receiver*>(object);
}

static int32_t EvaluateA(void* context, API::ManagedObject* object) {
    return ReadReceiver(context, object).Value + 1;
}
static int32_t EvaluateB(void* context, API::ManagedObject* object) {
    return ReadReceiver(context, object).Value + 100;
}
static double Blend(void* context, API::ManagedObject* object, float first, float second, double third) {
    return ReadReceiver(context, object).Value + double(first) + double(second) + third;
}
static float GetSingle(void* context, API::ManagedObject* object) {
    return ReadReceiver(context, object).Single;
}
static double GetDouble(void* context, API::ManagedObject* object) {
    return ReadReceiver(context, object).Real;
}
static void SetValue(void* context, API::ManagedObject* object, int32_t value) {
    ReadReceiver(context, object).Value = value;
}
static bool GetOutput(void* context, API::ManagedObject* object, float* output) {
    *output = ReadReceiver(context, object).Single;
    return true;
}
static API::ManagedObject* GetManaged(void* context, API::ManagedObject* object) {
    ReadReceiver(context, object);
    return reinterpret_cast<API::ManagedObject*>(uintptr_t{0x8000000000001234});
}
static void* GetPointer(void* context, API::ManagedObject* object) {
    ReadReceiver(context, object);
    return reinterpret_cast<void*>(uintptr_t{0x8000000000005678});
}
static bool IsPositive(void* context, API::ManagedObject* object) {
    return ReadReceiver(context, object).Value > 0;
}
static int32_t DirectDeclaration(void*, API::ManagedObject*) {
    return 707;
}

struct Method {
    const char* Signature;
    int32_t Index;
    bool Static;
    void* Entry;
};

static std::array<Method, 12> Methods{{
    {"evaluate()", 0, false, reinterpret_cast<void*>(&DirectDeclaration)},
    {"blend(System.Single, System.Single, System.Double)", 1, false, nullptr},
    {"get_Single()", 2, false, nullptr},
    {"get_Double()", 3, false, nullptr},
    {"set_Value(System.Int32)", 4, false, nullptr},
    {"getOutput(System.Single)", 5, false, nullptr},
    {"get_Managed()", 6, false, nullptr},
    {"get_Pointer()", 7, false, nullptr},
    {"isPositive()", 8, false, nullptr},
    {"notVirtual()", -1, false, nullptr},
    {"staticMethod()", 0, true, nullptr},
    {"direct()", -1, false, reinterpret_cast<void*>(&DirectDeclaration)},
}};
static std::array<RE::RETypeDefinition, 3> Types{};
static std::array<RE::RETypeImpl, 3> Implementations{};
static RE::RETypeDefinition InterfaceType{};
static RE::TDB Database{};
static std::array<void*, 9> SlotsA{};
static std::array<void*, 9> SlotsB{};
// Two native functions follow ObjectInfo[0], deliberately separate from Slots.
static std::array<void*, 5> InfoA{};
static std::array<void*, 5> InfoB{};

static REFrameworkTypeDefinitionHandle FindType(REFrameworkTDBHandle, const char* name) {
    ++TypeLookups;
    if (std::strcmp(name, "Tests.AbstractBase") == 0)
        return reinterpret_cast<REFrameworkTypeDefinitionHandle>(&Types[0]);
    if (std::strcmp(name, "Tests.Interface") == 0)
        return reinterpret_cast<REFrameworkTypeDefinitionHandle>(&InterfaceType);
    return nullptr;
}
static REFrameworkMethodHandle FindMethod(REFrameworkTypeDefinitionHandle type, const char* signature) {
    ++MethodLookups;
    if (std::strcmp(signature, "direct()") != 0)
        Check(type == reinterpret_cast<REFrameworkTypeDefinitionHandle>(&Types[0]),
              "declaration lookup used a receiver type instead of the requested base");
    for (auto& method : Methods)
        if (std::strcmp(method.Signature, signature) == 0)
            return reinterpret_cast<REFrameworkMethodHandle>(&method);
    return nullptr;
}
static int GetVirtualIndex(REFrameworkMethodHandle method) {
    ++IndexLookups;
    return reinterpret_cast<Method*>(method)->Index;
}
static bool IsStatic(REFrameworkMethodHandle method) {
    return reinterpret_cast<Method*>(method)->Static;
}
static void* GetFunction(REFrameworkMethodHandle method) {
    ++FunctionLookups;
    return reinterpret_cast<Method*>(method)->Entry;
}
static REFrameworkTDBHandle GetDatabase() {
    return reinterpret_cast<REFrameworkTDBHandle>(&Database);
}
static REFrameworkVMContextHandle GetContext() {
    return reinterpret_cast<REFrameworkVMContextHandle>(CurrentContext);
}
static REFrameworkTypeDefinitionHandle GetReceiverType(REFrameworkManagedObjectHandle object) {
    return reinterpret_cast<REFrameworkTypeDefinitionHandle>(reinterpret_cast<Receiver*>(object)->Info[0]);
}

static void Initialize() {
    for (size_t index = 0; index < Types.size(); ++index) {
        Types[index].index = Types[index].implIndex = index;
        Implementations[index].numManagedVtable = SlotsA.size();
        Implementations[index].numNativeEntries = 3;
    }
    InterfaceType.typeFlags = static_cast<uint32_t>(System::Reflection::TypeAttributes::Interface);
    // AbstractBase intentionally has no default ObjectInfo.
    Types[0].objectInfo = nullptr;
    SlotsA = {reinterpret_cast<void*>(&EvaluateA), reinterpret_cast<void*>(&Blend),
              reinterpret_cast<void*>(&GetSingle), reinterpret_cast<void*>(&GetDouble),
              reinterpret_cast<void*>(&SetValue), reinterpret_cast<void*>(&GetOutput),
              reinterpret_cast<void*>(&GetManaged), reinterpret_cast<void*>(&GetPointer),
              reinterpret_cast<void*>(&IsPositive)};
    SlotsB = SlotsA;
    SlotsB[0] = reinterpret_cast<void*>(&EvaluateB);
    InfoA = {SlotsA.data(), reinterpret_cast<void*>(uintptr_t{16}), &Types[1],
             reinterpret_cast<void*>(&DirectDeclaration), reinterpret_cast<void*>(&DirectDeclaration)};
    InfoB = {SlotsB.data(), reinterpret_cast<void*>(uintptr_t{16}), &Types[2],
             reinterpret_cast<void*>(&DirectDeclaration), reinterpret_cast<void*>(&DirectDeclaration)};
    Types[1].objectInfo = InfoA.data() + 2;
    Types[2].objectInfo = InfoB.data() + 2;
    Database.version = 81;
    Database.numTypeImpl = Implementations.size();
    Database.typeImpls = Implementations.data();
    static const REFrameworkSDKFunctions functions{.get_tdb = &GetDatabase, .get_vm_context = &GetContext};
    static const REFrameworkTDB database{.find_type = &FindType};
    static const REFrameworkTDBTypeDefinition type{.find_method = &FindMethod};
    static const REFrameworkTDBMethod method{.get_function = &GetFunction,
                                            .get_virtual_index = &GetVirtualIndex,
                                            .is_static = &IsStatic};
    static const REFrameworkManagedObject object{.get_type_definition = &GetReceiverType};
    static const REFrameworkSDKData sdk{.functions = &functions, .tdb = &database,
                                       .type_definition = &type, .method = &method, .managed_object = &object};
    static const REFrameworkPluginInitializeParam parameter{.sdk = &sdk};
    API::initialize(&parameter);
}

static API::ManagedObject* Managed(Receiver& object) {
    return reinterpret_cast<API::ManagedObject*>(&object);
}
static int32_t Evaluate(Receiver& object) {
    return RE_VIRTUAL_CALL<int32_t>(Managed(object), "Tests.AbstractBase", "evaluate()");
}
static double CallBlend(Receiver& object, float& first, float& second, double& third) {
    return RE_VIRTUAL_CALL<double>(Managed(object), "Tests.AbstractBase",
                                   "blend(System.Single, System.Single, System.Double)", first, second, third);
}
static float CallSingle(Receiver& object) {
    return RE_VIRTUAL_CALL<float>(Managed(object), "Tests.AbstractBase", "get_Single()");
}
static double CallDouble(Receiver& object) {
    return RE_VIRTUAL_CALL<double>(Managed(object), "Tests.AbstractBase", "get_Double()");
}

static void Run() {
    Initialize();
    Receiver first{.Info = InfoA.data() + 2, .Value = 10, .Single = 1.25f, .Real = 4.5};
    Receiver second{.Info = InfoB.data() + 2, .Value = 20, .Single = 2.5f, .Real = 8.75};
    const auto lookups = MethodLookups;
    for (int repeat = 0; repeat < 4; ++repeat) {
        Check(Evaluate(first) == 11, "A override dispatch");
        Check(Evaluate(second) == 120, "B override dispatch from the same call site");
    }
    Check(MethodLookups == lookups + 1 && IndexLookups == 1 && TypeLookups == 1,
          "the fixed base declaration is resolved once per call site");
    Check(FunctionLookups == 0, "virtual dispatch must not call Method.get_function");
    CurrentContext = &SecondContext;
    Check(Evaluate(first) == 11, "call context is read for the current invocation");
    SlotsA[0] = reinterpret_cast<void*>(&EvaluateB);
    Check(Evaluate(first) == 110, "receiver entry is read on every invocation");
    SlotsA[0] = reinterpret_cast<void*>(&EvaluateA);
    first.Info = InfoB.data() + 2;
    Check(Evaluate(first) == 110, "receiver ObjectInfo is not cached");
    first.Info = InfoA.data() + 2;

    float left = 0.25f, right = 0.5f;
    double tail = 0.125;
    CheckBits(CallBlend(first, left, right, tail), 10.875, "float lvalue parameters use value ABI");
    CheckBits(RE_VIRTUAL_CALL<double>(Managed(first), "Tests.AbstractBase",
                                      "blend(System.Single, System.Single, System.Double)", 0.25f, 0.5f, 0.125),
              10.875, "floating rvalues use the same ABI");
    for (uint32_t bits : {uint32_t{0}, uint32_t{0x80000000}, uint32_t{0x7f800000}, uint32_t{0x7fc01234}}) {
        first.Single = std::bit_cast<float>(bits);
        CheckBits(CallSingle(first), first.Single, "Single return bits");
    }
    for (uint64_t bits : {uint64_t{0}, uint64_t{0x8000000000000000}, uint64_t{0x7ff0000000000000},
                          uint64_t{0x7ff8000000001234}}) {
        first.Real = std::bit_cast<double>(bits);
        CheckBits(CallDouble(first), first.Real, "Double return bits");
    }
    RE_VIRTUAL_CALL<void>(Managed(first), "Tests.AbstractBase", "set_Value(System.Int32)", int32_t{-9});
    Check(first.Value == -9, "void and integer argument");
    Check(!RE_VIRTUAL_CALL<bool>(Managed(first), "Tests.AbstractBase", "isPositive()"), "bool result");
    first.Single = 19.5f;
    float output{};
    Check(RE_VIRTUAL_CALL<bool>(Managed(first), "Tests.AbstractBase", "getOutput(System.Single)", &output),
          "explicit out pointer result");
    CheckBits(output, first.Single, "explicit out pointer payload");
    Check(RE_VIRTUAL_CALL<API::ManagedObject*>(Managed(first), "Tests.AbstractBase", "get_Managed()") ==
              reinterpret_cast<API::ManagedObject*>(uintptr_t{0x1234}), "managed pointer return normalization");
    Check(RE_VIRTUAL_CALL<void*>(Managed(first), "Tests.AbstractBase", "get_Pointer()") ==
              reinterpret_cast<void*>(uintptr_t{0x8000000000005678}), "native pointer bits are unchanged");

    Rejects([&] { RE::get_managed_virtual_index("Tests.Absent", "evaluate()"); }, "missing declaring type");
    Rejects([&] { RE::get_managed_virtual_index("Tests.AbstractBase", "absent()"); }, "missing method");
    Rejects([&] { RE::get_managed_virtual_index("Tests.AbstractBase", "staticMethod()"); }, "static method rejected");
    Rejects([&] { RE::get_managed_virtual_index("Tests.AbstractBase", "notVirtual()"); }, "nonvirtual method rejected");
    Rejects([&] { RE::get_managed_virtual_index("Tests.Interface", "evaluate()"); }, "interface index rejected");
    Rejects([&] { RE::invoke_re_virtual_call<int32_t>(nullptr, 0); }, "null receiver");
    Rejects([&] { RE::invoke_re_virtual_call<int32_t>(Managed(first), -1); }, "negative virtual index");
    Rejects([&] { RE::invoke_re_virtual_call<int32_t>(Managed(first), 9); }, "out of range virtual index");
    SlotsA[0] = nullptr;
    Rejects([&] { Evaluate(first); }, "null virtual entry");
    SlotsA[0] = reinterpret_cast<void*>(&EvaluateA);
    InfoA[0] = nullptr;
    Rejects([&] { Evaluate(first); }, "null managed table");
    InfoA[0] = SlotsA.data();
    Database.version = 80;
    Rejects([&] { Evaluate(first); }, "unverified TDB layout");
    Database.version = 81;
    const auto implIndex = Types[1].implIndex;
    Types[1].implIndex = Database.numTypeImpl;
    Rejects([&] { Evaluate(first); }, "invalid type implementation index");
    Types[1].implIndex = implIndex;

    Check(RE_CALL<int32_t>(Managed(first), "direct()") == 707, "existing direct RE_CALL remains direct");
    Check(ContextCalls > 0 && FunctionLookups == 1, "only direct RE_CALL requested a declaration function");
}
} // namespace CommonManagedVirtualCallTests

int main() {
    try {
        CommonManagedVirtualCallTests::Run();
        std::cout << "Managed virtual call checks: " << CommonManagedVirtualCallTests::Checks << " passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Managed virtual call test failed: " << error.what() << '\n';
        return 1;
    }
}
