#pragma once

#include <filesystem>

namespace Paths {

  std::filesystem::path getHome();
   
  inline constexpr std::string conifgDirectoryName = ".nudge";
  inline constexpr std::string dbName = "list.db";

  inline const auto configDirectoryPath = getHome() / conifgDirectoryName;
  inline const auto dbPath = configDirectoryPath / dbName;

}
