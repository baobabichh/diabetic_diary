#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include "nlohmann/json.hpp"
#include <set>
#include <unordered_set>

template <typename F>
class ScopeExit
{
    F func;

public:
    explicit ScopeExit(F f) : func(std::move(f)) {}
    ~ScopeExit() { func(); }
};

template <typename F>
ScopeExit<F> makeScopeExit(F f)
{
    return ScopeExit<F>(std::move(f));
}

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

    inline void bindInfo(std::ostream *stream)
    {
        _info_stream = stream;
    }

    inline void bindError(std::ostream *stream)
    {
        _error_stream = stream;
    }

    inline void info(const std::string &str)
    {
        (*_info_stream) << str << std::endl;
    }

    inline void error(const std::string &str)
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

#undef LOG_ERROR
#define LOG_ERROR(x)                                                                                                                     \
    do                                                                                                                                   \
    {                                                                                                                                    \
        Logger::getInstance().error(std::string{__FILE__} + ":" + std::to_string(__LINE__) + " " + std::string{__FUNCTION__} + " " + x); \
    } while (0)

#undef LOG_INFO
#define LOG_INFO(x)                                                                                                                     \
    do                                                                                                                                  \
    {                                                                                                                                   \
        Logger::getInstance().info(std::string{__FILE__} + ":" + std::to_string(__LINE__) + " " + std::string{__FUNCTION__} + " " + x); \
    } while (0)

inline std::string getFileAsString(const std::string &path)
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

inline bool writeBytesToFile(const std::string &filename, const std::vector<uint8_t> &data)
{
    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open())
    {
        LOG_ERROR("Can not open file to write: " + filename);
        return false;
    }

    file.write(reinterpret_cast<const char *>(data.data()), data.size());
    if (file.fail())
    {
        LOG_ERROR("Can write to file: " + filename);
        return false;
    }

    return true;
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

        return checkValues();
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

        return checkValues();
    }

    inline bool checkValues() const
    {
        static const std::array needed_keys{
            "openai_api_key",
            "gemini_api_key",
            "claude_api_key",
            "db_user",
            "db_pass",
            "rabbitmq_user",
            "rabbitmq_pass",
            "photos_storage_absolute_path",
        };

        for (const auto &key : needed_keys)
        {
            if (getCfgValue(key).empty())
            {
                LOG_ERROR("no key in cfg: " + key);
                return false;
            }
        }


        try
        {
            std::filesystem::create_directories(getCfgValue("photos_storage_absolute_path"));
        }
        catch(const std::exception& e)
        {
            LOG_ERROR("failed to create photos_storage_absolute_path: " + getCfgValue("photos_storage_absolute_path"));
            LOG_ERROR(e.what());
            return false;
        }
        
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

inline static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

inline const std::vector<unsigned char> base64_decode(const std::string &base64_string)
{
    std::vector<unsigned char> decoded_data;
    decoded_data.reserve(base64_string.size() * 3 / 4);

    int in_len = base64_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];

    while (in_len-- && (base64_string[in_] != '=') &&
           (isalnum(base64_string[in_]) || (base64_string[in_] == '+') || (base64_string[in_] == '/')))
    {
        char_array_4[i++] = base64_string[in_];
        in_++;
        if (i == 4)
        {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                decoded_data.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++)
            decoded_data.push_back(char_array_3[j]);
    }

    return decoded_data;
}

inline std::string base64_encode(const std::vector<unsigned char> &data)
{
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

inline static const std::unordered_set<std::string> supported_mime_types
{
    "image/jpeg",
    "image/png",
    "image/jpg",
};

inline MimeTypeAndBase64 image_to_base64_data_uri(const std::string &image_path)
{
    std::string base64_string = image_to_base64(image_path);
    if (base64_string.empty())
    {
        return {};
    }

    std::string mime_type = "image/jpeg";
    std::string ext = image_path.substr(image_path.find_last_of(".") + 1);

    if (ext == "png")
    {
        mime_type = "image/png";
    }
    else if (ext == "jpg")
    {
        mime_type = "image/jpg";
    }

    if(!supported_mime_types.count(mime_type))
    {
        return {};
    }

    return MimeTypeAndBase64{mime_type, base64_string};
}

inline std::string ext_of_mime_type(const std::string& mime_type)
{
    if(!supported_mime_types.count(mime_type))
    {
        return {};
    }

    const auto img_post = mime_type.find("image/");
    if(img_post == std::string::npos)
    {
        return {};
    }

    std::string copy_str = mime_type;
    copy_str.erase(img_post, strlen("image/"));

    return copy_str;
}
