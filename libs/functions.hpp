#include <iostream>
#include <string>
#include <fstream>
#include "nlohmann/json.hpp"

class Logger
{
public:
    static inline Logger &getInstance()
    {
        static Logger s{};
        return s;
    }

    constexpr void bindInfo(std::ostream* stream)
    {
        _info_stream = stream;
    }

    constexpr void bindError(std::ostream* stream)
    {
        _error_stream = stream;
    }

    constexpr void info(const std::string& str)
    {
        (*_info_stream) << str << std::endl;
    }

    constexpr void error(const std::string& str)
    {
        (*_error_stream) << str << std::endl;
    }

private:
    std::ostream* _info_stream{&std::cout};
    std::ostream* _error_stream{&std::cerr};
};

std::string getFileAsString(const std::string& path)
{
    std::ifstream file{path};
    if(!file)
    {
        Logger::getInstance().error("std::string getFileAsString(const std::string& path): " + path);
        return {};
    }

    std::ostringstream ss{};
    ss << file.rdbuf();
    
    return ss.str();
}

std::string getCfgValue(const std::string& filename, const std::string &key)
{
    std::string file_str = getFileAsString(filename);
    if(file_str.empty())
    {
        Logger::getInstance().error(std::string{__FUNCTION__} + " if(file_str.empty())");
        return {};
    }

    if(!nlohmann::json::accept(file_str))
    {
        Logger::getInstance().error(std::string{__FUNCTION__} + " if(!nlohmann::json::accept(file_str))");
        return {};
    }

    nlohmann::json file_json = nlohmann::json::parse(file_str);
    if(!file_json.contains(key) || !file_json[key].is_string())
    {
        return {};
    }
    
    return file_json[key];
}
