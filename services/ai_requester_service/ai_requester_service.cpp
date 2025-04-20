#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <iostream>
#include <string>
#include "functions.hpp"

int main(int argc, char *argv[])
{
    if (!Cfg::getInstance().loadFromEnv())
    {
        LOG_ERROR("if(!Cfg::getInstance().loadFromEnv())");
        return 1;
    }

    const auto rabbitmq_user = Cfg::getInstance().getCfgValue("rabbitmq_user");
    const auto rabbitmq_pass = Cfg::getInstance().getCfgValue("rabbitmq_pass");

    if (rabbitmq_user.empty() || rabbitmq_pass.empty())
    {
        LOG_ERROR("if(rabbitmq_user.empty() || rabbitmq_pass.empty())");
        return 1;
    }

    try
    {
        const std::string hostname = "localhost";
        const int port = 5672;
        const std::string username = rabbitmq_user;
        const std::string password = rabbitmq_pass;
        const std::string vhost = "/";

        AmqpClient::Channel::ptr_t channel = AmqpClient::Channel::Create(hostname, port, username, password, vhost);
        std::string queue_name = "test_queue";
        while (true)
        {
            AmqpClient::Envelope::ptr_t envelope = channel->BasicConsumeMessage(channel->BasicConsume(queue_name, "", true, true));

            if (envelope)
            {
                AmqpClient::BasicMessage::ptr_t message = envelope->Message();
                std::string body = message->Body();

                std::cout << "Received message: " << body << std::endl;

                channel->BasicAck(envelope);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}