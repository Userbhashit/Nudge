#include <filesystem>
#include "sqlite3.h"
#include <print>

#include "setup.hpp"

namespace Setup {

  namespace {
    enum class SetupStatus {
      Uninitialized,
      DbAbsent,
      Complete,
    };

    bool validOS () {
      #if defined (__APPLE__) || defined (__LINUX__) 
        return true;
      #else
        return false;
      #endif
    }

    std::filesystem::path getHome() {
      const char* home = std::getenv("HOME");

      if (!home) {
      std::println(stderr, "Unable to get HOME directory.");
      std::exit(1);
      }

      return { home };
    }

    SetupStatus getSetupStatus() {

    auto configDirectoryPath = getHome() / ".nudge";
    auto dbPath = configDirectoryPath / "list.db";

    if (!std::filesystem::is_directory(configDirectoryPath)) {
      return SetupStatus::Uninitialized;
    }
  
    if (!std::filesystem::is_regular_file(dbPath)) {
      return SetupStatus::DbAbsent;
    }

    return SetupStatus::Complete;
  }

  void createDb() {

    auto dbPath = getHome() / ".nudge/list.db";

    sqlite3 *db;

    int rc = sqlite3_open(dbPath.string().c_str(), &db);

    if (rc != SQLITE_OK) {
      std::println(stderr, "Cannot create the database: {}", sqlite3_errmsg(db));
      std::exit(2);
    }

    sqlite3_close(db);
  }

  void setupEnvironment(SetupStatus setupStatus) {
    switch (setupStatus) {
      case SetupStatus::Uninitialized :
        std::filesystem::create_directory(getHome() / ".nudge");
        createDb();
        break;
      case SetupStatus::DbAbsent:
        createDb();
        break;
      default:
        break;
    }
  }

  } // private namespace
 
  void initializeApplication() {
      
      if (!validOS()) {
        std::exit(3);
      } 

      SetupStatus setupStatus = getSetupStatus();

      if (setupStatus != SetupStatus::Complete) {
        setupEnvironment(setupStatus);
      }
    }
};

