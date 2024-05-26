#ifndef _CONSINE_SIMILARITY_HPP
#define _CONSINE_SIMILARITY_HPP
#include "utils.hpp"
namespace nlp_utils {
double cosineSimilarity(const std::unordered_map<std::string, int> &docVector,
                        const std::map<std::string, int> &queryVector) {
  double dotProduct = 0.0;
  double docMagnitude = 0.0;
  double queryMagnitude = 0.0;

  for (const auto &[term, queryFreq] : queryVector) {
    if (docVector.find(term) != docVector.end()) {
      dotProduct += queryFreq * docVector.at(term);
    }
    queryMagnitude += queryFreq * queryFreq;
  }

  for (const auto &[term, docFreq] : docVector) {
    docMagnitude += docFreq * docFreq;
  }

  docMagnitude = std::sqrt(docMagnitude);
  queryMagnitude = std::sqrt(queryMagnitude);

  if (docMagnitude == 0 || queryMagnitude == 0) {
    return 0.0;
  }
  return dotProduct / (docMagnitude * queryMagnitude);
}

} // namespace nlp_utils
#endif
