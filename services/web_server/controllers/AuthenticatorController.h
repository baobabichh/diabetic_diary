#pragma once

#include <drogon/HttpController.h>
#include "controller_utils.hpp"

using namespace drogon;

class AuthenticatorController : public drogon::HttpController<AuthenticatorController>
{
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(AuthenticatorController::register_user, "/register_user", Get);
  ADD_METHOD_TO(AuthenticatorController::login_user, "/login_user", Get);
  METHOD_LIST_END

  void register_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
  void login_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
};
