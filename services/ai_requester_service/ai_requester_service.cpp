#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <iostream>
#include <string>
#include "functions.hpp"
#include "gemini.hpp"
#include "openai.hpp"

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
    driver = sql::mysql::get_mysql_driver_instance();

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
                LOG_INFO("PROCESSING");
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
                            sql::Connection *con{nullptr};

                            const auto scope_exit = makeScopeExit(
                                [&]()
                                {
                                    if (pstmt)
                                        delete pstmt;
                                    if (res)
                                        delete res;
                                    if (con)
                                        delete con;
                                });
                            
                            con = driver->connect("127.0.0.1:3306", db_user, db_pass);
                            con->setSchema("dd");

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
                        if (!gemini::jsonTextImg("gemini-2.0-flash-exp", Prompts::prompt, mime_and_base64.mime_type, mime_and_base64.base64_string, Prompts::nutrition_schema, res_json))
                        {
                            LOG_ERROR("if (!gemini::jsonTextImg(Prompts::prompt, mime_and_base64.mime_type, mime_and_base64.base64_string, Prompts::nutrition_schema, res_json))");
                            channel->BasicReject(envelope, true);
                            return;
                        }

                        const auto get_float_smart = [](const nlohmann::json& obj, const std::string& key) -> std::optional<float>
                        {
                            float res{0.0f};
                            if(obj.count(key) && obj[key].is_number_float())
                            {
                                res = obj[key].get<float>();
                            }
                            else if(obj.count(key) && obj[key].is_string())
                            {
                                res = stringToFloat(obj[key].get<std::string>());
                            }
                            else if(obj.count(key) && obj[key].is_number())
                            {
                                res = obj[key].get<int>();
                            }
                            else
                            {
                                return std::nullopt;
                            }
                            return res;
                        };

                        if(res_json.count("products") && res_json["products"].is_array())
                        {
                            for(auto& product : res_json["products"])
                            {   
                                auto carbs_opt = get_float_smart(product, "carbs");
                                auto grams_opt = get_float_smart(product, "grams");

                                if(grams_opt.value() <= 0.0001f || carbs_opt.value() <= 0.0001f)
                                {
                                    product["ratio"] = float{0};
                                }
                                else
                                {
                                    product["ratio"] = float{carbs_opt.value() / grams_opt.value() * 100.0f};
                                }
                            }
                        }

                        {
                            sql::PreparedStatement *pstmt{nullptr};
                            sql::ResultSet *res{nullptr};
                            sql::Connection *con{nullptr};

                            const auto scope_exit = makeScopeExit(
                                [&]()
                                {
                                    if (pstmt)
                                        delete pstmt;
                                    if (res)
                                        delete res;
                                    if (con)
                                        delete con;
                                });
                            
                            con = driver->connect("127.0.0.1:3306", db_user, db_pass);
                            con->setSchema("dd");

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