#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <iostream>
#include <string>
#include "functions.hpp"
#include "gemini.hpp"

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <iostream>

int main(int argc, char *argv[])
{
    if (!Cfg::getInstance().loadFromEnv())
    {
        LOG_ERROR("if(!Cfg::getInstance().loadFromEnv())");
        return 1;
    }

    const auto rabbitmq_user = Cfg::getInstance().getCfgValue("rabbitmq_user");
    const auto rabbitmq_pass = Cfg::getInstance().getCfgValue("rabbitmq_pass");
    const auto db_user = Cfg::getInstance().getCfgValue("db_user");
    const auto db_pass = Cfg::getInstance().getCfgValue("db_pass");
    const auto photos_folder_path = Cfg::getInstance().getCfgValue("photos_storage_absolute_path");

    sql::mysql::MySQL_Driver *driver{nullptr};
    sql::Connection *con{nullptr};

    const auto con_scope_exit = makeScopeExit(
        [&]()
        {
            if (con)
            {
                delete con;
            }
        });

    driver = sql::mysql::get_mysql_driver_instance();
    con = driver->connect("127.0.0.1:3306", db_user, db_pass);
    con->setSchema("dd");

    static const nlohmann::json nutrition_schema = {
        {"type", "object"},
        {"properties", {{"products", {{"type", "array"}, {"items", {{"type", "object"}, {"properties", {{"name", {{"type", "string"}, {"description", "Exact food name identified in the image"}}}, {"grams", {{"type", "integer"}, {"description", "Detected weight in grams"}}}, {"carbs", {{"type", "integer"}, {"description", "Calculated total carbohydrates rounded to the nearest integer"}}}}}, {"required", {"name", "grams", "carbs"}}}}}}}},
        {"required", {"products"}}};

    static const std::string prompt =
        R"(You are a nutrition‐analysis assistant. Given any input image of foods:

1. Detect every unique food item and its weight in grams.
2. Use surrounding and image depth to determine amount of products.
3. Split products to the smallest parts.
4. For each item, retrieve the standard carbohydrate content per 100 g from a reliable nutrition database. 
5. Calculate the total carbohydrates for each item as.
6. Output ONLY valid JSON in the following format:

{
  "products": [
    {
      "name":    "<exact food name>",
      "grams":   <detected weight in grams as an integer>,
      "carbs":   <calculated total carbohydrates rounded to the nearest integer>
    }
  ]
})";

    try
    {
        const std::string hostname = "localhost";
        const int port = 5672;
        const std::string username = rabbitmq_user;
        const std::string password = rabbitmq_pass;
        const std::string vhost = "/";

        AmqpClient::Channel::ptr_t channel = AmqpClient::Channel::Create(hostname, port, username, password, vhost);
        std::string consumer_tag = channel->BasicConsume("recognize_food", "", true, false, false, 1);

        while (true)
        {
            AmqpClient::Envelope::ptr_t envelope = channel->BasicConsumeMessage(consumer_tag);
            if (envelope)
            {
                [&]()
                {
                    AmqpClient::BasicMessage::ptr_t message = envelope->Message();
                    std::string body = message->Body();

                    if (!nlohmann::json::accept(body))
                    {
                        LOG_ERROR("if(!nlohmann::json::accept(body))");
                        channel->BasicReject(envelope, true);
                        return;
                    }
                    nlohmann::json obj = nlohmann::json::parse(body);

                    if (!obj.count("FoodRecognitionID") || !obj["FoodRecognitionID"].is_string())
                    {
                        LOG_ERROR("if(!obj.count(\"FoodRecognitionID\") || !obj[\"FoodRecognitionID\"].is_string())");
                        channel->BasicReject(envelope, true);
                        return;
                    }
                    const std::string req_id = obj["FoodRecognitionID"].get<std::string>();

                    try
                    {
                        std::string image_path{};
                        size_t rows_count{};

                        {
                            sql::PreparedStatement *pstmt{nullptr};
                            sql::ResultSet *res{nullptr};

                            const auto scope_exit = makeScopeExit(
                                [&]()
                                {
                                    if (pstmt)
                                        delete pstmt;
                                    if (res)
                                        delete res;
                                });

                            pstmt = con->prepareStatement("select ImagePath from FoodRecognitions where id = ?");
                            pstmt->setString(1, req_id);
                            res = pstmt->executeQuery();

                            while (res->next())
                            {
                                ++rows_count;
                                image_path = res->getString("ImagePath");
                            }
                        }

                        if (rows_count == 0)
                        {
                            LOG_ERROR("if(image_path == 0)");
                            channel->BasicReject(envelope, true);
                            return;
                        }

                        if (image_path.empty())
                        {
                            LOG_ERROR("if(image_path.empty())");
                            channel->BasicReject(envelope, true);
                            return;
                        }

                        const auto mime_and_base64 = image_to_base64_data_uri(image_path);
                        if (mime_and_base64.base64_string.empty() || mime_and_base64.mime_type.empty())
                        {
                            LOG_ERROR("if(mime_and_base64.base64_string.empty() || mime_and_base64.mime_type.empty())");
                            channel->BasicReject(envelope, true);
                            return;
                        }

                        nlohmann::json res_json{};
                        if (!gemini::jsonTextImg(prompt, mime_and_base64.mime_type, mime_and_base64.base64_string, nutrition_schema, res_json))
                        {
                            LOG_ERROR("if (!gemini::jsonTextImg(prompt, mime_and_base64.mime_type, mime_and_base64.base64_string, nutrition_schema, res_json))");
                            channel->BasicReject(envelope, true);
                            return;
                        }

                        {
                            sql::PreparedStatement *pstmt{nullptr};
                            sql::ResultSet *res{nullptr};

                            const auto scope_exit = makeScopeExit(
                                [&]()
                                {
                                    if (pstmt)
                                        delete pstmt;
                                    if (res)
                                        delete res;
                                });

                            pstmt = con->prepareStatement("update FoodRecognitions set Status = ?, ResultJson = ? where id = ?");
                            pstmt->setString(1, FoodRecognitions::Status::Done);
                            pstmt->setString(2, res_json.dump());
                            pstmt->setString(3, req_id);
                            res = pstmt->executeQuery();
                            channel->BasicAck(envelope);
                            return;
                        }
                    }
                    catch (sql::SQLException &e)
                    {
                        LOG_ERROR("SQLException: " + e.what());
                        LOG_ERROR("SQLState: " + e.getSQLStateCStr());
                        channel->BasicReject(envelope, true);
                        return;
                    }
                }();
            }
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(e.what());
        return 1;
    }

    return 0;
}