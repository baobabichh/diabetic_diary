#pragma once

#include <drogon/HttpController.h>
#include "controller_utils.hpp"

using namespace drogon;

class AuthenticatorController : public drogon::HttpController<AuthenticatorController>
{
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(AuthenticatorController::register_user, "/register_user", {Post, Get});
  ADD_METHOD_TO(AuthenticatorController::login_user, "/login_user", {Post, Get});
  ADD_METHOD_TO(AuthenticatorController::add_record, "/add_record", {Post, Get});
  ADD_METHOD_TO(AuthenticatorController::get_record_ids, "/get_record_ids", {Post, Get});
  ADD_METHOD_TO(AuthenticatorController::get_records_by_ids, "/get_records_by_ids", {Post, Get});
  METHOD_LIST_END

  void register_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
  void login_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
  void add_record(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
  void get_record_ids(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
  void get_records_by_ids(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
};
