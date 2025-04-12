#include "nlohmann/json.hpp"
#include <drogon/orm/Exception.h>
#include <drogon/drogon.h>
#include <string>

using namespace drogon;

inline const std::string createStatusJson(const std::string& type, const std::string& message, const std::vector<std::pair<std::string, std::string>>& add_info = {})
{
    nlohmann::json object{};
    for(const auto& [key, value] : add_info)
    {
        object[key] = value;
    }

    return object.dump();
}

inline void responseWithError(std::function<void (const HttpResponsePtr &)> &callback, const std::string& message, const std::vector<std::pair<std::string, std::string>>& add_info = {})
{
    auto response = HttpResponse::newHttpResponse();
    response->setStatusCode(HttpStatusCode::k200OK);
    response->setBody(createStatusJson("Error", message, add_info));
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    callback(response);
}

inline void responseWithSuccess(std::function<void (const HttpResponsePtr &)> &callback, const std::string& message, const std::vector<std::pair<std::string, std::string>>& add_info = {})
{
    auto response = HttpResponse::newHttpResponse();
    response->setStatusCode(HttpStatusCode::k200OK);
    response->setBody(createStatusJson("Success", message, add_info));
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    callback(response);
}