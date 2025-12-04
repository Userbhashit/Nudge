#pragma once

#include <string_view>
#include <memory>
#include <stdexcept>

#include "sqlite3.h"
#include "flags.hpp"

struct sqlite3;
struct sqlite3_stmt;

struct SqliteDeleter {
  void operator()(sqlite3* db) const {
    if (db) {
      sqlite3_close(db); 
    }
  }
};

struct StmtDeleter {
  void operator()(sqlite3_stmt* stmt) const {
    if (stmt) {
      sqlite3_finalize(stmt); 
    }
  }
};

using DatabasePtr = std::unique_ptr<sqlite3, SqliteDeleter>;
using StatementPtr = std::unique_ptr<sqlite3_stmt, StmtDeleter>;

class DatabaseException : public std::runtime_error {
  public:
    DatabaseException(const std::string& message) : std::runtime_error(message) {}
};

namespace Queries {
  // Use local device time for timestamps by using SQLite's 'localtime' modifier.
  // SQLite's CURRENT_TIMESTAMP is UTC; using datetime('now','localtime') stores local time.
  inline constexpr std::string_view TODO_TABLE_QUERY = R"(
        CREATE TABLE IF NOT EXISTS tasks(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        task TEXT NOT NULL,
        status TEXT DEFAULT 'pending', 
        created_at DATETIME DEFAULT (datetime('now','localtime')));
    )";

  inline constexpr std::string_view COMPLETED_TABLE_QUERY = R"(
        CREATE TABLE IF NOT EXISTS completed (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        task TEXT NOT NULL,
        completed_at DATETIME DEFAULT (datetime('now','localtime')));
    )";

  inline constexpr std::string_view INSERT_TASK_QUERY = "INSERT INTO tasks (task, status) VALUES (?, 'pending');";
  inline constexpr std::string_view DELETE_TASK_QUERY = "DELETE FROM tasks WHERE id = ?;";

  inline constexpr std::string_view SELECT_ALL_TASKS_QUERY = "SELECT id, task, status, created_at FROM tasks ORDER BY created_at DESC;";
  inline constexpr std::string_view SELECT_COMPLETED_TASK_QUERY = "SELECT task, completed_at FROM completed ORDER BY completed_at DESC;";
  inline constexpr std::string_view SELECT_TASK_BY_ID_QUERY = "SELECT task FROM tasks WHERE id = ?;";
  inline constexpr std::string_view INSERT_COMPLETED_TASK_QUERY = "INSERT INTO completed (task) VALUES (?);";
  inline constexpr std::string_view BEGIN_TRANSACTION_QUERY = "BEGIN TRANSACTION;";
} // Queries

namespace database {
  DatabasePtr openDatabase(); 
  void setupTables(); 
  bool addTask(const ParsedCommand& pc);
  bool deleteTask(const ParsedCommand& pc);
  bool listAllTasks();
  bool listAllBoth();
  bool markTaskComplete(const ParsedCommand& pc);
  bool listAllCompletedCommands(const ParsedCommand& pc);
} // Database
