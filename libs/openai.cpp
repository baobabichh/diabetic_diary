#include "openai.hpp"
#include <curl/curl.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp)
{
    size_t total_size = size * nmemb;
    userp->append((char *)contents, total_size);
    return total_size;
}

bool openai::jsonTextImg(const std::string& model_type, const std::string &prompt, const std::string &mime_type, const std::string &base64_image, const nlohmann::json &response_schema, nlohmann::json &res_json)
{
    const std::string api_key = Cfg::getInstance().getCfgValue("openai_api_key");

    struct curl_slist *headers = NULL;
    CURL *curl = NULL;
    CURLcode res;
    std::string response_string;
    bool success = false;

    const auto cleanup = [&]()
    {
        if (headers)
            curl_slist_free_all(headers);
        if (curl)
            curl_easy_cleanup(curl);
    };

    curl = curl_easy_init();
    if (!curl)
    {
        cleanup();
        res_json = {{"function_error", "if(!curl)"}};
        LOG_ERROR(res_json.dump());
        return false;
    }

    std::string url = "https://api.openai.com/v1/chat/completions";
    
    std::string system_message = "You are a helpful assistant that returns JSON responses only. ";
    system_message += "Your response must follow this JSON schema: " + response_schema.dump();
    
    nlohmann::json messages = nlohmann::json::array();
    messages.push_back({
        {"role", "system"},
        {"content", system_message}
    });
    
    nlohmann::json user_message = {
        {"role", "user"},
        {"content", nlohmann::json::array()}
    };
    
    user_message["content"].push_back({
        {"type", "text"},
        {"text", prompt}
    });
    
    user_message["content"].push_back({
        {"type", "image_url"},
        {"image_url", {
            {"url", "data:" + mime_type + ";base64," + base64_image}
        }}
    });
    
    messages.push_back(user_message);
    
    nlohmann::json request_json = {
        {"model", model_type},
        {"messages", messages},
        {"response_format", {{"type", "json_object"}}}
    };

    std::string json_str = request_json.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    std::string auth_header = "Authorization: Bearer " + api_key;
    headers = curl_slist_append(headers, auth_header.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        cleanup();
        res_json = {{"function_error", "curl_easy_perform failed: " + std::string(curl_easy_strerror(res))}};
        LOG_ERROR(res_json.dump());
        return false;
    }

    if (!nlohmann::json::accept(response_string))
    {
        cleanup();
        res_json = {{"function_error", "Response is not valid JSON"}};
        LOG_ERROR(res_json.dump());
        return false;
    }

    nlohmann::json full_response = nlohmann::json::parse(response_string);
    
    if (full_response.contains("choices") && !full_response["choices"].empty())
    {
        if (full_response["choices"][0].contains("message") &&
            full_response["choices"][0]["message"].contains("content"))
        {
            auto& content = full_response["choices"][0]["message"]["content"];
            if (content.is_string())
            {
                std::string content_str = content.get<std::string>();
                if (!nlohmann::json::accept(content_str))
                {
                    cleanup();
                    res_json = {{"function_error", "Response content is not valid JSON"}};
                    LOG_ERROR(res_json.dump());
                    return false;
                }
                
                res_json = nlohmann::json::parse(content_str);
                cleanup();
                return true;
            }
        }
    }
    
    cleanup();
    res_json = {{"function_error", "Could not parse structured JSON from OpenAI response"}};
    LOG_ERROR(res_json.dump());
    LOG_ERROR(full_response.dump());
    return false;
}