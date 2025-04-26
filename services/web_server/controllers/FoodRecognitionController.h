#pragma once

#include <drogon/HttpController.h>
#include "controller_utils.hpp"

using namespace drogon;

class FoodRecognitionController : public drogon::HttpController<FoodRecognitionController>
{
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(FoodRecognitionController::recognize_food, "/recognize_food", Post);
  ADD_METHOD_TO(FoodRecognitionController::get_status, "/get_status", Get);
  ADD_METHOD_TO(FoodRecognitionController::get_result, "/get_result", Get);
  METHOD_LIST_END

  void recognize_food(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
  void get_status(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
  void get_result(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
};
