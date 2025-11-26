#include <filesystem>
#include <print>

#include "paths.hpp"

namespace Paths {
  std::filesystem::path getHome() {
    const char* home = std::getenv("HOME");

    if (!home) {
      std::println(stderr, "Unable to get HOME directory.");
      std::exit(1);
    }

    return { home };
  }
}
