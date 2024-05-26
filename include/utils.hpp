#ifndef _NLP_UTIL_HPP
#define _NLP_UTIL_HPP
#include <cmath>
#include <map>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
namespace nlp_utils {
std::vector<std::string> tokenize(const std::string &text) {
  std::vector<std::string> tokens;
  std::regex wordRegex(
      "[a-zA-Z0-9\u1200-\u137F]+"); // Including Ethiopian characters
  auto wordsBegin = std::sregex_iterator(text.begin(), text.end(), wordRegex);
  auto wordsEnd = std::sregex_iterator();

  for (auto it = wordsBegin; it != wordsEnd; ++it) {
    std::string token = it->str();
    std::transform(token.begin(), token.end(), token.begin(), ::tolower);
    tokens.push_back(token);
  }

  return tokens;
}
} // namespace nlp_utils
#endif
