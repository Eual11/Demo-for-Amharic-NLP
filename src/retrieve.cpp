#include "../include/cosine_similarity.hpp"
#include "../include/imgui/imgui.h"
#include "../include/imgui/imgui_impl_sdl2.h"
#include "../include/imgui/imgui_impl_sdlrenderer2.h"
#include "../include/indexer.hpp"
#include "../include/utils.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
using namespace nlp_utils;
SDL_Window *gWindow = nullptr;
SDL_Renderer *gRenderer = nullptr;
// initalizes SDL and imgui
const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

bool validatePath(const char *path);
std::string LoadTextFile(const std::string &filePath);
void init() {

  if (SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "Error Occured: " << SDL_GetError() << std::endl;
    exit(1);
  }
  gWindow = SDL_CreateWindow("Retrive", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH,
                             WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

  gRenderer = SDL_CreateRenderer(
      gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  //
}
void close_program() {

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_DestroyRenderer(gRenderer);
  SDL_DestroyWindow(gWindow);
  SDL_Quit();
  std::clog << "Program Closed Gracefully\n";
}
void UpdateTheme(int theme_index) {
  switch (theme_index) {
  case 0: {
    ImGui::StyleColorsDark();
    break;
  }
  case 1: {
    ImGui::StyleColorsLight();
    break;
  }
  case 2: {
    ImGui::StyleColorsClassic();
    break;
  }
  }
}

int main(int, char **) {
  init();
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)(io);
  io.ConfigFlags |= ImGuiConfigFlags_None;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  // setting the dark mode interface
  ImGui::StyleColorsDark();
  /* ImGui::StyleColorsLight(); // for light mode */
  //
  //
  //
  ImGui_ImplSDL2_InitForSDLRenderer(gWindow, gRenderer);
  ImGui_ImplSDLRenderer2_Init(gRenderer);

  std::unordered_map<std::string, std::unordered_map<std::string, int>>
      forwardIndex;
  std::unordered_map<std::string, std::unordered_map<std::string, int>>
      invertedIndex;

  static std::vector<std::pair<std::string, double>> resultDoc;

  static std::map<std::string, double> similarityScores;
  static std::vector<float> scores;
  std::string indexFilename = "forward_index.txt";
  std::string invertedIndexFilename = "inverted_index.txt";

  bool quit = false;
  SDL_Event event;
  std::string indexerStatusString;
  bool queryExecuted = false;
  int selected_theme = 0;
  const char *Themes[3] = {"Dark", "Light", "Classic"};
  ImVec4 indexerStatusColor{0, 0, 0, 0};
  // Loading a better font
  ImVector<ImWchar> ranges;

  ImFontGlyphRangesBuilder builder;
  builder.AddText("Hello World");
  builder.AddChar(L'ሃ');
  builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
  builder.BuildRanges(&ranges);

  static const ImWchar amharic_ranges[] = {
      0x01, 0x137F, // Amharic block
      0,            // End of ranges
  };
  ImFont *font = io.Fonts->AddFontFromFileTTF("./font/noto_sans_ethiopic.ttf",
                                              18.0f, NULL, amharic_ranges);
  assert(font != NULL);
  while (!quit) {

    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) {
        quit = true;
      }
    }

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));
    ImGui::SetNextWindowCollapsed(true);
    ImGui::Begin("Hello World##window", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar()) {
      if (ImGui::Button("Theme"))
        ImGui::OpenPopup("my_select_popup");
      ImGui::SameLine();
      ImGui::TextUnformatted(selected_theme == -1 ? "<None>"
                                                  : Themes[selected_theme]);
      if (ImGui::BeginPopup("my_select_popup")) {
        ImGui::SeparatorText("Themes");
        for (int i = 0; i < IM_ARRAYSIZE(Themes); i++)
          if (ImGui::Selectable(Themes[i])) {
            selected_theme = i;
            UpdateTheme(selected_theme);
          }
        ImGui::EndPopup();
      }
      ImGui::EndMenuBar();
    }
    ImGui::SeparatorText("Generate Index/Inverted Index");

    ImGui::Text("Directory to Index");
    ImGui::SameLine();
    static char dirToIndex[1024];

    ImGui::InputTextWithHint("##dirinput",
                             "Directory to index (use . for current directory)",
                             dirToIndex, 1024);
    ImGui::Indent();
    if (ImGui::Button("Genrerate Index")) {
      bool validPath = validatePath(dirToIndex);
      if (validPath) {
        std::filesystem::path directoryPath = dirToIndex;
        std::vector<std::filesystem::path> filepaths;
        for (const auto &entry :
             std::filesystem::directory_iterator(directoryPath)) {
          if (entry.is_regular_file()) {
            filepaths.push_back(entry.path());
          }
        }
        createForwardIndex(filepaths, forwardIndex);
        std::cout << "size of created index " << forwardIndex.size() << '\n';
        int status = saveIndexToFile(indexFilename, forwardIndex);
        if (status) {
          // error occured
          indexerStatusColor = selected_theme == 0
                                   ? ImVec4(1, 0, 0, 1)
                                   : ImVec4(0.6f, 0.0f, 0.0f, 1.0f);
          ;
          indexerStatusString = "Failed to Save Index File!";

        } else {
          indexerStatusColor = selected_theme == 0
                                   ? ImVec4(.5f, 1.0f, 0.5f, 1.0f)
                                   : ImVec4(.0f, .5f, 0.0f, 1.0f);

          indexerStatusString =
              "Index File Generated for " +
              std::filesystem::canonical(std::filesystem::path(dirToIndex))
                  .string();
        }
      } else {
        // invalid path
        indexerStatusColor = selected_theme == 0
                                 ? ImVec4(1, 0, 0, 1)
                                 : ImVec4(0.6f, 0.0f, 0.0f, 1.0f);
        indexerStatusString = "Invalid Path!";
      }
    }

    ImGui::Unindent();
    ImGui::SameLine(ImGui::GetItemRectMax().x * 1.5f);
    if (ImGui::Button("Generate Inverted Index")) {
      // dome some stuff
      bool validPath = validatePath(dirToIndex);
      if (validPath) {
        std::filesystem::path directoryPath = dirToIndex;
        std::vector<std::filesystem::path> filepaths;
        for (const auto &entry :
             std::filesystem::directory_iterator(directoryPath)) {
          if (entry.is_regular_file()) {
            filepaths.push_back(entry.path());
          }
        }
        createForwardIndex(filepaths, forwardIndex);
        createInvertedIndex(forwardIndex, invertedIndex);

        int status =
            saveInvertedIndexToFile(invertedIndexFilename, invertedIndex);
        if (status) {
          indexerStatusColor = selected_theme == 0
                                   ? ImVec4(1, 0, 0, 1)
                                   : ImVec4(0.6f, 0.0f, 0.0f, 1.0f);
          indexerStatusString = "Failed to Save Inverted Index File!";

          // error occured
        } else {
          indexerStatusColor = selected_theme == 0
                                   ? ImVec4(.5f, 1.0f, 0.5f, 1.0f)
                                   : ImVec4(.0f, .5f, 0.0f, 1.0f);
          indexerStatusString =
              "Inverted Index File Generated for " +
              std::filesystem::canonical(std::filesystem::path(dirToIndex))
                  .string();
        }
      } else {
        indexerStatusColor = selected_theme == 0
                                 ? ImVec4(1, 0, 0, 1)
                                 : ImVec4(0.6f, 0.0f, 0.0f, 1.0f);

        indexerStatusString = "Invalid Path!";
        // invalid path
      }
    }
    ImGui::TextColored(indexerStatusColor, "%s",
                       (char *)indexerStatusString.data());
    ImGui::SeparatorText("Retrive Information");

    ImGui::Text("Query: ");
    ImGui::SameLine();
    static char queryString[1024] = {u8'\0'};
    ImGui::InputTextWithHint("##queryinput", "Search...", queryString, 1024);
    ImGui::SameLine();

    if (ImGui::Button(u8"Search")) {
      queryExecuted = true;
      std::vector<std::string> queryTokens = tokenize(std::string(queryString));
      std::map<std::string, int> queryVector;
      for (const auto &token : queryTokens) {
        std::cout << token << std::endl;
        queryVector[token]++;
      }

      forwardIndex.clear();
      int status = loadIndexFromFile(indexFilename, forwardIndex);
      if (status) {
        ImVec4 accent = selected_theme == 0 ? ImVec4(1, 0, 0, 1)
                                            : ImVec4(0.6f, 0.0f, 0.0f, 1.0f);
        ImGui::TextColored(accent, "Failed to Load Index");
      }
      // Compute cosine similarity between query and documents
      similarityScores.clear();
      for (const auto &[docID, docVector] : forwardIndex) {
        double similarity = cosineSimilarity(docVector, queryVector);
        if (similarity > 0) {
          similarityScores[docID] = similarity;
        }
      }

      // Sort documents by similarity scores in descending order

      std::vector<std::pair<std::string, double>> sortedDocs(
          similarityScores.begin(), similarityScores.end());
      std::sort(
          sortedDocs.begin(), sortedDocs.end(),
          [](const auto &a, const auto &b) { return a.second > b.second; });
      if (sortedDocs.empty()) {
        std::cout << "No documents found matching the query." << std::endl;
      } else {
        std::cout << "Documents matching the query:" << std::endl;
        for (const auto &[docID, score] : sortedDocs) {
          std::cout << docID << " (score: " << score << ")" << std::endl;
        }
      }

      resultDoc = std::move(sortedDocs);

      // Output the sorted documents

      // show results
    }
    if (!resultDoc.empty()) {
      ImGui::Text("Results: ");
      ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_BordersOuterH;
      ImGui::Indent();
      ImVec2 outer_size = ImVec2(ImGui::GetContentRegionAvail().x,
                                 ImGui::GetContentRegionAvail().y / 2);
      scores.clear();
      if (ImGui::BeginTable("scrollY", 4, flags, outer_size)) {

        ImGui::TableSetupScrollFreeze(0, 1); // make the top row always visible
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Cousin Similarity",
                                ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("Access", ImGuiTableColumnFlags_None);

        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        int idx = 0;
        clipper.Begin(resultDoc.size());
        while (clipper.Step()) {
          for (int row = clipper.DisplayStart; row < clipper.DisplayEnd;
               row++) {
            const auto &[docID, score] = resultDoc[row];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", idx);
            ImGui::TableNextColumn();
            ImGui::Text("%s", docID.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%f", score);
            scores.push_back(score);
            ImGui::TableNextColumn();
            ImGui::PushID(idx);
            if (ImGui::Button("Open")) {
              ImGui::OpenPopup(docID.c_str());
            }
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));
            if (ImGui::BeginPopupModal(docID.c_str(), nullptr,
                                       ImGuiWindowFlags_None)) {
              if (ImGui::Button("close"))
                ImGui::CloseCurrentPopup();
              std::filesystem::path filepath(dirToIndex);
              filepath /= docID;
              if (std::filesystem::exists(filepath) &&
                  std::filesystem::is_regular_file(filepath)) {
                std::string text = LoadTextFile(filepath.string());
                ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), true,
                                  ImGuiWindowFlags_HorizontalScrollbar |
                                      ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::TextUnformatted(text.c_str());
                ImGui::EndChild();
              }

              ImGui::EndPopup();
            }

            ImGui::PopID();

            idx++;
          }
        }
        ImGui::EndTable();
      }

      ImGui::NewLine();
      ImVec4 accent = selected_theme == 0 ? ImVec4{0.0f, 1.0f, 1.0f, 1.0f}
                                          : ImVec4{0.0f, 0.7f, 0.7f, 1.0f};
      ImGui::TextColored(accent, "Frequency Distrubution");

      ImGui::PlotHistogram("##FrequencyDistrubution", scores.data(),
                           scores.size(), 0, nullptr, 0, scores.front(),
                           ImVec2(ImGui::GetContentRegionAvail().x,
                                  ImGui::GetContentRegionAvail().y / 3));
    } else if (queryExecuted) {
      ImGui::Text("No Results for this query ");
    }
    ImGui::End();
    ImGui::Render();

    SDL_RenderSetScale(gRenderer, io.DisplayFramebufferScale.x,
                       io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(gRenderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

    SDL_RenderPresent(gRenderer);
  }
  close_program();

  return 0;
}
bool validatePath(const char *path) {
  std::filesystem::path directoryPath = path;

  if (!std::filesystem::exists(directoryPath) ||
      !std::filesystem::is_directory(directoryPath)) {
    std::cerr << "The provided path is not a valid directory." << std::endl;
    return false;
  }
  return true;
}
std::string LoadTextFile(const std::string &filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filePath << std::endl;
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}
