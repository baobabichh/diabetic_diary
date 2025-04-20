#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <iostream>
#include <string>
#include "functions.hpp"

int main(int argc, char* argv[]) {

    const std::string filename = "../../../credentials.json";
    const auto rabbitmq_user = getCfgValue(filename, "rabbitmq_user");
    const auto rabbitmq_pass = getCfgValue(filename, "rabbitmq_pass");

    try {
        // Connection parameters
        const std::string hostname = "localhost";
        const int port = 5672;
        const std::string username = rabbitmq_user;
        const std::string password = rabbitmq_pass;
        const std::string vhost = "/";

        // Create a connection
        AmqpClient::Channel::ptr_t channel = AmqpClient::Channel::Create(
            hostname, port, username, password, vhost);

        // Declare a queue
        std::string queue_name = "test_queue";
        channel->DeclareQueue(queue_name, false, true, false, false);

        // Message to publish
        std::string message = "Hello, RabbitMQ!";
        if (argc > 1) {
            message = argv[1];
        }

        // Publish the message
        channel->BasicPublish("", queue_name, 
            AmqpClient::BasicMessage::Create(message));

        std::cout << "Published message: " << message << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}