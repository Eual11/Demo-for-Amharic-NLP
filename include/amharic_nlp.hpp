#ifndef _STEMMER_HPP
#define _STEMMER_HPP
#include <algorithm>
#include <codecvt>
#include <fcntl.h>
#include <float.h>
#include <fstream>
#include <io.h>
#include <ios>
#include <iostream>
#include <istream>
#include <limits.h>
#include <locale>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace amh_nlp {
static bool sortBySecond(const std::pair<std::wstring, int> &a,
                         const std::pair<std::wstring, int> &b) {
  return a.second < b.second;
}

static bool
sortBySecondInc(const std::pair<std::pair<std::wstring, float>, int> &a,
                const std::pair<std::pair<std::wstring, int>, int> &b) {
  return a.first.second < b.first.second;
}
static bool
sortByThirdDec(const std::pair<std::pair<std::wstring, float>, long long> &a,
               const std::pair<std::pair<std::wstring, float>, long long> &b) {
  return a.second > b.second;
}

class AmharicNLP {

public:
  class WLinSpell {

  public:
    WLinSpell() = default;
    std::unordered_map<std::wstring, long long> Dictionary;
    void load(std::string path) {

      std::ifstream corpus(path);
      if (corpus.fail()) {
        std::cout << "File: " << path << " not found\n";
        exit(1);
      }

      std::stringstream data_stream;
      data_stream << corpus.rdbuf();
      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
      std::wstring bufferCpy = converter.from_bytes(data_stream.str());

      std::wstringstream buffer(bufferCpy);

      std::wstring readline;
      while (std::getline(buffer, readline, L'\n')) {

        std::wstring word = readline.substr(0, readline.find_first_of(' ', 0));
        word = AmharicNLP::decomposeStringSyllables(word);
        std::wstring fword =
            readline.substr(readline.find_first_of(' ', 0) + 1);
        try {

          long long freq = std::stoll(converter.to_bytes(fword));
          Dictionary[word] = freq;
        } catch (const std::invalid_argument &e) {
          std::wcout << readline << std::endl;
          std::wcout << L"invalid stoll fuck this shit argument: " << fword
                     << L"in " << readline << std::endl;
          try {
            fword = readline.substr(readline.find_first_of(' ', 0) + 1);
            long long freq = std::stoll(converter.to_bytes(fword));
            Dictionary[word] = freq;

          } catch (const std::invalid_argument &e) {
            std::wcout << "Still fauled\n";
            continue;
          }

        } catch (const std::out_of_range &e) {
          std::wcout << L"out of range stol " << fword << " in " << readline
                     << std::endl;
          Dictionary[word] = LONG_MAX;
        }
      }
    }
    std::wstring Correct(std::wstring word, const int editDistance) {
      std::wstring result =
          Correct(AmharicNLP::decomposeStringSyllables(word), editDistance, 1);
      if (result == L"")
        return Correct(AmharicNLP::decomposeStringSyllables(word),
                       editDistance + 1, 1);
      return result;
    }

