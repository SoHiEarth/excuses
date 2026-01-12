#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
int main() {
  std::cout << "Random Excuse Generator" << std::endl;

  std::ifstream casual_file("casual_excuses.txt");
  if (!casual_file.is_open()) {
    std::cerr << "Error: Could not open casual_excuses.txt" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<std::string> casual_excuses;
  std::string line;
  while (std::getline(casual_file, line)) {
    casual_excuses.push_back(line);
  }
  casual_file.close();

  std::ifstream formal_file("formal_excuses.txt");
  if (!formal_file.is_open()) {
    std::cerr << "Error: Could not open formal_excuses.txt" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<std::string> formal_excuses;
  while (std::getline(formal_file, line)) {
    formal_excuses.push_back(line);
  }
  formal_file.close();

  std::cout << "Mode (1: Casual, 2: Formal, 3: Consult AI): ";
  int mode;
  std::cin >> mode;
  if (mode == 1) {
    std::cout << casual_excuses.at(rand() % (casual_excuses.size() + 1)) << std::endl;
  } else if (mode == 2) {
    std::cout << formal_excuses.at(rand() % (formal_excuses.size() + 1)) << std::endl;
  } else if (mode == 3) {
    std::cout << "Consulting AI for an excuse..." << std::endl;
    // Contact ChatGPT API
    std::ifstream api_key_file("api_key.txt");
    if (!api_key_file.is_open()) {
      std::cerr << "Error: Could not open api_key.txt" << std::endl
                << "Please create the file and add your OpenAI API key." << std::endl;
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
      j["input"] = "Give me a random excuse for not completing my work. Do not add any extra explanations. Just give me the excuse.";
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
