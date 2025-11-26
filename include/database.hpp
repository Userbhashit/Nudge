#pragma once

#include "sqlite3.h"
#include "paths.hpp"


namespace Queries {
    inline constexpr std::string_view TODO_TABLE_QUERY = R"(
        CREATE TABLE IF NOT EXISTS tasks(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        task TEXT NOT NULL,
        status TEXT,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
      );
    )";

    inline constexpr std::string_view COMPLETED_TABLE_QUERY = R"(
        CREATE TABLE IF NOT EXISTS completed (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          task TEXT NOT NULL,
          completed_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
} // Queries

namespace database {
  void createDb();
  void setupTables();
} // Database

