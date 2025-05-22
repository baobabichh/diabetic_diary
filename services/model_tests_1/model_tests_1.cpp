#include "openai.hpp"
#include "gemini.hpp"
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <future>

namespace fs = std::filesystem;

int main()
{
    if (!Cfg::getInstance().loadFromEnv())
    {
        LOG_ERROR("if(!Cfg::getInstance().loadFromEnv())");
        return 1;
    }

    srand(static_cast<unsigned int>(time(nullptr)));

    LOG_INFO("");

    static const std::string dataset_folder{"../dataset"};
    static const std::string results_folder{"../results"};

    fs::create_directories(dataset_folder);
    fs::create_directories(results_folder);

    LOG_INFO("dataset_folder: " + dataset_folder);
    LOG_INFO("results_folder: " + results_folder);

    const bool only_new = true;
    std::set<std::string> used_pathes{};
    if (only_new)
    {
        static const std::string tmp_f{"../results/o4-mini"};
        for (const auto &entry : std::filesystem::directory_iterator(tmp_f))
        {
            if (entry.is_regular_file())
            {
                std::string str = entry.path().filename().string();
                str.erase(str.find(".json"));
                used_pathes.insert(str);
            }
        }
    }
    LOG_INFO("used_pathes.size() : " + std::to_string(used_pathes.size()));

    std::vector<fs::path> image_files;
    for (const auto &entry : std::filesystem::directory_iterator(dataset_folder))
    {
        if (entry.is_regular_file())
        {
            std::string mimeType = image_to_base64_data_uri(entry.path()).mime_type;
            if (mimeType.find("image/") == 0 && !used_pathes.count(entry.path().filename().string()))
            {
                image_files.push_back(entry.path());
            }
        }
    }

    LOG_INFO("image_files: " + std::to_string(image_files.size()));

    const bool only_not_food = false;
    for (const auto &image_file : image_files)
    {
        if (only_not_food)
        {
            if (image_file.string().find("not_food") == std::string::npos)
            {
                continue;
            }
        }

        const auto mime_and_base64 = image_to_base64_data_uri(image_file);
        if (mime_and_base64.base64_string.empty() || mime_and_base64.mime_type.empty())
        {
            LOG_ERROR("if(mime_and_base64.base64_string.empty() || mime_and_base64.mime_type.empty())");
            continue;
        }

        std::vector<std::future<void>> futures{};

        {
            std::vector<std::string> models{
                "gpt-4o",
                "gpt-4o-mini",
                "gpt-4.1",
                "gpt-4.1-mini",
                "o4-mini",
            };

            for (const auto &model : models)
            {
                const auto func_process = [=]()
                {
                    // std::this_thread::sleep_for(std::chrono::seconds{1});
                    std::string folder_path = results_folder + "/" + model;
                    std::string res_file_path = folder_path + "/" + image_file.filename().string() + ".json";
                    LOG_INFO("Processing: " + res_file_path);

                    fs::create_directories(folder_path);

                    std::chrono::time_point start_point = std::chrono::system_clock::now();
                    std::chrono::time_point end_point = std::chrono::system_clock::now();

                    nlohmann::json res_json{};
                    bool sucuess{false};
                    while (!sucuess)
                    {
                        // std::this_thread::sleep_for(std::chrono::seconds{1});
                        start_point = std::chrono::system_clock::now();
                        if (!openai::jsonTextImg(model, Prompts::prompt, mime_and_base64.mime_type, mime_and_base64.base64_string, Prompts::nutrition_schema, res_json))
                        {
                            LOG_ERROR("if (!openai::jsonTextImg(Prompts::prompt, mime_and_base64.mime_type, mime_and_base64.base64_string, Prompts::nutrition_schema, res_json))");
                            continue;
                        }
                        sucuess = true;
                        end_point = std::chrono::system_clock::now();
                    }

                    res_json["time_spent"] = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end_point - start_point).count());

                    std::ofstream file{res_file_path};
                    if (!file)
                    {
                        LOG_ERROR("if(!file)");
                        return;
                    }
                    file << res_json.dump();
                };

                futures.emplace_back(std::async(func_process));
            }
        }

        {
            std::vector<std::string> models{
                "gemini-2.0-flash",
                "gemini-2.0-flash-lite",
                "gemini-1.5-flash",
                "gemini-2.0-flash-exp",
                "gemini-2.5-flash-preview-05-20",
                "gemini-2.5-pro-preview-05-06",
            };

            for (const auto &model : models)
            {
                const auto func_process = [=]()
                {
                    // std::this_thread::sleep_for(std::chrono::seconds{1});
                    std::string folder_path = results_folder + "/" + model;
                    std::string res_file_path = folder_path + "/" + image_file.filename().string() + ".json";
                    LOG_INFO("Processing: " + res_file_path);

                    fs::create_directories(folder_path);

                    std::chrono::time_point start_point = std::chrono::system_clock::now();
                    std::chrono::time_point end_point = std::chrono::system_clock::now();

                    nlohmann::json res_json{};

                    bool sucuess{false};
                    while (!sucuess)
                    {
                        // std::this_thread::sleep_for(std::chrono::seconds{1});
                        start_point = std::chrono::system_clock::now();
                        if (!gemini::jsonTextImg(model, Prompts::prompt, mime_and_base64.mime_type, mime_and_base64.base64_string, Prompts::nutrition_schema, res_json))
                        {
                            LOG_ERROR("if (!gemini::jsonTextImg(Prompts::prompt, mime_and_base64.mime_type, mime_and_base64.base64_string, Prompts::nutrition_schema, res_json))");
                            continue;
                        }
                        sucuess = true;
                        end_point = std::chrono::system_clock::now();
                    }

                    res_json["time_spent"] = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end_point - start_point).count());

                    std::ofstream file{res_file_path};
                    if (!file)
                    {
                        LOG_ERROR("if(!file)");
                        return;
                    }
                    file << res_json.dump();
                };

                futures.emplace_back(std::async(func_process));
            }
        }

        for(const auto& fut : futures)
        {
            fut.wait();
        }
    }

    return 0;
}