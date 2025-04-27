#include "FoodRecognitionController.h"

void FoodRecognitionController::recognize_food(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    const auto user_identity = getUserIdentity(req);
    if (!user_identity.isCorrect())
    {
        responseWithNotLoggedIn(callback);
        return;
    }

    bool is_error{false};
    auto client = drogon::app().getDbClient("dd");
    std::string request_id{};
    std::string full_photo_path{};

    const auto scope_exit = makeScopeExit(
        [&]()
        {
            if(!is_error)
            {
                return;
            }
            auto client = drogon::app().getDbClient("dd");
            if (request_id.size())
            {
                try
                {
                    static const std::string query = "delete from FoodRecognitions where id = ?";
                    client->execSqlSync(query, request_id);
                }
                catch (const drogon::orm::DrogonDbException &e)
                {
                    LOG_ERROR(e.base().what());
                    responseWithErrorMsg(callback, "Internal server error.");
                    return;
                }
            }

            if (full_photo_path.size())
            {
                try
                {
                    std::filesystem::remove(full_photo_path);
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR(e.what());
                }
            }
        });

    static const std::string photos_folder_path = Cfg::getInstance().getCfgValue("photos_storage_absolute_path");
    const auto &base64_string = req->getParameter("base64_string");
    const auto &mime_type = req->getParameter("mime_type");

    if (base64_string.empty() || base64_string.empty())
    {
        is_error = true;
        responseWithErrorMsg(callback, "base64_string or mime_type is empty.");
        return;
    }

    const auto decoded_image_data = base64_decode(base64_string);
    if(decoded_image_data.empty())
    {
        is_error = true;
        LOG_ERROR("if(decoded_image_data.empty())");
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }

    const std::string photo_ext = ext_of_mime_type(mime_type);
    if(photo_ext.empty())
    {
        is_error = true;
        LOG_ERROR("if(photo_ext.empty())");
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }
    
    try
    {
        {
            static const std::string query = "insert into FoodRecognitions (UserID, Status) values (?, ?)";
            client->execSqlSync(query, std::to_string(user_identity.id), FoodRecognitions::Status::Waiting);
        }

        {
            static const std::string query = "SELECT LAST_INSERT_ID()";
            const auto result = client->execSqlSync(query);
            size_t request_id_int = result[0][0].as<size_t>();
            if (request_id_int == 0)
            {
                LOG_ERROR("if (request_id_int == 0)");
                responseWithError(callback, "Internal server error.");
                return;
            }

            request_id = std::to_string(request_id_int);
        }
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        is_error = true;
        LOG_ERROR(e.base().what());
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }



    full_photo_path = photos_folder_path + "/" + request_id + "." + photo_ext;
    if(!writeBytesToFile(full_photo_path, decoded_image_data))
    {
        is_error = true;
        LOG_ERROR("if(!writeBytesToFile(full_photo_path, decoded_image_data))");
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }

    try
    {
        static const std::string query = "update FoodRecognitions set ImagePath = ? where id = ?";
        client->execSqlSync(query, full_photo_path, request_id);
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        is_error = true;
        LOG_ERROR(e.base().what());
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }

    nlohmann::json food_obj{};
    food_obj["FoodRecognitionID"] = request_id;

    if(!getRabbitMqPublisher().publish("recognize_food", food_obj.dump()))
    {
        is_error = true;
        LOG_ERROR("failed to publish");
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }

    is_error = false;
    responseWithSuccess(callback, food_obj);
    return;
}

void FoodRecognitionController::edit_result(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    LOG_INFO("here");
    const auto user_identity = getUserIdentity(req);
    if (!user_identity.isCorrect())
    {
        responseWithNotLoggedIn(callback);
        return;
    }

    auto client = drogon::app().getDbClient("dd");

    const std::string req_id = req->getParameter("request_id");
    const std::string new_json_str = req->getParameter("new_json");
    LOG_INFO("new_json_str: " + new_json_str);

    if (req_id.empty() || stringToSizeT(req_id) <= 0)
    {
        responseWithErrorMsg(callback, "request_id is empty.");
        return;
    }

    if (new_json_str.empty())
    {
        responseWithErrorMsg(callback, "new_json is empty.");
        return;
    }

    if (!nlohmann::json::accept(new_json_str))
    {
        responseWithErrorMsg(callback, "new_json is not complete json.");
        return;
    }

    nlohmann::json new_json = nlohmann::json::parse(new_json_str);

    try
    {
        static const std::string query = "update FoodRecognitions set ResultJson = ? where id = ?";
        const auto result = client->execSqlSync(query, new_json.dump(), req_id);

        responseWithSuccess(callback, "{}");
        return;
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        LOG_ERROR(e.base().what());
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }
}

void FoodRecognitionController::get_status(const HttpRequestPtr & req, std::function<void(const HttpResponsePtr&)>&& callback) const
{
    const auto user_identity = getUserIdentity(req);
    if (!user_identity.isCorrect())
    {
        responseWithNotLoggedIn(callback);
        return;
    }

    auto client = drogon::app().getDbClient("dd");

    const std::string req_id = req->getParameter("request_id");
    if(req_id.empty() || stringToSizeT(req_id) <= 0)
    {
        responseWithErrorMsg(callback, "req_id is empty.");
        return;
    }

    try
    {
        static const std::string query = "select Status from FoodRecognitions where id = ?";
        const auto result = client->execSqlSync(query, req_id);

        if(result.size() <= 0)
        {
            responseWithErrorMsg(callback, "No such reqeust.");
            return;
        }

        nlohmann::json res_json{};

        for(auto& row : result)
        {
            std::string status_int = row["Status"].as<std::string>();
            if(FoodRecognitions::Status::Waiting == status_int)
            {
                res_json["Status"] = "Waiting";
            }
            else if(FoodRecognitions::Status::Processing == status_int)
            {
                res_json["Status"] = "Processing";
            }
            else if(FoodRecognitions::Status::Error == status_int)
            {
                res_json["Status"] = "Error";
            }
            else if(FoodRecognitions::Status::Done == status_int)
            {
                res_json["Status"] = "Done";
            }
            else
            {
                responseWithErrorMsg(callback, "Internal server error.");
                return;
            }
            responseWithSuccess(callback, res_json);
            return;
        }
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        LOG_ERROR(e.base().what());
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }
}

void FoodRecognitionController::get_result(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    const auto user_identity = getUserIdentity(req);
    if (!user_identity.isCorrect())
    {
        responseWithNotLoggedIn(callback);
        return;
    }

    auto client = drogon::app().getDbClient("dd");

    const std::string req_id = req->getParameter("request_id");
    if(req_id.empty() || stringToSizeT(req_id) <= 0)
    {
        responseWithErrorMsg(callback, "request_id is empty.");
        return;
    }

    try
    {
        static const std::string query = "select ResultJson from FoodRecognitions where id = ? and Status = ?";
        const auto result = client->execSqlSync(query, req_id, FoodRecognitions::Status::Done);

        if(result.size() <= 0)
        {
            responseWithErrorMsg(callback, "No such reqeust.");
            return;
        }

        nlohmann::json res_json{};

        for(auto& row : result)
        {
            std::string result_json_str = row["ResultJson"].as<std::string>();

            if(!nlohmann::json::accept(result_json_str))
            {
                responseWithErrorMsg(callback, "Internal server error.");
                return;
            }
            auto info_json = nlohmann::json::parse(result_json_str);
            responseWithSuccess(callback, info_json);
            return;
        }
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        LOG_ERROR(e.base().what());
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }
}
