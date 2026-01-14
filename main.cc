#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <random>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <map>
#include <format>
#include <ctime>

enum class Style : int {
  CASUAL = 1,
  FORMAL = 2,
  AI = 3
};

enum class Purpose : int {
  WORK = 1,
  SOCIAL = 2,
  OTHER = 3
};

constexpr Style styles[] = {
  Style::CASUAL,
  Style::FORMAL
};

constexpr Purpose purposes[] = {
  Purpose::WORK,
  Purpose::SOCIAL,
  Purpose::OTHER
};

const std::map<Style, std::string> style_map = {
  {Style::CASUAL, "casual"},
  {Style::FORMAL, "formal"},
  {Style::AI, "ai"}
};

const std::map<Purpose, std::string> purpose_map = {
  {Purpose::WORK, "work"},
  {Purpose::SOCIAL, "social"},
  {Purpose::OTHER, "other"}
};

namespace config {
bool ai_only = false;
bool incognito = false;
const std::string first_run_filename = "first_run.log";
const std::string api_key_filename = "api_key.txt";
const std::string history_filename = "history.txt";
const std::string favorites_filename = "favorites.txt";
const std::string ai_only_argument = "--ai-only";
const std::string incognito_argument = "--incognito";
}

void AddToHistory(std::string_view excuse) {
  if (config::incognito) return;
  std::ofstream history_file(config::history_filename, std::ios::app);
  if (history_file.is_open()) {
    std::time_t now = std::time(nullptr);
    history_file << now  << ": " << excuse << std::endl;
    history_file.close();
  } else {
    std::cerr << "Error: Could not open " << config::history_filename << std::endl;
  }
}

void AskIfFavorite(std::string_view excuse) {
  std::cout << "Add to favorites? (y/n): ";
  char fav_choice;
  std::cin >> fav_choice;
  if (fav_choice == 'y' || fav_choice == 'Y') {
    std::ofstream fav_file(config::favorites_filename, std::ios::app);
    if (fav_file.is_open()) {
      fav_file << excuse << std::endl;
      fav_file.close();
    } else {
      std::cerr << "Error: Could not open " << config::favorites_filename << std::endl;
    }
  }
}

std::map<Style, std::map<Purpose, std::vector<std::string>>> LoadExcuses() {
  std::map<Style, std::map<Purpose, std::vector<std::string>>> excuses;
  for (auto s : styles) {
    for (auto p : purposes) {
      auto filename = std::format("excuses_{}_{}.txt", style_map.at((Style)s), purpose_map.at((Purpose)p));
      std::ifstream file(filename);
      auto& excuse_list = excuses[static_cast<Style>(s)][static_cast<Purpose>(p)];
      if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open " + filename);
      }
      std::string line;
      while (std::getline(file, line)) {
        excuse_list.push_back(line);
      }
      if (excuse_list.empty()) {
        throw std::runtime_error("Error: No excuses found in " + filename);
      }
    }
  }
  return excuses;
}

void InitialSetup() {
  std::cout << "Welcome to Excuses! It seems this is your first time running the program." << std::endl;
  std::cout << "To get started, please enter your OpenAI API key (you can get one from https://platform.openai.com/account/api-keys)\n";
  std::cout << "This step is totally optional, but it will allow you to generate AI-based excuses." << std::endl;
  std::cout << "Enter your API key (or leave blank to skip): ";
  std::string api_key;
  std::getline(std::cin, api_key);
  if (!api_key.empty()) {
    std::ofstream api_key_file(config::api_key_filename);
    if (api_key_file.is_open()) {
      api_key_file << api_key << std::endl;
      api_key_file.close();
      std::cout << "API key saved successfully!" << std::endl;
    } else {
      std::cerr << "Error: Could not save API key." << std::endl;
    }
  }

  if (!std::filesystem::exists(config::first_run_filename)) {
    std::ofstream first_run_file(config::first_run_filename);
    if (!first_run_file.is_open()) {
      first_run_file << "This file indicates that the initial setup has been completed." << std::endl;
    }
    first_run_file.close();
  }
}

