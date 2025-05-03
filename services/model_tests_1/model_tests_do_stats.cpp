#include "openai.hpp"
#include "gemini.hpp"
#include <filesystem>

namespace fs = std::filesystem;

int main()
{
    if (!Cfg::getInstance().loadFromEnv())
    {
        LOG_ERROR("if(!Cfg::getInstance().loadFromEnv())");
        return 1;
    }

    LOG_INFO("");

    static const std::string dataset_folder{"../dataset"};
    static const std::string true_results_floder{"../true_results"};
    static const std::string results_folder{"../results"};
    static const std::string model_stats_folder{"../model_stats"};

    static const std::vector<std::string> models{
        "gpt-4o",
        "gpt-4o-mini",
        "gpt-4.1",
        "gpt-4.1-mini",
        "o4-mini",
        "gemini-2.0-flash",
        "gemini-2.0-flash-lite",
        "gemini-1.5-flash",
        "gemini-2.0-flash-exp",
    };

    fs::create_directories(dataset_folder);
    fs::create_directories(true_results_floder);
    fs::create_directories(results_folder);
    fs::create_directories(model_stats_folder);

    LOG_INFO("true_results_floder: " + true_results_floder);
    LOG_INFO("dataset_folder: " + dataset_folder);
    LOG_INFO("results_folder: " + results_folder);
    LOG_INFO("model_stats_folder: " + model_stats_folder);

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

    struct PhotoStats
    {
        std::string image_path{};
        float true_carbs{0.0f};
        float carbs{0.0f};
        float accuracy{0.0f};
        float time_ms{0.0f};
    };

    struct ModelStats
    {
        std::string model_name{};
        float accuracy{0.0f};
        float avg_time_ms{0.0f};
    };

    std::unordered_map<std::string, std::vector<PhotoStats>> model_photo_stats{};
    for (const auto &model : models)
    {
        for (const auto &image_file : image_files)
        {
            std::string folder_path = results_folder + "/" + model;
            std::string res_file_path = folder_path + "/" + image_file.filename().string() + ".json";
            std::string true_result_file_path = true_results_floder + "/" + image_file.filename().string() + ".txt";

            fs::create_directories(folder_path);

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

            float total_carbs{0.0f};
            if (json_obj.contains("products") && json_obj["products"].is_array() && json_obj["products"].size())
            {
                for (auto &prod : json_obj["products"])
                {
                    auto carbs_opt = get_float_smart(prod, "carbs");

                    if (carbs_opt.value() <= 0.0001f)
                    {
                        total_carbs = 0.0f;
                    }
                    else
                    {
                        total_carbs = carbs_opt.value();
                    }
                }
            }

            size_t time_ms{0};
            if (json_obj.contains("time_spent"))
            {
                auto val_opt = get_float_smart(json_obj, "time_spent");
                if(val_opt)
                {
                    time_ms = val_opt.value();
                }
                else
                {
                    LOG_ERROR("if(!val_opt)");
                    continue;
                }
            }

            float true_total_carbs{0.0f};
            {
                std::string true_carbs_str = getFileAsString(true_result_file_path);
                if (true_carbs_str.empty())
                {
                    LOG_ERROR("if(true_carbs_str.empty())");
                    continue;
                }

                true_total_carbs = stringToSizeT(true_carbs_str);
            }

            float accuracy = calculateAccuracy(true_total_carbs, total_carbs);

            PhotoStats stats{};
            stats.accuracy = accuracy;
            stats.carbs = total_carbs;
            stats.true_carbs = true_total_carbs;
            stats.image_path = image_file.filename().string();
            stats.time_ms = time_ms;

            model_photo_stats[model].push_back(stats);
        }
    }

    std::unordered_map<std::string, ModelStats> all_model_stats{};

    for(const auto& [model, stats] : model_photo_stats)
    {
        ModelStats model_stats{};
        model_stats.model_name = model;
        
        for(const auto& stat : stats)
        {
            model_stats.avg_time_ms += stat.time_ms;
            model_stats.accuracy += stat.accuracy;
        }

        model_stats.avg_time_ms /= stats.size();
        model_stats.accuracy /= stats.size();

        all_model_stats[model_stats.model_name] = model_stats;
    }

    std::string res_csv_string{};
    res_csv_string += "\"Model\",\"AvgTime\",\"Accuracy\"\n";

    for(const auto& [model, stats] : all_model_stats)
    {
        res_csv_string += "\"" + stats.model_name + "\",";
        res_csv_string += "\"" + floatToStringWithPrecision(stats.avg_time_ms) + "\",";
        res_csv_string += "\"" + floatToStringWithPrecision(stats.accuracy) + "\"";
        res_csv_string += "\n";
    }

    std::ofstream file{model_stats_folder + "/" + "results.csv"};
    if(!file)
    {
        LOG_ERROR("if(!file)");
        return 1;
    }

    file << res_csv_string;

    return 0;
}