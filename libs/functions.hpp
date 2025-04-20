#pragma once

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
        const char *file_path = getenv("ex_cfg_path");
        if (!file_path || strlen(file_path) <= 0)
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

inline std::string base64_encode(const std::vector<unsigned char> &data)
{
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    size_t in_len = data.size();

    while (in_len--)
    {
        char_array_3[i++] = data[j++];
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
            {
                ret += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i)
    {
        for (int j = i; j < 3; j++)
        {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (int j = 0; j < i + 1; j++)
        {
            ret += base64_chars[char_array_4[j]];
        }

        while (i++ < 3)
        {
            ret += '=';
        }
    }

    return ret;
}

inline std::string image_to_base64(const std::string &image_path)
{
    std::ifstream file(image_path, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("if (!file)");
        return {};
    }

    file.seekg(0, std::ios::end);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> buffer(file_size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), file_size))
    {
        LOG_ERROR("if (!file.read(reinterpret_cast<char *>(buffer.data()), file_size))");
        return {};
    }

    return base64_encode(buffer);
}

struct MimeTypeAndBase64
{
    std::string mime_type{};
    std::string base64_string{};

    inline std::string encode() const
    {
        return "data:" + mime_type + ";base64," + base64_string;
    }
};

inline MimeTypeAndBase64 image_to_base64_data_uri(const std::string &image_path)
{
    std::string base64_string = image_to_base64(image_path);
    if(base64_string.empty())
    {
        LOG_ERROR("if(base64_string.empty())");
        return {};
    }

    std::string mime_type = "image/jpeg";
    std::string ext = image_path.substr(image_path.find_last_of(".") + 1);

    if (ext == "png")
    {
        mime_type = "image/png";
    }
    else if (ext == "gif")
    {
        mime_type = "image/gif";
    }
    else if (ext == "bmp")
    {
        mime_type = "image/bmp";
    }
    else if (ext == "webp")
    {
        mime_type = "image/webp";
    }

    return MimeTypeAndBase64{mime_type, base64_string};
}
