#include "functions.hpp"

const std::string FoodRecognitions::Status::Waiting = {"1"};
const std::string FoodRecognitions::Status::Processing = {"2"};
const std::string FoodRecognitions::Status::Done = {"3"};
const std::string FoodRecognitions::Status::Error = {"4"};

const nlohmann::json Prompts::nutrition_schema = {
    {"type", "object"},
    {"properties", {{"products", {{"type", "array"}, {"items", {{"type", "object"}, {"properties", {{"name", {{"type", "string"}, {"description", "Exact food name identified in the image"}}}, {"grams", {{"type", "integer"}, {"description", "Detected weight in grams"}}}, {"carbs", {{"type", "integer"}, {"description", "Calculated total carbohydrates rounded to the nearest integer"}}}}}, {"required", {"name", "grams", "carbs"}}}}}}}},
    {"required", {"products"}}};

const std::string Prompts::prompt =
    R"(You are a nutrition‐analysis assistant. Given any input image of foods:

1. Detect every unique food item and its weight in grams.
2. Use surrounding and image depth to determine amount of products.
3. Split products to the smallest parts(for example you should not have a single product with name "Zucchini and cherry tomatoes", it should be two separate products).
4. For each item, retrieve the standard carbohydrate content per 100 g from a reliable nutrition database. 
5. Calculate the total carbohydrates for each item.
6. If there is no food on photo return return zero products.
7. Output ONLY valid JSON in the following format:

{
"products": [
{
  "name":    "<exact food name>",
  "grams":   <detected weight in grams as an integer>,
  "carbs":   <calculated total carbohydrates rounded to the nearest integer>
}
]
})";