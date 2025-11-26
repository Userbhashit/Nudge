#include <print>

#include "database.hpp"
#include "sqlite3.h"

namespace database {

  void createDb() {

    sqlite3 *db;

    int rc = sqlite3_open(Paths::dbPath.string().c_str(), &db);

    if (rc != SQLITE_OK) {
      std::println(stderr, "Cannot create the database: {}", sqlite3_errmsg(db));
      std::exit(2);
    }

    sqlite3_close(db);
  }

  void setupTables() {
    sqlite3 *db;

    int rc = sqlite3_open(Paths::dbPath.string().c_str(), &db);
    char *errMsg = nullptr;

    if (rc != SQLITE_OK) {
      std::println(stderr, "Cannot create the database: {}", sqlite3_errmsg(db));
      std::exit(2);
    }  

    sqlite3_exec(db, Queries::TODO_TABLE_QUERY.data(), nullptr, nullptr, &errMsg);

    if (errMsg) {
      std::println(stderr, "Error: {}", errMsg);
      sqlite3_free(errMsg);
      sqlite3_close(db);
      std::exit(4);
    }

    sqlite3_exec(db, Queries::COMPLETED_TABLE_QUERY.data(), nullptr, nullptr, &errMsg);

    if (errMsg) {
      std::println(stderr, "Error: {}", errMsg);
      sqlite3_free(errMsg);
      sqlite3_close(db);
      std::exit(4);
    }  

    sqlite3_close(db);
  }

} // Database

