#include "functions.hpp"
#include <drogon/drogon.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/utils/Utilities.h>

int main()
{
    if(!Cfg::getInstance().loadFromEnv())
    {
        return false;
    }

    {
        const auto db_user = Cfg::getInstance().getCfgValue("db_user");
        const auto db_pass = Cfg::getInstance().getCfgValue("db_pass");

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

    drogon::app().setClientMaxBodySize(20 * 1024 * 1024);
    drogon::app().addListener("0.0.0.0", 5050);
    drogon::app().run();
    return 0;
}
