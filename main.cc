#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>

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

  std::cout << "Mode (1: Casual, 2: Formal): ";
  int mode;
  std::cin >> mode;
  if (mode == 1) {
    std::cout << casual_excuses.at(rand() % (casual_excuses.size() + 1)) << std::endl;
  } else if (mode == 2) {
    std::cout << formal_excuses.at(rand() % (formal_excuses.size() + 1)) << std::endl;
  } else {
    std::cout << "Invalid mode selected." << std::endl;
  }
  return EXIT_SUCCESS;
}
