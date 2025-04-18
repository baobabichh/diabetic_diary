#include "AuthenticatorController.h"

// Add definition of your processing function here

void AuthenticatorController::register_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, double p1, int p2) const
{

    const std::string &email = req->getParameter("email");
    const std::string &password = req->getParameter("password");

    if (email.size() <= 100 && email.size() > 0 && password.size() >= 8 && password.size() <= 100)
    {
    }
    else
    {
        responseWithError(callback, "Incorrect email or password.");
        return;
    }

    auto client = drogon::app().getDbClient("food_ai");
    std::string uuid{};

    try
    {
        {
            static const std::string query = "select count(*) from Users where Email = ?";
            const auto result = client->execSqlSync(query, email);
            if (result[0][0].as<size_t>() >= 1)
            {
                responseWithError(callback, "There is already user with such email.");
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

        responseWithSuccess(callback, "User registered.", {{"UUID", uuid}});
        return;
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        responseWithError(callback, "Internal server error.", {{"Exception", e.base().what()}});
        return;
    }
}

void AuthenticatorController::login_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, double p1, int p2) const
{
}
