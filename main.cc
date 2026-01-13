#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
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
      std::ifstream file(std::format("excuses_{}_{}.txt", style_map.at((style)s), purpose_map.at((purpose)p)));
      if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open " + std::format("excuses_{}_{}.txt", style_map.at((style)s), purpose_map.at((purpose)p)));
      }
      std::string line;
      while (std::getline(file, line)) {
        excuses[static_cast<style>(s)][static_cast<purpose>(p)].push_back(line);
      }
    }
  }
  return excuses;
}

int main() {
  std::cout << "Random Excuse Generator" << std::endl;
  std::map<style, std::map<purpose, std::vector<std::string>>> excuses;
  try {
    excuses = LoadExcuses();
  }
  catch (std::exception& exception) {
    std::cerr << "Failed to load local excuses, program will be limited to AI consultation. Message: " << exception.what() << std::endl;
    config::ai_only = true;
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
  if (mode == 1) {
    if (purpose == WORK) {
      std::cout << excuses[CASUAL][WORK].at(rand() % excuses[CASUAL].size()) << std::endl;
    } else if (purpose == SOCIAL) {
      std::cout << excuses[CASUAL][SOCIAL].at(rand() % excuses[CASUAL].size()) << std::endl;
    } else if (purpose == OTHER) {
      std::cout << excuses[CASUAL][OTHER].at(rand() % excuses[CASUAL].size()) << std::endl;
    } else {
      std::cout << "Invalid purpose selected." << std::endl;
    }
  } else if (mode == 2) {
    if (purpose == WORK) {
      std::cout << excuses[FORMAL][WORK].at(rand() % excuses[FORMAL].size()) << std::endl;
    } else if (purpose == SOCIAL) {
      std::cout << excuses[FORMAL][SOCIAL].at(rand() % excuses[FORMAL].size()) << std::endl;
    } else if (purpose == OTHER) {
      std::cout << excuses[FORMAL][OTHER].at(rand() % excuses[FORMAL].size()) << std::endl;
    } else {
      std::cout << "Invalid purpose selected." << std::endl;
    }
  } else if (mode == 3) {
    std::cout << "Consulting AI for an excuse..." << std::endl;
    // Contact ChatGPT API
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
      j["input"] = "Give me a random excuse for not completing my work. Do not add any extra explanations. Just give me the excuse plz.";
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
        std::cerr << "curl_easy_perform() failed: "
                  << curl_easy_strerror(res) << std::endl;
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
  } else {
    std::cout << "Invalid mode selected." << std::endl;
  }
  return EXIT_SUCCESS;
}