    std::wstring Correct(std::wstring word, const int editDistance,
                         int verbosity) {
      std::wstring result = L"";
      if (verbosity == 1 &&
          Dictionary.find(AmharicNLP::decomposeStringSyllables(word)) !=
              Dictionary.end()) {
        return word;
      }

      // decomposing the word
      word = AmharicNLP::decomposeStringSyllables(word);
      std::vector<std::pair<std::pair<std::wstring, float>, long long>>
          candidatesVec;
      for (std::pair<std::wstring, int> entry : Dictionary) {

        int pr = word.length() - entry.first.length();
        pr = pr < 0 ? -pr : pr;

        if ((pr <= editDistance)) {
          float levD = levenD(entry.first, word);
          if (levD <= editDistance) {
            candidatesVec.push_back(std::make_pair(
                std::make_pair(entry.first, levD), Dictionary[entry.first]));
          }
        }
      }

      std::sort(candidatesVec.begin(), candidatesVec.end(), sortBySecondInc);
      if (candidatesVec.size() > 6)
        candidatesVec.erase(candidatesVec.begin() + 7, candidatesVec.end());
      if (!candidatesVec.empty()) {

        if (verbosity == 1) {
          auto cand = candidatesVec[0];
          return AmharicNLP::composeStringSyllables(cand.first.first);
        }

        std::vector<std::pair<std::pair<std::wstring, float>, long long>>::
            iterator iter = candidatesVec.begin();

        while (iter != candidatesVec.end()) {
          auto loc = std::find_if_not(
              iter, candidatesVec.end(),
              [&iter](
                  const std::pair<std::pair<std::wstring, int>, long long> &a) {
                return a.first.second == iter->first.second;
              });

          std::sort(iter, loc, sortByThirdDec);
          iter = loc;
        }

        for (auto d : candidatesVec)
          std::wcout << AmharicNLP::composeStringSyllables(d.first.first)
                     << L" ";
      }
      return L"";
    }

