include(FetchContent)
FetchContent_Declare(
  SDL
  GIT_REPOSITORY "https://github.com/libsdl-org/SDL"
  GIT_TAG release-2.30.3
  )
FetchContent_Declare(
  imgui
  GIT_REPOSITORY "https://github.com/ocornut/imgui"
  GIT_TAG 6ccc561a2ab497ad4ae6ee1dbd3b992ffada35cb
  )
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(SDL_SHARED_ENABLED OFF)
# set(SDL_SHARED FALSE CACHE BOOL "Build a SDL shared library (if available)")
message(STATUS "Fetching SDL")
FetchContent_MakeAvailable(SDL)
add_subdirectory(imgui-setup)
