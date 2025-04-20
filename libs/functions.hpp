#include <iostream>
#include <string>
#include <fstream>
#include "nlohmann/json.hpp"

class Logger
{
public:
    Logger(const Logger &l) = delete;
    Logger(Logger &&l) = delete;
    Logger &operator=(const Logger &l) = delete;
    Logger &operator=(Logger &&l) = delete;

    static inline Logger &getInstance()
    {
        static Logger s{};
        return s;
    }

    constexpr void bindInfo(std::ostream *stream)
    {
        _info_stream = stream;
    }

    constexpr void bindError(std::ostream *stream)
    {
        _error_stream = stream;
    }

    constexpr void info(const std::string &str)
    {
        (*_info_stream) << str << std::endl;
    }

    constexpr void error(const std::string &str)
    {
        (*_error_stream) << str << std::endl;
    }

private:
    inline Logger()
    {
    }

    std::ostream *_info_stream{&std::cout};
    std::ostream *_error_stream{&std::cerr};
};

#define LOG_ERROR(x)                                                                                                                     \
    do                                                                                                                                   \
    {                                                                                                                                    \
        Logger::getInstance().error(std::string{__FILE__} + ":" + std::to_string(__LINE__) + " " + std::string{__FUNCTION__} + " " + x); \
    } while (0)
#define LOG_INFO(x)                                                                                                                     \
    do                                                                                                                                  \
    {                                                                                                                                   \
        Logger::getInstance().info(std::string{__FILE__} + ":" + std::to_string(__LINE__) + " " + std::string{__FUNCTION__} + " " + x); \
    } while (0)

std::string getFileAsString(const std::string &path)
{
    std::ifstream file{path};
    if (!file)
    {
        Logger::getInstance().error("std::string getFileAsString(const std::string& path): " + path);
        return {};
    }

    std::ostringstream ss{};
    ss << file.rdbuf();

    return ss.str();
}

class Cfg
{
public:
    Cfg(const Cfg &l) = delete;
    Cfg(Cfg &&l) = delete;
    Cfg &operator=(const Cfg &l) = delete;
    Cfg &operator=(Cfg &&l) = delete;

    static inline Cfg &getInstance()
    {
        static Cfg s{};
        return s;
    }

    inline bool loadFromArgcArv(int argc, char **argv)
    {
        if (argc != 2)
        {
            LOG_ERROR("if(argc != 2)");
            return false;
        }

        std::string file_str = getFileAsString(argv[1]);
        if (file_str.empty())
        {
            LOG_ERROR("if(file_str.empty())");
            return false;
        }

        if (!nlohmann::json::accept(file_str))
        {
            LOG_ERROR("if (!nlohmann::json::accept(file_str))");
            return false;
        }

        _file_json = nlohmann::json::parse(file_str);

        return true;
    }

    inline bool loadFromEnv()
    {
        const char* file_path = getenv("ex_cfg_path");
        if(!file_path || strlen(file_path) <= 0)
        {
            LOG_ERROR(" if(!file_path || strlen(file_path) <= 0)");
            return false;
        }

        std::string file_str = getFileAsString(file_path);
        if (file_str.empty())
        {
            LOG_ERROR("if(file_str.empty())");
            return false;
        }

        if (!nlohmann::json::accept(file_str))
        {
            LOG_ERROR("if (!nlohmann::json::accept(file_str))");
            return false;
        }

        _file_json = nlohmann::json::parse(file_str);

        return true;
    }

    inline std::string getCfgValue(const std::string &key) const
    {
        if (!_file_json.contains(key) || !_file_json[key].is_string())
        {
            return {};
        }

        return _file_json[key].get<std::string>();
    }

private:
    inline Cfg()
    {
    }

    nlohmann::json _file_json{};
};