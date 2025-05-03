#pragma once
#include "functions.hpp"

namespace gemini
{
    bool jsonTextImg(const std::string& model_type, const std::string& promt, const std::string& mime_type, const std::string& base64_image, const nlohmann::json& response_schema, nlohmann::json& res_json);
}