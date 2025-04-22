#include "nlohmann/json.hpp"
#include <drogon/orm/Exception.h>
#include <drogon/drogon.h>
#include <string>
#include "functions.hpp"

using namespace drogon;

inline void responseWithError(std::function<void (const HttpResponsePtr &)> &callback, const nlohmann::json& object)
{
    auto response = HttpResponse::newHttpResponse();
    response->setStatusCode(HttpStatusCode::k404NotFound);
    response->setBody(object.dump());
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    callback(response);
}

inline void responseWithErrorMsg(std::function<void (const HttpResponsePtr &)> &callback, const std::string& msg)
{
    nlohmann::json object{};
    object["Msg"] = msg;

    responseWithError(callback, object);
}

inline void responseWithSuccess(std::function<void (const HttpResponsePtr &)> &callback, const nlohmann::json& object)
{
    auto response = HttpResponse::newHttpResponse();
    response->setStatusCode(HttpStatusCode::k200OK);
    response->setBody(object.dump());
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    callback(response);
}

inline void responseWithSuccessMsg(std::function<void (const HttpResponsePtr &)> &callback, const std::string& msg)
{
    nlohmann::json object{};
    object["Msg"] = msg;
    responseWithSuccess(callback, object);
}