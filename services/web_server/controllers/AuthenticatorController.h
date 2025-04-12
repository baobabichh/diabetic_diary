#pragma once

#include <drogon/HttpController.h>
#include "controller_utils.hpp"

using namespace drogon;

class AuthenticatorController : public drogon::HttpController<AuthenticatorController>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(AuthenticatorController::register_user, "/register_user", Get);
    METHOD_ADD(AuthenticatorController::login_user, "/login_user", Get);
    METHOD_LIST_END
    
    void register_user(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, double p1, int p2) const;
    void login_user(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, double p1, int p2) const;
};
