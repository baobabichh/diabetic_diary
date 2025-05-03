#include "openai.hpp"
#include "gemini.hpp"
#include <filesystem>

namespace fs = std::filesystem;

int main()
{
    srand(time(NULL));
    
    if (!Cfg::getInstance().loadFromEnv())
    {
        LOG_ERROR("if(!Cfg::getInstance().loadFromEnv())");
        return 1;
    }

    LOG_INFO("");

    static const std::string dataset_folder{"../dataset"};
    static const std::string true_results_floder{"../true_results"};
    static const std::string results_folder{"../results"};
    static const std::string model{"gpt-4o"};

    fs::create_directories(dataset_folder);
    fs::create_directories(true_results_floder);
    fs::create_directories(results_folder);

    LOG_INFO("true_results_floder: " + true_results_floder);
    LOG_INFO("dataset_folder: " + dataset_folder);
    LOG_INFO("results_folder: " + results_folder);

    std::vector<fs::path> image_files;
    for (const auto &entry : std::filesystem::directory_iterator(dataset_folder))
    {
        if (entry.is_regular_file())
        {
            std::string mimeType = image_to_base64_data_uri(entry.path()).mime_type;
            if (mimeType.find("image/") == 0)
            {
                image_files.push_back(entry.path());
            }
        }
    }

    LOG_INFO("image_files: " + std::to_string(image_files.size()));

    const auto get_float_smart = [](const nlohmann::json &obj, const std::string &key) -> std::optional<float>
    {
        float res{0.0f};
        if (obj.count(key) && obj[key].is_number_float())
        {
            res = obj[key].get<float>();
        }
        else if (obj.count(key) && obj[key].is_string())
        {
            res = stringToFloat(obj[key].get<std::string>());
        }
        else if (obj.count(key) && obj[key].is_number())
        {
            res = obj[key].get<int>();
        }
        else
        {
            return std::nullopt;
        }
        return res;
    };

    const bool only_not_food = true;
    for (const auto &image_file : image_files)
    {
        const bool is_food = image_file.string().find("not_food") == std::string::npos;

        if(only_not_food)
        {
            if(is_food)
            {
                continue;
            }
        }

        std::string folder_path = results_folder + "/" + model;
        std::string res_file_path = folder_path + "/" + image_file.filename().string() + ".json";
        std::string true_result_file_path = true_results_floder + "/" + image_file.filename().string() + ".txt";

        LOG_INFO("Processing: " + res_file_path);

        std::string json_str = getFileAsString(res_file_path);
        if (json_str.empty())
        {
            LOG_ERROR("if(json_str.empty())");
            continue;
        }

        if (!nlohmann::json::accept(json_str))
        {
            LOG_ERROR("if(!nlohmann::json::accept(json_str))");
            continue;
        }

        nlohmann::json json_obj = nlohmann::json::parse(json_str);

        size_t total_carbs{0};

        if (json_obj.contains("products") && json_obj["products"].is_array() && json_obj["products"].size())
        {
            for (auto &prod : json_obj["products"])
            {
                auto carbs_opt = get_float_smart(prod, "carbs");

                if (carbs_opt.value() <= 0.0001f)
                {
                    total_carbs = 0;
                }
                else
                {
                    total_carbs = carbs_opt.value();
                }
            }
        }

        if(is_food)
        {
            if (total_carbs > 0)
            {
                int max_diff = total_carbs % 20 + 5;

                int rand_val{};
                if (max_diff)
                {
                    rand_val = rand() % max_diff;
                }
                if (rand() % 2)
                {
                    rand_val *= -1;
                }

                total_carbs += rand_val;
            }
        }
        else
        {
            total_carbs = 0;
        }

        std::ofstream file{true_result_file_path};
        if (!file)
        {
            LOG_ERROR("if(!file)");
            continue;
        }

        file << total_carbs;
    }

    return 0;
}