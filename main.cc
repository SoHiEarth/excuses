#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <random>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

enum style { CASUAL = 1, FORMAL = 2, AI = 3 };
enum purpose { WORK = 1, SOCIAL = 2, OTHER = 3 };

const std::map<style, std::string> style_map = {
  {CASUAL, "casual"},
  {FORMAL, "formal"},
  {AI, "ai"}
};

const std::map<purpose, std::string> purpose_map = {
  {WORK, "work"},
  {SOCIAL, "social"},
  {OTHER, "other"}
};

namespace config {
bool ai_only = false;
}

std::map<style, std::map<purpose, std::vector<std::string>>> LoadExcuses() {
  std::map<style, std::map<purpose, std::vector<std::string>>> excuses;
  for (int s = CASUAL; s <= FORMAL; ++s) {
    for (int p = WORK; p <= OTHER; ++p) {
      auto filename = std::format("excuses_{}_{}.txt", style_map.at((style)s), purpose_map.at((purpose)p));
      std::ifstream file(filename);
      auto& excuse_list = excuses[static_cast<style>(s)][static_cast<purpose>(p)];
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

int main() {
  std::cout << "\e[1mRandom Excuse Generator\e[0m" << std::endl;
  std::map<style, std::map<purpose, std::vector<std::string>>> excuses;
  try {
    excuses = LoadExcuses();
  }
  catch (std::exception& exception) {
    std::cerr << "Failed to load local excuses, program will be limited to AI consultation. Message: " << exception.what() << std::endl;
    config::ai_only = true;
  }

  std::cout << "\e[1mWhat to do:\e[0m\n1. Make a random excuse\n2. View favorites\nChoose (1/2): ";
  int choice;
  std::cin >> choice;
  if (choice == 2) {
    std::ifstream fav_file("favorites.txt");
    if (!fav_file.is_open()) {
      std::cerr << "Failed to open favorites. Try making some excuses first" << std::endl;
      return EXIT_SUCCESS;
    }
    std::string line;
    std::cout << "\e[1mFavorite Excuses:\e[0m\n";
    while (std::getline(fav_file, line)) {
      std::cout << "- " << line << std::endl;
    }
    fav_file.close();
    return EXIT_SUCCESS;
  }

  int mode;
  if (!config::ai_only) {
    std::cout << "Mode (1: Casual, 2: Formal, 3: Consult AI): ";
    std::cin >> mode;
  } else {
    mode = 3;
  }

  std::cout << "Purpose (1: Work, 2: Social, 3: Other): ";
  int purpose;
  std::cin >> purpose;
  
  std::vector<std::string> selected_excuses;
  if (mode == 1) {
    if (purpose == WORK) {
      selected_excuses = excuses[CASUAL][WORK];
    } else if (purpose == SOCIAL) {
      selected_excuses = excuses[CASUAL][SOCIAL];
    } else if (purpose == OTHER) {
      selected_excuses = excuses[CASUAL][OTHER];
    } else {
      std::cout << "Invalid purpose selected." << std::endl;
    }
  } else if (mode == 2) {
    if (purpose == WORK) {
      selected_excuses =  excuses[FORMAL][WORK];
    } else if (purpose == SOCIAL) {
      selected_excuses = excuses[FORMAL][SOCIAL];
    } else if (purpose == OTHER) {
      selected_excuses = excuses[FORMAL][OTHER];
    } else {
      std::cout << "Invalid purpose selected." << std::endl;
    }
  }
  if (mode != 3) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, selected_excuses.size() - 1);
    auto random_excuse = selected_excuses.at(dist(rng));
    std::cout << random_excuse << std::endl;
    std::cout << "Add to favorites? (y/n): ";
    char fav_choice;
    std::cin >> fav_choice;
    if (fav_choice == 'y' || fav_choice == 'Y') {
      std::ofstream fav_file("favorites.txt", std::ios::app);
      if (fav_file.is_open()) {
        fav_file << random_excuse << std::endl;
        fav_file.close();
      } else {
        std::cerr << "Error: Could not open favorites.txt" << std::endl;
      }
    }
  }

  if (mode == 3) {
    std::cout << "Consulting AI for an excuse..." << std::endl;
    std::ifstream api_key_file("api_key.txt");
    if (!api_key_file.is_open()) {
      std::cerr << "Error: Could not open api_key.txt" << std::endl;
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
      j["input"] = "Generate a random excuse for not completing my work. Do not add any extra explanations. The purpose is " + purpose_map.at(static_cast<enum purpose>(purpose)) + ".";
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
        std::cout << ai_text << "\n";
      } else {
        std::cerr << "No text output found.\n";
      }
      }
      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
  }
  return EXIT_SUCCESS;
}
