#include <filesystem>
#include <cstdio>
#include <print>

#include "setup.hpp"
#include "paths.hpp"
#include "sqlite3.h"
#include "database.hpp"


namespace {
   enum class SetupStatus {
    Uninitialized,
    DbAbsent,
    Complete,
  };

  void checkOS () {
    #if defined (__APPLE__) || defined (__linux__) 
      return;
    #else
      std::println(stderr, "Sorry Nudge is only available only on MacOS and Linux.");
      std::exit(3);
    #endif
  }

  SetupStatus getSetupStatus() {
    if (!std::filesystem::is_directory(Paths::configDirectoryPath)) {
      return SetupStatus::Uninitialized;
    }

    if (!std::filesystem::is_regular_file(Paths::dbPath)) {
      return SetupStatus::DbAbsent;
    }

    return SetupStatus::Complete;
  }

  void setupEnvironment(SetupStatus setupStatus) {
    switch (setupStatus) {
      case SetupStatus::Uninitialized :
        std::filesystem::create_directory(Paths::configDirectoryPath);
        database::openDatabase();
        break;
      case SetupStatus::DbAbsent:
        database::openDatabase();
        break;
      default:
        break;
    }
  }

} // private namespace

void initializeApplication() {
  checkOS();
  const SetupStatus setupStatus = getSetupStatus();

  if (setupStatus != SetupStatus::Complete) {
    setupEnvironment(setupStatus);
  }

  database::setupTables();
}

