#include "AuthenticatorController.h"

void AuthenticatorController::register_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    const std::string &email = req->getParameter("email");
    const std::string &password = req->getParameter("password");

    if (email.size() <= 100 && email.size() > 0 && password.size() >= 8 && password.size() <= 100)
    {
    }
    else
    {
        responseWithErrorMsg(callback, "Incorrect email or password.");
        return;
    }

    auto client = drogon::app().getDbClient("dd");
    std::string uuid{};

    try
    {
        {
            static const std::string query = "select count(*) from Users where Email = ?";
            const auto result = client->execSqlSync(query, email);
            if (result[0][0].as<size_t>() >= 1)
            {
                responseWithErrorMsg(callback, "There is already user with such email.");
                return;
            }
        }

        while (true)
        {
            uuid = drogon::utils::getUuid();
            static const std::string query = "select count(*) from Users where UUID = ?";
            const auto result = client->execSqlSync(query, uuid);
            if (result[0][0].as<size_t>() <= 0)
            {
                break;
            }
        }

        {
            static const std::string query = "insert into Users (Email, Password, UUID) values (?, ?, ?)";
            client->execSqlSync(query, email, password, uuid);
        }

        responseWithSuccess(callback, {{"UUID", uuid}});
        return;
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        LOG_ERROR(e.base().what());
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }
}

void AuthenticatorController::login_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    const std::string &email = req->getParameter("email");
    const std::string &password = req->getParameter("password");
    LOG_INFO("login |" + email + "|" + password + "|");

    if (email.size() <= 100 && email.size() > 0 && password.size() >= 8 && password.size() <= 100)
    {
    }
    else
    {
        responseWithErrorMsg(callback, "Incorrect email or password.");
        return;
    }

    auto client = drogon::app().getDbClient("dd");

    try
    {
        static const std::string query = "select UUID from Users where Email = ? and Password = ?";
        const auto result = client->execSqlSync(query, email, password);
        if (result.empty())
        {
            responseWithErrorMsg(callback, "Wrong email or password");
            return;
        }

        std::string uuid = result[0]["UUID"].as<std::string>();

        responseWithSuccess(callback, {{"UUID", uuid}});
        return;
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        LOG_ERROR(e.base().what());
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }
}

void AuthenticatorController::add_record(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    const auto user_identity = getUserIdentity(req);
    if (!user_identity.isCorrect())
    {
        responseWithNotLoggedIn(callback);
        return;
    }

    std::string time_coefficient = req->getParameter("time_coefficient");
    std::string sport_coefficient = req->getParameter("sport_coefficient");
    std::string personal_coefficient = req->getParameter("personal_coefficient");
    std::string insulin = req->getParameter("insulin");
    std::string carbohydrates = req->getParameter("carbohydrates");
    std::string request_id = req->getParameter("request_id");

    if (time_coefficient.empty())
    {
        time_coefficient = "1.0";
    }
    if (sport_coefficient.empty())
    {
        sport_coefficient = "1.0";
    }
    if (personal_coefficient.empty())
    {
        personal_coefficient = "1.0";
    }
    if (insulin.empty())
    {
        insulin = "0.0";
    }
    if (carbohydrates.empty())
    {
        carbohydrates = "0.0";
    }
    if (request_id.empty())
    {
        request_id = "NULL";
    }

    if (!isFloat(time_coefficient))
    {
        responseWithErrorMsg(callback, "time_coefficient is not NULL and not float.");
        return;
    }
    if (!isFloat(sport_coefficient))
    {
        responseWithErrorMsg(callback, "sport_coefficient is not NULL and not float.");
        return;
    }
    if (!isFloat(personal_coefficient))
    {
        responseWithErrorMsg(callback, "personal_coefficient is not NULL and not float.");
        return;
    }
    if (!isFloat(insulin))
    {
        responseWithErrorMsg(callback, "insulin is not NULL and not float.");
        return;
    }
    if (!isFloat(carbohydrates))
    {
        responseWithErrorMsg(callback, "carbohydrates is not NULL and not float.");
        return;
    }
    if(request_id != "NULL" && stringToSizeT(request_id) <= 0)
    {
        responseWithErrorMsg(callback, "Incorrect request_id.");
        return;
    }

    auto client = drogon::app().getDbClient("dd");

    try
    {
        const std::string query =
            "insert into Records "
            "(UserID,FoodRecognitionID,Insulin,Carbohydrates,TimeCoefficient,SportCoefficient,PersonalCoefficient) "
            "values (?, " + request_id + ", ?, ?, ?, ?, ?)";
        const auto result = client->execSqlSync(
            query, std::to_string(user_identity.id), insulin, carbohydrates, time_coefficient, sport_coefficient, personal_coefficient);

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

void AuthenticatorController::get_record_ids(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    const auto user_identity = getUserIdentity(req);
    if (!user_identity.isCorrect())
    {
        responseWithNotLoggedIn(callback);
        return;
    }

    auto client = drogon::app().getDbClient("dd");

    try
    {
        nlohmann::json res_json = nlohmann::json::array();

        static const std::string query = "select ID from Records where UserID = ? order by ID desc";
        const auto result = client->execSqlSync(query, std::to_string(user_identity.id));
        for(auto& res : result)
        {
            res_json.push_back(res["ID"].as<std::string>());
        }

        responseWithSuccess(callback, res_json);
        return;
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        LOG_ERROR(e.base().what());
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }
}

void AuthenticatorController::get_records_by_ids(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const
{
    const auto user_identity = getUserIdentity(req);
    if (!user_identity.isCorrect())
    {
        responseWithNotLoggedIn(callback);
        return;
    }

    const std::string& ids_str = req->getParameter("ids");
    if(ids_str.empty())
    {
        responseWithErrorMsg(callback, "ids is empty.");
        return;
    }

    auto client = drogon::app().getDbClient("dd");

    try
    {
        nlohmann::json res_json = nlohmann::json::array();

        const std::string query = "select * from Records where UserID = ? and ID in (" + ids_str + ")";
        const auto result = client->execSqlSync(query, std::to_string(user_identity.id));
        for(auto& res : result)
        {
            nlohmann::json tmp_obj{};

            tmp_obj["ID"] = res["ID"].as<std::string>();
            tmp_obj["UserID"] = res["UserID"].as<std::string>();
            tmp_obj["FoodRecognitionID"] = res["FoodRecognitionID"].as<std::string>();
            tmp_obj["Insulin"] = res["Insulin"].as<std::string>();
            tmp_obj["Carbohydrates"] = res["Carbohydrates"].as<std::string>();
            tmp_obj["TimeCoefficient"] = res["TimeCoefficient"].as<std::string>();
            tmp_obj["SportCoefficient"] = res["SportCoefficient"].as<std::string>();
            tmp_obj["PersonalCoefficient"] = res["PersonalCoefficient"].as<std::string>();
            tmp_obj["CreateTS"] = res["CreateTS"].as<std::string>();


            res_json.push_back(tmp_obj);
        }

        responseWithSuccess(callback, res_json);
        return;
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        LOG_ERROR(e.base().what());
        responseWithErrorMsg(callback, "Internal server error.");
        return;
    }
}