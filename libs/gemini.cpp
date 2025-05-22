#include "gemini.hpp"
#include <curl/curl.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp)
{
    size_t total_size = size * nmemb;
    userp->append((char *)contents, total_size);
    return total_size;
}

bool gemini::jsonTextImg(const std::string& model_type, const std::string &prompt, const std::string &mime_type, const std::string &base64_image, const nlohmann::json &response_schema, nlohmann::json &res_json)
{
    const std::string api_key = Cfg::getInstance().getCfgValue("gemini_api_key");

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

    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/" + model_type + ":generateContent?key=" + api_key;
    
    nlohmann::json generation_config = {
        {"response_mime_type", "application/json"},
        {"response_schema", response_schema}};

    nlohmann::json request_json = {
        {"contents", {{{"parts", {{{"text", prompt}}, {{"inline_data", {{"mime_type", mime_type}, {"data", base64_image}}}}}}}}},
        {"generation_config", generation_config}};

    std::string json_str = request_json.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        cleanup();
        res_json = {{"function_error", "if (res != CURLE_OK)"}};
        LOG_ERROR(res_json.dump());
        return false;
    }

    if (!nlohmann::json::accept(response_string))
    {
        cleanup();
        res_json = {{"function_error", "if (!nlohmann::json::accept(response_string))"}};
        LOG_ERROR(res_json.dump());
        return false;
    }

    nlohmann::json full_response = nlohmann::json::parse(response_string);
    if (full_response.contains("candidates") && !full_response["candidates"].empty())
    {
        if (full_response["candidates"][0].contains("content") &&
            full_response["candidates"][0]["content"].contains("parts") &&
            !full_response["candidates"][0]["content"]["parts"].empty() &&
            full_response["candidates"][0]["content"]["parts"][0].contains("text") &&
            full_response["candidates"][0]["content"]["parts"][0]["text"].is_string())
        {
            auto& correct_resp = full_response["candidates"][0]["content"]["parts"][0]["text"];
            if(!nlohmann::json::accept(correct_resp.get<std::string>()))
            {
                res_json = {{"function_error", "if(!nlohmann::json::accept(correct_resp.get<std::string>()))"}};
                LOG_ERROR(res_json.dump());
            }
            res_json = nlohmann::json::parse(correct_resp.get<std::string>());
            cleanup();
            return true;
        }
        else
        {
            res_json = {{"function_error", "Unexpected response format"}};
            LOG_ERROR(res_json.dump());
        }
    }
    else
    {
        cleanup();
        res_json = {{"function_error", "No candidates in response"}};
        LOG_ERROR(res_json.dump());
        LOG_ERROR(full_response.dump());
        return false;
    }

    cleanup();
    return false;
}