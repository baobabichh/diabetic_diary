#include <string>
#include <vector>
#include "nlohmann/json.hpp"

class InterfaceRequest
{
public:
    virtual void addImage(const std::string &base64_img) = 0;
    virtual void addText(const std::string &text) = 0;
    virtual std::string getRequest() const;
};

class OpenAIRequest : public InterfaceRequest
{
public:
    void addImage(const std::string &base64_img) override final;
    void addText(const std::string &text) override final;
    std::string getRequest() const override final;

private:
    nlohmann::json _messages{nlohmann::json::array()};
};

class Requester
{
public:
    constexpr void setApiKey(const std::string &api_key) { _api_key = api_key; }
    constexpr const std::string &getApiKey() const { return _api_key; }

private:
    std::string _api_key{};
};
