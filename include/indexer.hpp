#ifndef _NLP_INDEXER_HPP
#define _NLP_INDEXER_HPP
#include "utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
namespace nlp_utils {
void createForwardIndex(
    const std::vector<std::filesystem::path> &filepaths,
    std::unordered_map<std::string, std::unordered_map<std::string, int>>
        &forwardIndex) {
  for (const auto &filepath : filepaths) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      std::cerr << "Could not open the file " << filepath << std::endl;
      continue;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    std::vector<std::string> tokens = tokenize(content);
    for (const auto &token : tokens) {
      forwardIndex[filepath.filename().string()][token]++;
    }
  }
}
// Save the forward index to a file
int saveIndexToFile(
    const std::string &filename,
    const std::unordered_map<std::string, std::unordered_map<std::string, int>>
        &forwardIndex) {
  std::ofstream outFile(filename);
  if (!outFile.is_open()) {
    std::cerr << "Could not open the file for writing!" << std::endl;
    return -1;
  }

  for (const auto &[docID, termMap] : forwardIndex) {
    outFile << "<source: " << docID << ">\n";
    for (const auto &[term, freq] : termMap) {
      outFile << term << ":" << freq << "\n";
    }
  }
  outFile.close();
  return 0;
}
// Load the forward index from a file
[[nodiscard]] int loadIndexFromFile(
    const std::string &filename,
    std::unordered_map<std::string, std::unordered_map<std::string, int>>
        &forwardIndex) {
  std::ifstream inFile(filename);
  if (!inFile.is_open()) {
    std::cerr << "Could not open the file for reading!" << std::endl;
    return -1;
  }
  std::regex pattern("<source: ([\\w.-]*\\.?\\w+)>");
  std::smatch matches;
  std::string docname;
  std::map<std::string, int> docsFileCount;

  std::string line;
  while (std::getline(inFile, line, '\n')) {
    if (std::regex_match(line, matches, pattern) && matches.length() > 1)
      docname = line;
    if (docname.length() && line != docname) {
      docsFileCount[docname]++;
    }
  }

  inFile.clear();
  inFile.seekg(std::ios::beg);
  while (std::getline(inFile, line, '\n')) {
    if (docsFileCount.find(line) != docsFileCount.end()) {
      std::unordered_map<std::string, int> termMap;
      std::regex_match(line, matches, pattern);
      std::string docname = matches[1].str();
      int count = docsFileCount[line];
      while (count > 0 && std::getline(inFile, line, '\n')) {
        size_t colpos = line.find_last_of(':');
        std::string term = line.substr(0, colpos);
        try {
          int freq = std::stoi(line.substr(colpos + 1));
          termMap[term] = freq;
        } catch (std::invalid_argument &e) {
        } catch (std::out_of_range &e) {
          termMap[line] = std::numeric_limits<int>::max();
        }
        count--;
      }
      forwardIndex[docname] = termMap;
    }
  }

  inFile.close();
  return 0;
}
void createInvertedIndex(
    std::unordered_map<std::string, std::unordered_map<std::string, int>>
        &forwardIndex,
    std::unordered_map<std::string, std::unordered_map<std::string, int>>
        &invertedIndex) {
  for (const auto &[docID, termMap] : forwardIndex) {
    for (const auto &[term, freq] : termMap) {
      invertedIndex[term][docID] = freq;
    }
  }
}
int saveInvertedIndexToFile(
    const std::string &filename,
    const std::unordered_map<std::string, std::unordered_map<std::string, int>>
        &invertedIndex) {
  std::ofstream outFile(filename);
  if (!outFile.is_open()) {
    std::cerr << "Could not open the file for writing!" << std::endl;
    return -1;
  }

  for (const auto &[term, docMap] : invertedIndex) {
    outFile << term;
    for (const auto &[docID, freq] : docMap) {
      outFile << " " << docID << ":" << freq;
    }
    outFile << "\n";
  }
  outFile.close();
  return 0;
}
} // namespace nlp_utils
#endif
