#include "../include/cosine_similarity.hpp"
#include "../include/indexer.hpp"
#include "../include/utils.hpp"

using namespace nlp_utils;
int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <directory_path>" << std::endl;
    return 1;
  }

  std::filesystem::path directoryPath = argv[1];

  if (!std::filesystem::exists(directoryPath) ||
      !std::filesystem::is_directory(directoryPath)) {
    std::cerr << "The provided path is not a valid directory." << std::endl;
    return 1;
  }

  // Collect all file paths in the directory
  std::vector<std::filesystem::path> filepaths;
  for (const auto &entry : std::filesystem::directory_iterator(directoryPath)) {
    if (entry.is_regular_file()) {
      filepaths.push_back(entry.path());
    }
  }

  // Create the forward index
  std::unordered_map<std::string, std::unordered_map<std::string, int>>
      forwardIndex;
  createForwardIndex(filepaths, forwardIndex);
  std::unordered_map<std::string, std::unordered_map<std::string, int>>
      invertedIndex;
  createInvertedIndex(forwardIndex, invertedIndex);
  // Save the forward index to a file
  std::string indexFilename = "forward_index.txt";
  std::string invertedIndexFilename = "inverted_index.txt";

  saveInvertedIndexToFile(invertedIndexFilename, invertedIndex);
  saveIndexToFile(indexFilename, forwardIndex);

  // Clear the index and load it from the file
  forwardIndex.clear();
  loadIndexFromFile(indexFilename, forwardIndex);

  // Ask the user for a query
  std::string query;
  std::cout << "Enter a query: ";
  std::getline(std::cin, query);
  // Tokenize the query
  std::vector<std::string> queryTokens = tokenize(query);
  std::map<std::string, int> queryVector;
  for (const auto &token : queryTokens) {
    queryVector[token]++;
  }

  // Compute cosine similarity between query and documents
  std::map<std::string, double> similarityScores;
  for (const auto &[docID, docVector] : forwardIndex) {
    double similarity = cosineSimilarity(docVector, queryVector);
    if (similarity > 0) {
      similarityScores[docID] = similarity;
    }
  }

  // Sort documents by similarity scores in descending order
  std::vector<std::pair<std::string, double>> sortedDocs(
      similarityScores.begin(), similarityScores.end());
  std::sort(sortedDocs.begin(), sortedDocs.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  // Output the sorted documents
  if (sortedDocs.empty()) {
    std::cout << "No documents found matching the query." << std::endl;
  } else {
    std::cout << "Documents matching the query:" << std::endl;
    for (const auto &[docID, score] : sortedDocs) {
      std::cout << docID << " (score: " << score << ")" << std::endl;
    }
  }
  return 0;
}