  private:
    friend class AmharicNLP;
    float levenD(const std::wstring &a, const std::wstring &b) {

      int N1 = a.length() + 1;
      int N2 = b.length() + 1;

      std::vector<std::vector<float>> M(N1, std::vector<float>(N2));

      for (int i = 0; i < N1; i++)
        M[i][0] = i;
      for (int i = 0; i < N2; i++)
        M[0][i] = i;

      auto lamdaMin = [](const int &a, const int &b, const int &c) {
        int lst[3] = {a, b, c};

        int min = lst[0];
        for (int a : lst)
          if (a < min)
            min = a;

        return min;
      };

      for (int i = 1; i < N1; i++)
        for (int j = 1; j < N2; j++) {
          int cost = !(a[i - 1] == b[j - 1]) + 1;
          if (AmharicNLP::getSyllableConsonant(a[i - 1]) ==
              AmharicNLP::getSyllableConsonant(b[j - 1]))
            cost -= 0.5;
          M[i][j] = lamdaMin(M[i - 1][j] + 1, M[i][j - 1] + 1,
                             M[i - 1][j - 1] + cost);

          if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1])
            M[i][j] = std::min(M[i][j], M[i - 2][j - 2] + 1);
        }
      return M[N1 - 1][N2 - 1];
    }
  };
  AmharicNLP() {

#ifdef ALP_ENABLE_SUGGESTION
    std::wcout << "Loading Recovery " << std::endl;
    suggest.load("./assets/am_frequency_data.txt");
    std::wcout << "Recovery Completed" << std::endl;

#endif
  }
  std::fstream file;
  WLinSpell suggest;
  // checks if the given amharic character is vowel or not
  void testDecompose() {

    std::stringstream buffer;
    std::string data_string;
    buffer << file.rdbuf();
    data_string = buffer.str();

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    std::wstring wstr = converter.from_bytes(data_string);

    std::vector<std::wstring> words = splitWString(wstr, ' ');

    int testsPassed = 0;
    int testsFailed = 0;
    for (const auto &word : words) {
      std::wstring decomposed = decomposeStringSyllables(word);
      std::wstring composed = composeStringSyllables(decomposed);

      if (composed == word) {
        std::wcout << " Test Passed " << word << " = " << composed << "\n";
        testsPassed++;
      } else {
        std::wcout << " Test Failed " << word << " != " << composed
                   << " was decomposed to [" << decomposed << "]\n";
        testsFailed++;
      }
    }
    std::wcout << testsPassed << " Tests Passed \n"
               << testsFailed << " Tests Failed";
  }

  void testStemmer() {
    std::fstream file("input.txt", std::ios::in | std::ios::binary);

    if (!file.is_open()) {
      std::wcerr << "Couldn't open file\n";
      return;
    }

    std::stringstream buffer;
    std::string data_string;
    buffer << file.rdbuf();
    data_string = buffer.str();

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    std::wstring wstr = converter.from_bytes(data_string);

    std::vector<std::wstring> words = splitWString(wstr, ' ');

    for (const auto &word : words) {
      std::wstring word_to_process;
      for (auto ch : word) {
        if (isSyllable(ch) || isVowel(ch) || isConsonant(ch)) {
          word_to_process += ch; // stripping uncessary characters
        }
      }
      std::wstring stemmed_word =
          wstring_stem(decomposeStringSyllables(word_to_process), 1, 1);
      std::wcout << word_to_process << "  " << stemmed_word << " "
                 << suggest.Correct(
                        wstring_stem(decomposeStringSyllables(word_to_process),
                                     1, 1),
                        2, 1)
                 << "\n";
    }
  }

  std::wstring normalizeString(const std::wstring &wstr) {
    std::wstring word;
    word = stopword_remove(wstr);
    return decomposeStringSyllables(word);
  }
  /**
   * @brief Checks if a character is a vowel.
   *
   * @param ch The character to be checked.
   * @return True if the character is a vowel, false otherwise.
   */
  static bool isVowel(const char32_t ch) {

    unsigned int codepoint = static_cast<unsigned int>(ch);
    unsigned int character_row = codepoint / 16; // 0xabce -> 0xabc
    unsigned int character_col = codepoint % 16; // 0xabce -> 0xe
    //

    if (character_col == 5)
      return false;
    if (character_row == 0x12A &&
        (character_col == 7 || (character_col >= 1 && character_col <= 6)))
      return true;

    return false;
  }
  static std::u32string wstring_to_u32string(const std::wstring &wstr) {

    std::u32string u32str;
    u32str.resize(wstr.size());

    for (size_t i = 0; i < wstr.size(); i++) {
      u32str.insert(u32str.begin() + i, static_cast<uint32_t>(wstr[i]));
    }
    return u32str;
  }
  /**
   * @brief Checks if a character is a digit.
   *
   * @param ch The character to be checked.
   * @return True if the character is a digit, false otherwise.
   */
  static bool isAhaz(const char32_t ch) {

    uint32_t codepoint = static_cast<uint32_t>(ch);

    if (codepoint >= 0x1369 && codepoint <= 0x1371)
      return true;
    return false;
  }

  /**
   * @brief Checks if a character is a number.
   *
   * @param ch The character to be checked.
   * @return True if the character is a number, false otherwise.
   */
  static bool isKuter(const char32_t ch) {

    uint32_t codepoint = static_cast<uint32_t>(ch);

    if (isAhaz(ch) || (codepoint >= 0x1372 && codepoint <= 0x137C))
      return true;
    return false;
  }
  /**
   * @brief Checks if a character is a consonant.
   *
   * @param ch The character to be checked.
   * @return True if the character is a consonant, false otherwise.
   */
  static bool isConsonant(const char32_t ch) {
    if (isVowel(ch))
      return false;
    if (isKuter(ch))
      return false;
    if (isCombiningMark(ch) || isPunctuation(ch))
      return false;
    uint32_t codepoint = static_cast<uint32_t>(ch);

    uint32_t character_col = codepoint % 16;
    if (character_col % 8 == 5)
      return true;

    return false;
  }
  /**
   * @brief Checks if a character is a syllable.
   *
   * @param ch The character to be checked.
   * @return True if the character is a syllable, false otherwise.
   */
  static bool isSyllable(const char32_t ch) {

    if ((uint32_t)ch < 0x1200 || (uint32_t)(ch) > 0x137F)
      return false;
    if (isKuter(ch) || isCombiningMark(ch) || isPunctuation(ch))
      return false;
    return true;
  }
  static bool isCombiningMark(const char32_t ch) {

    uint32_t codepoint = static_cast<uint32_t>(ch);

    if (codepoint >= 0x135D && codepoint <= 0x135F)
      return true;
    return false;
  }
  /**
   * @brief Checks if a character is punctuation.
   *
   * @param ch The character to be checked.
   * @return True if the character is punctuation, false otherwise.
   */
  static bool isPunctuation(const char32_t ch) {
    uint32_t codepoint = static_cast<uint32_t>(ch);

    if (codepoint >= 0x1360 && codepoint <= 0x1368)
      return true;
    return false;
  }

  /**
   * @brief Checks if a character is an abukuter (syllable or number).
   *
   * @param ch The character to be checked.
   * @return True if the character is an abukuter, false otherwise.
   */
  static bool isAbukuter(const char32_t ch) {
    return (isSyllable(ch) || isKuter(ch));
  }

  static char32_t getSyllableConsonant(const char32_t ch) {

    if (!isSyllable(ch))
      return ch;
    if (isConsonant(ch))
      return ch;
    if (isVowel(ch))
      return ch;

    else {
      uint32_t codepoint = static_cast<uint32_t>(ch);
      uint32_t character_col = codepoint % 16;
      if (character_col > 7) {
        codepoint -= character_col;
        codepoint += 0xD;
      } else {
        codepoint -= character_col;
        codepoint += 0x5;
      }

      return static_cast<char32_t>(codepoint);
    }
  }

  static std::wstring decomposeSyllable(const char32_t ch) {
    if (!isSyllable(ch)) {
      std::wstring decomposed{static_cast<wchar_t>(ch)};
      return decomposed;
    }
    if (isDerivateSyllable(ch)) {
      // the consonant and vowel form for derivate syllabel is different from
      // the regular ones the consonant remains the sadis letter but the vowel
      // is the letter ዋ
      char32_t consonant = getSyllableConsonant(ch);
      char32_t vowel = U'ዋ';
      std::wstring decomposed{static_cast<wchar_t>(consonant),
                              static_cast<wchar_t>(vowel)};
      return decomposed;
    }

    // han
    char32_t consonant = getSyllableConsonant(ch);
    char32_t vowel = getSyllableVowel(ch);
    if (consonant != vowel) {
      std::wstring decomposed{static_cast<wchar_t>(consonant),
                              static_cast<wchar_t>(vowel)};
      return decomposed;
    }
    std::wstring decomposed{static_cast<wchar_t>(consonant)};

    return decomposed;
  }
  static char32_t getSyllableVowel(const char32_t ch) {
    if (!isSyllable(ch))
      return ch;
    if (isVowel(ch))
      return ch;
    if (isConsonant(ch))
      return ch;
    //
    char32_t amharic_vowels[] = {U'ኧ', U'ኡ', U'ኢ', U'ኣ', U'ኤ', U'እ', U'ኦ'};
    uint32_t codepoint = static_cast<uint32_t>(ch);
    uint32_t character_col = codepoint % 16;

    if (character_col % 8 < (sizeof(amharic_vowels) / sizeof(char32_t))) {
      int vowel_index = character_col % 8;
      return amharic_vowels[vowel_index];
    }
    return ch;
  }
  /**
   * @brief Decomposes an Amharic string into its building blocks of consonants
   * and vowels.
   *
   * @param str The Amharic string to be decomposed.
   * @return The decomposed string consisting of consonants and vowels.
   */
  static std::wstring decomposeStringSyllables(const std::wstring &str) {
    std::u32string u32str = wstring_to_u32string(str);

    std::wstring decomposedString;

    for (size_t i = 0; i < str.length(); i++) {
      char32_t ch = u32str[i];
      decomposedString += decomposeSyllable(ch);
    }
    return decomposedString;
  }
  inline std::string wstring_to_string(const std::wstring &wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    return converter.to_bytes(wstr);
  }
  inline std::wstring string_to_wstring(const std::string &str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    return converter.from_bytes(str);
  }
  std::vector<std::wstring> stop_word_list = {
      L"ስለሚሆን", L"እና",     L"ስለዚህ",   L"በመሆኑም", L"ሁሉ",    L"ሆነ",    L"ሌላ",
      L"ልክ",    L"ስለ",     L"በቀር",    L"ብቻ",    L"ና",     L"አንዳች",  L"አንድ",
      L"እንደ",   L"እንጂ",    L"ያህል",    L"ይልቅ",   L"ወደ",    L"እኔ",    L"የእኔ",
      L"ራሴ",    L"እኛ",     L"የእኛ",    L"እራሳችን", L"አንቺ",   L"የእርስዎ", L"ራስህ",
      L"ራሳችሁ",  L"እሱ",     L"እሱን",    L"የእሱ",   L"ራሱ",    L"እርሷ",   L"የእሷ",
      L"ራሷ",    L"እነሱ",    L"እነሱን",   L"የእነሱ",  L"እራሳቸው", L"ምንድን",  L"የትኛው",
      L"ማንን",   L"ይህ",     L"እነዚህ",   L"እነዚያ",  L"ነኝ",    L"ነው",    L"ናቸው",
      L"ነበር",   L"ነበሩ",    L"ሁን",     L"ነበር",   L"መሆን",   L"አለኝ",   L"አለው",
      L"ነበረ",   L"መኖር",    L"ያደርጋል",  L"አደረገው", L"መሥራት",  L"እና",    L"ግን",
      L"ከሆነ",   L"ወይም",    L"ምክንያቱም", L"እንደ",   L"እስከ",   L"ቢሆንም",  L"ጋር",
      L"ላይ",    L"መካከል",   L"በኩል",    L"ወቅት",   L"በኋላ",   L"ከላይ",   L"በርቷል",
      L"ጠፍቷል",  L"በላይ",    L"ስር",     L"እንደገና", L"ተጨማሪ",  L"ከዚያ",   L"አንዴ",
      L"እዚህ",   L"እዚያ",    L"መቼ",     L"የት",    L"እንዴት",  L"ሁሉም",   L"ማናቸውም",
      L"ሁለቱም",  L"እያንዳንዱ", L"ጥቂቶች",   L"ተጨማሪ",  L"በጣም",   L"ሌላ",    L"አንዳንድ",
      L"አይ",    L"ወይም",    L"አይደለም",  L"ብቻ",    L"የራስ",   L"ተመሳሳይ", L"ስለዚህ",
      L"እኔም",   L"በጣም",    L"ይችላል",   L"ይሆናል",  L"በቃ",    L"አሁን",
  };
  std::wstring stopword_remove(const std::wstring &srcString) {
    std::wstring finalString = srcString;

    for (const auto &stopword : stop_word_list) {
      std::wregex pattern(L"\\b" + stopword + L"\\b");
      finalString = std::regex_replace(finalString, pattern, L"");
    }
    // stripping redundant tabs or spaces
    finalString =
        std::regex_replace(finalString, std::wregex(L"(\\t|\\s){2,}"), L"$1");
    return finalString;
  }
  /**
   * @brief Composes a string by combining individual consonants and vowels into
   * Amharic syllables.
   *
   * @param str The string consisting of individual consonants and vowels.
   * @return The composed Amharic string with syllables.
   */
  static std::wstring composeStringSyllables(const std::wstring &str) {
    std::u32string u32str = wstring_to_u32string(str);

    std::wstring composedString;
    for (size_t i = 0; i < str.length(); i++) {

      if (i >= str.length())
        break;
      char32_t ch = u32str[i];

      if (isConsonant(ch)) {

        if (i + 1 < str.length()) {
          uint32_t nextch = u32str[i + 1];
          if (isVowel(nextch)) {

            auto getVowelIndex = [](char32_t ch) {
              char32_t amharic_vowels[] = {U'ኧ', U'ኡ', U'ኢ', U'ኣ',
                                           U'ኤ', U'እ', U'ኦ'};

              int size = sizeof(amharic_vowels) / sizeof(char32_t);
              for (int i = 0; i < size; i++) {
                if (ch == amharic_vowels[i])
                  return i;
              }
              return 0;
            };

            int vowelIndex = getVowelIndex(nextch);

            uint32_t intialCodepoint = static_cast<uint32_t>(ch) - 5;

            wchar_t syllabel =
                static_cast<uint32_t>((intialCodepoint + vowelIndex));
            composedString += syllabel;
            i++;
            // contniue processig
          } else if (nextch == U'ዋ') {
            // handing derivate sounds like ሟ
            uint32_t codepoint = static_cast<uint32_t>(ch);
            uint32_t character_col = codepoint % 16;

            if (character_col > 8) {
              codepoint -= character_col;
              codepoint += 0xF;
              composedString += static_cast<wchar_t>(codepoint);
            } else {
              codepoint -= character_col;
              codepoint += 0x7;
              composedString += static_cast<wchar_t>(codepoint);
            }

            i++;
          } else {
            composedString += static_cast<wchar_t>(ch);
          }
        } else {
          composedString += static_cast<wchar_t>(ch);
        }
        //
      } else if (isVowel(ch)) {
        composedString += static_cast<wchar_t>(ch);

        //
      } else {
        composedString += static_cast<wchar_t>(ch);
        //
      }
    }
    return composedString;
  }

  std::vector<std::wstring> splitWString(const std::wstring &str,
                                         wchar_t delimiter) {
    std::wstringstream stream(str);
    std::wstring token;
    std::vector<std::wstring> words;

    while (std::getline(stream, token, delimiter)) {

      words.push_back(token);
    }
    return words;
  }

  static bool isDerivateSyllable(const char32_t ch) {
    if (!isSyllable(ch))
      return false;
    if (isVowel(ch))
      return false;
    if (isConsonant(ch))
      return false;
    uint32_t codepoint = static_cast<uint32_t>(ch);

    uint32_t character_col = codepoint % 16;
    if (character_col % 8 == 7)
      return true;
    return false;
  }
  std::wstring wstring_stem(const std::wstring &wstr, int suffix_level,
                            int prefix_level) {

    // NOTE: a certain amharic string could match more than one suffix/prefix,
    // the removal of certain suffix/prefix results in a small dam-lev distance
    // compared to others, maybe check for each matched suffixe/prefix and add
    // them to a priority queue based on their dam-lev distance
    const std::vector<std::wstring> suffix_list = {
        L"ኦችኣችኧውንንኣ", L"ኦችኣችህኡ", L"ኦችኣችኧውን", L"ኣችኧውንንኣ", L"ኦችኣችኧው", L"ኢዕኧልኧሽ",
        L"ኦችኣችን",     L"ኣውኢው",   L"ኣችኧውኣል",  L"ችኣት",     L"ችኣችህኡ",  L"ችኣችኧው",
        L"ኣልኧህኡ",     L"ኣውኦች",   L"ኣልኧህ",    L"ኣልኧሽ",    L"ኣልችህኡ",  L"ኣልኣልኧች",
        L"ብኣችኧውስ",    L"ብኣችኧው",  L"ኣችኧውን",   L"ኣልኧች",    L"ኣልኧን",   L"ኣልኣችህኡ",
        L"ኣችህኡን",     L"ኣችህኡ",   L"ኣችህኡት",   L"ውኦችንንኣ",  L"ውኦችን",   L"ኣችኧው",
        L"ውኦችኡን",     L"ውኦችኡ",   L"ውንኣ",     L"ኦችኡን",    L"ውኦች",    L"ኝኣንኧትም",
        L"ኝኣንኣ",      L"ኝኣንኧት",  L"ኝኣን",     L"ኝኣውም",    L"ኝኣው",    L"ኣውኣ",
        L"ብኧትን",      L"ኣችህኡም",  L"ችኣችን",    L"ኦችህ",     L"ኦችሽ",    L"ኦችኡ",
        L"ኦችኤ",       L"ኦውኣ",    L"ኦቿ",      L"ችው",      L"ችኡ",     L"ኤችኡ",
        L"ንኧው",       L"ንኧት",    L"ኣልኡ",     L"ኣችን",     L"ክኡም",    L"ክኡት",
        L"ክኧው",       L"ችን",     L"ችም",      L"ችህ",      L"ችሽ",     L"ችን",
        L"ችው",        L"ይኡሽን",   L"ይኡሽ",     L"ውኢ",      L"ኦችንንኣ",  L"ኣውኢ",
        L"ብኧት",       L"ኦች",     L"ኦችኡ",     L"ውኦን",     L"ኝኣ",     L"ኝኣውን",
        L"ኝኣው",       L"ኦችን",    L"ኣል",      L"ም",       L"ሽው",     L"ክም",
        L"ኧው",        L"ውኣ",     L"ትም",      L"ውኦ",      L"ውም",     L"ውን",
        L"ንም",        L"ሽን",     L"ኣች",      L"ኡት",      L"ኢት",     L"ክኡ",
        L"ኤ",         L"ህ",      L"ሽ",       L"ኡ",       L"ሽ",      L"ክ",
        L"ች",         L"ኡን",     L"ን",       L"ም",       L"ንኣ",     L"ዋ"};

    const std::vector<std::wstring> prefix_list = {
        L"ስልኧምኣይ", L"ይኧምኣት", L"ዕንድኧ", L"ይኧትኧ", L"ብኧምኣ", L"ብኧትኧ", L"ዕኧል", L"ስልኧ",
        L"ምኧስ",    L"ዕይኧ",   L"ይኣል",  L"ስኣት",  L"ስኣን",  L"ስኣይ",  L"ስኣል", L"ይኣስ",
        L"ይኧ",     L"ልኧ",    L"ብኧ",   L"ክኧ",   L"እን",   L"አል",   L"አስ",  L"ትኧ",
        L"አት",     L"አን",    L"አይ",   L"ይ",    L"አ",    L"እ"};

    std::wstring wsteam = normalizeString(wstr);

    // removing suffixes
    /* std::wcout << "////// removing suffixes \n" << std::endl; */
    for (const auto &suffix : suffix_list) {
      if (!(suffix_level))
        break;
      std::wregex pattern(decomposeStringSyllables(suffix) + L"\\b");
      std::wstring pre_removal_string =
          wsteam; // holding a copy of wstem before it gets eviscirated

      wsteam = std::regex_replace(wsteam, pattern, L"");
      if (wsteam != pre_removal_string) {
#ifdef ALP_STEMMER_DEBUG
        std::wcout << "[ " << decomposeStringSyllables(suffix) << " removed"

                   << "] " << composeStringSyllables(wsteam) << std::endl;
#endif
        suffix_level--;
      }
      if (composeStringSyllables(wsteam).length() <= 1) {
        wsteam = pre_removal_string;
        suffix_level++;
      }
    }

    /* std::wcout << "////// removing prefix \n" << std::endl; */
    for (const auto &prefix : prefix_list) {

      if (!(prefix_level))
        break;
      std::wregex pattern(L"\\b" + decomposeStringSyllables(prefix));
      std::wstring pre_removal_string =
          wsteam; // holding a copy of wstem before it gets eviscirated

      wsteam = std::regex_replace(wsteam, pattern, L"");

      if (wsteam != pre_removal_string) {
#ifdef ALP_STEMMER_DEBUG
        std::wcout << "[ " << decomposeStringSyllables(prefix) << " removed"

                   << "] " << composeStringSyllables(wsteam) << std::endl;

        prefix_level--;
#endif
      }
      if (composeStringSyllables(wsteam).length() <= 1) {
        wsteam = pre_removal_string;
        prefix_level++;
      }
    }

    return composeStringSyllables(wsteam);
  }
};
} // namespace amh_nlp
#endif
