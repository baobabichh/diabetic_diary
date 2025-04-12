#include "functions.hpp"
#include <drogon/drogon.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/utils/Utilities.h>

int main()
{
    {
        const std::string filename = "../../../credentials.json";
        const auto db_user = getCfgValue(filename, "db_user");
        const auto db_pass = getCfgValue(filename, "db_pass");

        if(db_user.empty() || db_pass.empty())
        {
            Logger::getInstance().error("if(db_user.empty() || db_pass.empty())");
            return 1;
        }

        drogon::orm::MysqlConfig cfg{};
        cfg.host = "127.0.0.1";
        cfg.port = 3306;
        cfg.databaseName = "dd";
        cfg.username = db_user;
        cfg.password = db_pass;
        cfg.connectionNumber = 1;
        cfg.name = "dd";

        drogon::app().addDbClient(cfg);
    }

    drogon::app().addListener("0.0.0.0", 5555);
    drogon::app().run();
    return 0;
}
