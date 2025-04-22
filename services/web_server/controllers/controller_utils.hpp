#pragma once

#include "nlohmann/json.hpp"
#include <drogon/orm/Exception.h>
#include <drogon/drogon.h>
#include <string>
#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include "functions.hpp"
#include <mutex>

using namespace drogon;

struct FoodRecognitions
{
    struct Status
    {
        inline static const std::string Waiting{"1"};
        inline static const std::string Processing{"2"};
        inline static const std::string Done{"3"};
        inline static const std::string Error{"4"};
    };
};

inline void responseWithError(std::function<void(const HttpResponsePtr &)> &callback, const nlohmann::json &object)
{
    auto response = HttpResponse::newHttpResponse();
    response->setStatusCode(HttpStatusCode::k404NotFound);
    response->setBody(object.dump());
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    callback(response);
}

inline void responseWithErrorMsg(std::function<void(const HttpResponsePtr &)> &callback, const std::string &msg)
{
    nlohmann::json object{};
    object["Msg"] = msg;

    responseWithError(callback, object);
}

inline void responseWithSuccess(std::function<void(const HttpResponsePtr &)> &callback, const nlohmann::json &object)
{
    auto response = HttpResponse::newHttpResponse();
    response->setStatusCode(HttpStatusCode::k200OK);
    response->setBody(object.dump());
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    callback(response);
}

inline void responseWithSuccessMsg(std::function<void(const HttpResponsePtr &)> &callback, const std::string &msg)
{
    nlohmann::json object{};
    object["Msg"] = msg;
    responseWithSuccess(callback, object);
}

inline void responseWithNotLoggedIn(std::function<void(const HttpResponsePtr &)> &callback)
{
    responseWithErrorMsg(callback, "You are not logged in.");
}

struct UserIdentity
{
    size_t id{0};
    std::string uuid{};

    constexpr bool isCorrect() const
    {
        return id != 0 && uuid.size();
    }
};

inline UserIdentity getUserIdentity(const HttpRequestPtr &req)
{
    UserIdentity user_identity{};

    const std::string &uuid = req->getParameter("uuid");
    if (uuid.empty())
    {
        return UserIdentity{};
    }

    auto client = drogon::app().getDbClient("dd");
    try
    {
        static const std::string query = "select ID from Users where UUID = ?";
        const auto result = client->execSqlSync(query, uuid);
        if (result.empty())
        {
            return UserIdentity{};
        }

        user_identity.id = result[0]["ID"].as<size_t>();
        user_identity.uuid = uuid;

        return user_identity;
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
        return UserIdentity{};
    }
}

inline size_t stringToSizeT(const std::string &str)
{
    try
    {
        return std::stoull(str);
    }
    catch (...)
    {
        return 0;
    }
}

class RabbitMqPublisher
{
public:
    inline RabbitMqPublisher()
    {
        try
        {
            const auto user = Cfg::getInstance().getCfgValue("rabbitmq_user");
            const auto pass = Cfg::getInstance().getCfgValue("rabbitmq_pass");

            const std::string hostname = "localhost";
            const int port = 5672;
            const std::string username = user;
            const std::string password = pass;
            const std::string vhost = "/";
            const std::string queue_name = "test_queue";

            _channel = AmqpClient::Channel::Create(hostname, port, username, password, vhost);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR(e.what());
        }
    }

    inline bool publish(const std::string &queue, const std::string &message)
    {
        try
        {
            std::lock_guard{_channel_mut};
            AmqpClient::BasicMessage::ptr_t msg = AmqpClient::BasicMessage::Create(message);
            _channel->BasicPublish("", queue, msg);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR(e.what());
            return false;
        }
        return true;
    }

private:
    AmqpClient::Channel::ptr_t _channel{};
    std::mutex _channel_mut{};
};

inline RabbitMqPublisher &getRabbitMqPublisher()
{
    static RabbitMqPublisher s{};
    return s;
}