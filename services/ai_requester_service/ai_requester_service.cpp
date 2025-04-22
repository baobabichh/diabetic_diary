#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <iostream>
#include <string>
#include "functions.hpp"
#include "gemini.hpp"

int main(int argc, char *argv[])
{
    if (!Cfg::getInstance().loadFromEnv())
    {
        LOG_ERROR("if(!Cfg::getInstance().loadFromEnv())");
        return 1;
    }

    const auto rabbitmq_user = Cfg::getInstance().getCfgValue("rabbitmq_user");
    const auto rabbitmq_pass = Cfg::getInstance().getCfgValue("rabbitmq_pass");

    const nlohmann::json nutrition_schema = {
        {"type", "object"},
        {"properties", {{"products", {{"type", "array"}, {"items", {{"type", "object"}, {"properties", {{"name", {{"type", "string"}, {"description", "Exact food name identified in the image"}}}, {"grams", {{"type", "integer"}, {"description", "Detected weight in grams"}}}, {"carbs", {{"type", "integer"}, {"description", "Calculated total carbohydrates rounded to the nearest integer"}}}}}, {"required", {"name", "grams", "carbs"}}}}}}}},
        {"required", {"products"}}};

    const std::string prompt =
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

    nlohmann::json res_json{};
    std::string image_file_path = "11.jpg";
    const auto mime_and_base64 = image_to_base64_data_uri(image_file_path);
    if (!gemini::jsonTextImg(prompt, mime_and_base64.mime_type, mime_and_base64.base64_string, nutrition_schema, res_json))
    {
        return 1;
    }

    LOG_INFO(res_json.dump(4));

    return 0;

    try
    {
        const std::string hostname = "localhost";
        const int port = 5672;
        const std::string username = rabbitmq_user;
        const std::string password = rabbitmq_pass;
        const std::string vhost = "/";
        const std::string queue_name = "test_queue";

        AmqpClient::Channel::ptr_t channel = AmqpClient::Channel::Create(hostname, port, username, password, vhost);
        while (true)
        {
            AmqpClient::Envelope::ptr_t envelope = channel->BasicConsumeMessage(channel->BasicConsume(queue_name, "", true, true));

            if (envelope)
            {
                AmqpClient::BasicMessage::ptr_t message = envelope->Message();
                std::string body = message->Body();

                channel->BasicAck(envelope);
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