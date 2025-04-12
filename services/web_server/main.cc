#include <drogon/drogon.h>
#include "functions.hpp"

int main()
{
    Logger::getInstance().info("openai_api_key: " + getCfgValue("../../../credentials.json" ,"openai_api_key"));
    drogon::app().addListener("0.0.0.0", 5555);
    drogon::app().run();
    return 0;
}
