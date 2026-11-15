#pragma once
#include <GameTypes.h>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

namespace app::cNetHttpRequest {

    enum Method {
        Get = 0,
        Post = 1,
        Put = 2,
        Patch = 3,
        Delete = 4,
    };

    enum ErrorCode {
        Success = 0,
        FailedToFindHttpRequestType = 1,
        FailedToCreateHttpRequest = 2,
        FailedToAddBody = 3,
        FailedToCreateAction = 4,
        FailedToSendRequest = 5,
        FailedToGetHttpService = 6,
        FailedToGetRequestMethod = 7,
        FailedToGenerateJsonPayload = 8,
        FailedToGetCallbackMethod = 9,
        FailedToGetActionType = 10,
        FailedToGetNetworkManager = 11,
        FailedToGetAPIServer = 12,
        FailedToCreateByteArray = 13,
    };

    enum ContentType {
        Json = 1,
        MsgPack = 2,
    };

    struct RequestParams {
        using callback_t = void(*)(std::span<uint8_t> a_userData, bool a_failed, int32_t a_statusCode, std::span<uint8_t> a_response);
        std::string                 url;
        Method                      method = Method::Get;
        ContentType                 contentType = ContentType::MsgPack;
        bool                        requireAPIHeader = false;
        std::vector<std::string>    headers;
        std::span<uint8_t>          body;
        callback_t                  callback = nullptr;
        std::span<const uint8_t>    userData;
    };

    int32_t SendRequest(const RequestParams& a_params);
}

/*
---@class ace.cHttpRequest : REManagedObject, ace.HttpRequestService.IHttpRequest
---@field _RecvData System.Collections.Generic.List<System.Byte> private / offset: 0x10
---@field _URL System.String|string private / offset: 0x18
---@field _header System.String|string private / offset: 0x20
---@field _data via.network.utility.BinaryInfo private / offset: 0x28
---@field RequestCallback System.Action<System.Boolean, System.Int32, System.String> public / offset: 0x30
---@field RequestCallbackBinary System.Action<System.Boolean, System.Int32, System.Byte[]> public / offset: 0x38
---@field SendCallback System.Action<ace.cHttpRequest> public / offset: 0x40
---@field _statusCode System.Int32|integer private / offset: 0x48
---@field _method via.os.http_client.Method private / offset: 0x4c
---@field _state ace.cHttpRequest.REQUEST_STATE private / offset: 0x50
---@field _pdType ace.cHttpRequest.POSTDATA_TYPE private / offset: 0x54
---@field get_State fun(self): ace.cHttpRequest.REQUEST_STATE public / id: 250877
---@field get_PDType fun(self): ace.cHttpRequest.POSTDATA_TYPE public / id: 250878
---@field get_RecvData fun(self): System.Collections.Generic.List<System.Byte> public / id: 250879
---@field get_URL fun(self): System.String|string public / id: 250880
---@field get_Method fun(self): via.os.http_client.Method public / id: 250881
---@field get_Header fun(self): System.String|string public / id: 250882
---@field get_Data fun(self): via.network.utility.BinaryInfo public / id: 250883
---@field get_StatusCode fun(self): System.Int32|integer public / id: 250884
---@field ace.HttpRequestService.IHttpRequest.setResponseHeader fun(self, arg0: via.network.http_client.ResponseHeader): nil virtual / id: 250885
---@field ace.HttpRequestService.IHttpRequest.addBody fun(self, arg0: via.network.utility.BinaryInfo): nil virtual / id: 250886
---@field ace.HttpRequestService.IHttpRequest.updateStatus fun(self, arg0: ace.cHttpRequest.REQUEST_STATE): nil virtual / id: 250887
---@field ace.HttpRequestService.IHttpRequest.updateStatus fun(self, arg0: ace.cHttpRequest.REQUEST_STATE, arg1: via.network.Error): nil virtual / id: 250888
---@field ace.HttpRequestService.IHttpRequest.clearRequestBody fun(self): nil virtual / id: 250889
---@field ['.ctor'] fun(self): nil
---@field ['.ctor'] fun(self, arg0: System.String|string, arg1: via.os.http_client.Method): nil
---@field destory fun(self): nil public / id: 250892
---@field checkBody fun(self): nil private / id: 250893
---@field addHeader fun(self, arg0: System.String|string): nil public / id: 250894
---@field getHeader fun(self, arg0: System.String|string): System.String|string public / id: 250895
---@field addBody fun(self, arg0: System.String|string, arg1: System.Boolean|boolean): nil public / id: 250896
---@field addBody fun(self, arg0: System.Byte[]|SystemArray, arg1: System.Boolean|boolean): nil public / id: 250897
---@field isBodySend fun(self): System.Boolean|boolean public / id: 250898



---@class app.cNetHttpRequest : ace.cHttpRequest
---@field _type app.cNetHttpRequest.TYPE public / offset: 0x58
---@field ['.ctor'] fun(self, arg0: System.String|string, arg1: app.cNetHttpRequest.TYPE, arg2: via.os.http_client.Method): nil


---@class app.cNetHttpRequest : ace.cHttpRequest
---@field _type app.cNetHttpRequest.TYPE public / offset: 0x58
---@field ['.ctor'] fun(self, arg0: System.String|string, arg1: app.cNetHttpRequest.TYPE, arg2: via.os.http_client.Method): nil

*/