int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == config::ai_only_argument) {
      config::ai_only = true;
    } else if (arg == config::incognito_argument) {
      config::incognito = true;
    }
  }

  std::cout << "\e[1mExcuses\t\e[0m\e[3mThe Random Excuse Generator\e[0m" << std::endl;
  if (config::incognito) {
    std::cout << "\e[3mIncognito Mode Enabled: History will not be saved.\e[0m" << std::endl;
  }
  std::map<Style, std::map<Purpose, std::vector<std::string>>> excuses;
  try {
    excuses = LoadExcuses();
  }
  catch (std::exception& exception) {
    std::cerr << "Failed to load local excuses, program will be limited to AI consultation. Message: " << exception.what() << std::endl;
    config::ai_only = true;
  }

  if (!std::filesystem::exists(config::first_run_filename)) {
    InitialSetup();
  }

  while (true) {
  std::cout << "\e[1mWhat to do:\e[0m\n1. Make a random excuse\n2. View favorites\n3. View History\n4. Info\n(1/2/3/4): ";
  int choice;
  std::cin >> choice;
  if (choice == 2) {
    std::ifstream fav_file(config::favorites_filename);
    if (!fav_file.is_open()) {
      std::cerr << "Failed to open favorites. Try making some excuses first" << std::endl;
      return EXIT_SUCCESS;
    }
    std::string line;
    std::cout << "\e[1mFavorite Excuses:\e[0m\n";
    while (std::getline(fav_file, line)) {
      std::cout << "- \e[3m" << line << "\e[0m" << std::endl;
    }
    fav_file.close();
    return EXIT_SUCCESS;
  } else if (choice == 3) {
    std::ifstream history_file(config::history_filename);
    if (!history_file.is_open()) {
      std::cerr << "Failed to open history. Try making some excuses first" << std::endl;
      return EXIT_SUCCESS;
    }
    std::string line;
    std::cout << "\e[1mExcuse History:\e[0m\n";
    while (std::getline(history_file, line)) {
      // parse timestamp
      auto colon_pos = line.find(": ");
      if (colon_pos != std::string::npos) {
        std::string timestamp_str = line.substr(0, colon_pos);
        std::time_t timestamp = std::stoll(timestamp_str);
        std::tm* tm_ptr = std::localtime(&timestamp);
        char time_buf[20];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_ptr);
        std::cout << "[" << time_buf << "] ";
        line = line.substr(colon_pos + 2);
      }
      std::cout << "\e[3m" << line << "\e[0m" << std::endl;
    }
    history_file.close();
    return EXIT_SUCCESS;
  } else if (choice == 4) {
    std::cout << "\e[1mExcuses\e[0m\e[3m The Random Excuse Generator\e[0m\n";
    std::cout << "\e[1mArguments\e[0m\n- " << config::ai_only_argument << ": Only use AI to generate excuses\n- " << config::incognito_argument << ": Do not save history of generated excuses\n";
    std::cout << "Check out the project on GitHub for more info: \e[4mhttps://github.com/sohiearth/excuses\e[0m" << std::endl;
    return EXIT_SUCCESS;
  }

  Style mode = Style::CASUAL;
  if (!config::ai_only) {
    std::cout << "Mode\n1: Casual\n2: Formal\n3: Consult AI\n(1/2/3): ";
    mode = static_cast<Style>(std::cin >> choice, choice);
  } else {
    mode = Style::AI;
  }

  Purpose purpose = Purpose::WORK;
  std::cout << "Purpose\n1: Work\n2: Social\n3: Other\n(1/2/3): ";
  purpose = static_cast<Purpose>(std::cin >> choice, choice);
  
  if (mode != Style::AI) {
    std::vector<std::string> selected_excuses = excuses.at(mode).at(purpose);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, selected_excuses.size() - 1);
    auto random_excuse = selected_excuses.at(dist(rng));
    std::cout << "\e[3m" << random_excuse << "\e[0m" << std::endl;
    AddToHistory(random_excuse);
    AskIfFavorite(random_excuse); 
  }

  if (mode == Style::AI) {
    std::cout << "Consulting AI for an excuse..." << std::endl;
    std::ifstream api_key_file(config::api_key_filename);
    if (!api_key_file.is_open()) {
      std::cerr << "Error: Could not open " << config::api_key_filename << std::endl;
      return EXIT_FAILURE;
    }
    std::string api_key;
    std::getline(api_key_file, api_key);
    api_key_file.close();
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
      headers = curl_slist_append(headers, "Content-Type: application/json");
      nlohmann::json j;
      j["model"] = "gpt-4.1-mini";
      j["input"] = std::format(
        "Generate an excuse for a {} situation. No explanation.",
        purpose_map.at(purpose)
      );
      std::string json_str = j.dump();
      curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/responses");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
      std::string response_string;
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char *ptr, size_t size, size_t nmemb, std::string* data) {
            data->append(ptr, size * nmemb);
            return size * nmemb;
      });
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
      } else {
        auto response_json = nlohmann::json::parse(response_string);
        std::string ai_text;
        for (const auto& item : response_json["output"]) {
        if (item["type"] == "message") {
          for (const auto& content : item["content"]) {
            if (content["type"] == "output_text") {
              ai_text += content["text"].get<std::string>();
            }
          }
        }
      }
      if (!ai_text.empty()) {
        std::cout << "\e[3m" << ai_text << "\e[0m" << "\n";\
        AddToHistory(ai_text);
        AskIfFavorite(ai_text);
      } else {
        std::cerr << "No text output found.\n";
      }
      }
      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
  }
  std::cout << "Do you want to make another excuse? (y/n): ";
  char again_choice;
  std::cin >> again_choice;
  if (again_choice != 'y' && again_choice != 'Y') {
    break;
  }
  }

  return EXIT_SUCCESS;
}
