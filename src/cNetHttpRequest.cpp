#include "cNetHttpRequest.h"
#include <cstdint>
#include <reframework/API.h>
#include <LogUtils.h>
#include "GameTypes.h"
#include "HookUtils.h"

using API = reframework::API;

namespace app::cNetHttpRequest {

    void OnResponse(void *vmctx, RE::REArray<uint8_t> *a_userData, bool a_failed, int32_t a_statusCode, RE::REArray<uint8_t> *a_response) {
        if (a_userData == nullptr || a_userData->size() < sizeof(uintptr_t)) {
            _MESSAGE("OnResponse, payload is null, failed: %d, statusCode: %d, response: %p", a_failed, a_statusCode, a_response);
            return;
        }

        RequestParams::callback_t callback = *reinterpret_cast<RequestParams::callback_t*>(a_userData->begin());
        std::span<uint8_t> userData(a_userData->begin() + sizeof(uintptr_t), a_userData->end());
        // _MESSAGE("callback: %p", callback);
        if (callback) {
            std::span<uint8_t> response;
            if (a_response) {
                response = { a_response->begin(), a_response->end() };
            }
            callback(userData, a_failed, a_statusCode, response);
        }
    }

    int32_t SendRequest(const RequestParams& a_params) {
        // _MESSAGE("SendRequest: %s, %d", a_params.url.c_str(), a_params.method);
        auto * httpRequestType = API::get()->tdb()->find_type("app.cNetHttpRequest");
        if (!httpRequestType) {
            _MESSAGE("httpRequestType not found");
            return ErrorCode::FailedToFindHttpRequestType;
        }
        auto *httpRequest = httpRequestType->create_instance(REFRAMEWORK_CREATE_INSTANCE_FLAGS_SIMPLIFY);
        if (!httpRequest) {
            _MESSAGE("failed to create httpRequest");
            return ErrorCode::FailedToCreateHttpRequest;
        }
        RE::call(httpRequest, ".ctor(System.String, app.cNetHttpRequest.TYPE, via.os.http_client.Method)", RE::create_managed_string(a_params.url), 1, a_params.method);

        if (!a_params.body.empty()) {
            auto *byteArray = reinterpret_cast<RE::REArray<uint8_t> *>(
                RE::create_managed_array("System.Byte", a_params.body.size()));
            std::memcpy(byteArray->begin(), a_params.body.data(), a_params.body.size());
            RE::call(httpRequest, "addBody(System.Byte[], System.Boolean)", byteArray, true);
        }

        for (const auto& header : a_params.headers) {
            RE::call(httpRequest, "addHeader(System.String)", RE::create_managed_string(header));
        }

        static auto *networkManager = API::get()->get_managed_singleton("app.NetworkManager");
        if (!networkManager) {
            _MESSAGE("Failed to get app.NetworkManager");
            return ErrorCode::FailedToGetNetworkManager;
        }
        // ---@field get_APIServer fun(self): app.Net_APIServer public / id: 702613
        static auto *apiServer = RE::call(networkManager, "get_APIServer()");
        if (!apiServer) {
            _MESSAGE("Failed to get app.Net_APIServer");
            return ErrorCode::FailedToGetAPIServer;
        }

        // ---@field addAPIHeader fun(self, arg0: app.cNetHttpRequest): nil public / id: 251109
        if (a_params.requireAPIHeader) {
            RE::call(apiServer, "addAPIHeader(app.cNetHttpRequest)", httpRequest);
            // get_Header fun(self): System.String|string public 0x14a89c0e0 / id: 250882
#if 0
            static auto *getHeaderMethod = API::get()->tdb()->find_method("ace.cHttpRequest", "get_Header()");
            if (getHeaderMethod) {
                auto *header = RE::call<RE::SystemString *>(getHeaderMethod, httpRequest);
                if (header) {
                    _MESSAGE("header: %s", header->to_str().c_str());
                }
            }
#endif
        }

        static auto *actionType = API::get()->tdb()->find_type("System.Action`3<System.Boolean,System.Int32,System.Byte[]>");
        if (!actionType) {
            _MESSAGE("Failed to get System.Action`3<System.Boolean,System.Int32,System.Byte[]>");
            return ErrorCode::FailedToGetActionType;
        }

        static auto *resetTitleMethod = API::get()->tdb()->find_method("app.net_context_manager.cContextManager", "resetTitle()");
        if (!resetTitleMethod) {
            _MESSAGE("Failed to get app.net_context_manager.cContextManager.resetTitle()");
            return ErrorCode::FailedToGetCallbackMethod;
        }

        auto *byteArray = reinterpret_cast<RE::REArray<uint8_t> *>(
                    RE::create_managed_array("System.Byte", a_params.userData.size() + sizeof(uintptr_t)));
        if (!byteArray) {
            _MESSAGE("Failed to create byteArray");
            return ErrorCode::FailedToCreateByteArray;
        }
        std::memcpy(byteArray->begin(), &a_params.callback, sizeof(uintptr_t));
        std::memcpy(byteArray->begin() + sizeof(uintptr_t), a_params.userData.data(), a_params.userData.size());

        auto *action = RE::create_managed_action(API::get()->get_vm_context(), actionType, RE::to_managed_object(byteArray), resetTitleMethod, false);
        if (!action) {
            _MESSAGE("Failed to create action");
            return ErrorCode::FailedToCreateAction;
        }
        action->methods[0].func = OnResponse;

        // ---@field RequestCallbackBinary System.Action<System.Boolean, System.Int32, System.Byte[]> public / offset: 0x38
        RE::set_managed_field(httpRequest, "RequestCallbackBinary", RE::to_managed_object(action));

        // ---@class app.GA : ace.AceGA
        // ---@field get_Http fun(): app.NetHttpRequestService public static / id: 1090605
        static auto *getHttpMethod = API::get()->tdb()->find_method("app.GA", "get_Http()");
        if (!getHttpMethod) {
            _MESSAGE("Failed to get app.GA.get_Http()");
            return ErrorCode::FailedToGetHttpService;
        }

        auto *httpService = RE::call(getHttpMethod);
        if (!httpService) {
            _MESSAGE("Failed to get app.NetHttpRequestService");
            return ErrorCode::FailedToGetHttpService;
        }
        // _MESSAGE("httpService: %p", httpService);
        // ---@field request fun(self, arg0: ace.cHttpRequest): System.Boolean|boolean public virtual / id: 251084
        static auto *requestMethod = API::get()->tdb()->find_method("ace.HttpRequestService", "request(ace.cHttpRequest)");
        if (!requestMethod) {
            _MESSAGE("Failed to get app.NetHttpRequestService.request(ace.cHttpRequest)");
            return ErrorCode::FailedToGetRequestMethod;
        }
        bool result = RE::call<bool>(requestMethod, httpService, httpRequest);
        // _MESSAGE("SendRequest result: %d", result);
        if (!result) {
            _MESSAGE("Failed to send request");
            return ErrorCode::FailedToSendRequest;
        }
        return ErrorCode::Success;
    }
}
