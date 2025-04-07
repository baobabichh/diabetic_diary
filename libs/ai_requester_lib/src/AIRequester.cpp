#include "AIRequester.hpp"

void OpenAIRequest::addImage(const std::string &base64_img)
{
    nlohmann::json content{};
    content["type"] = "image_url";

    nlohmann::json json{};
    json["role"] = "user";
    

    json["content"] = base64_img;

    _messages.push_back(json);
}

void OpenAIRequest::addText(const std::string &text)
{
    nlohmann::json json{};
    json["role"] = "user";
    json["content"] = text;

    _messages.push_back(json);
}

std::string OpenAIRequest::getRequest() const
{
    return std::string();
}
