#include <print>
#include <format> 
#include <cstdlib>
#include <sstream>
#include <string_view> 

#include "paths.hpp"
#include "sqlite3.h"
#include "flags.hpp"
#include "database.hpp" 

namespace {
  int stringToId(const std::string& str) {
    try {
      return std::stoi(str);
    } catch (const std::invalid_argument&) {
      throw DatabaseException(std::format("Invalid task ID provided: '{}' is not a number.", str));
    } catch (const std::out_of_range&) {
      throw DatabaseException(std::format("Task ID out of range: '{}'.", str));
    }
  }
};

namespace database {

  DatabasePtr openDatabase() {
    sqlite3* raw_db = nullptr;
    int rc = sqlite3_open(Paths::dbPath.c_str(), &raw_db); 

    if (rc != SQLITE_OK) {
      const char* err_msg = raw_db ? sqlite3_errmsg(raw_db) : "Unknown error";

      if (raw_db) { 
        sqlite3_close(raw_db);
      }

      throw DatabaseException(std::format("Failed to open/create database (code: {}): {}", rc, err_msg));
    }

    return DatabasePtr(raw_db);
  }

  void setupTables() {
    auto db = openDatabase(); 
    char *errMsg = nullptr;

    int rc = sqlite3_exec(db.get(), Queries::TODO_TABLE_QUERY.data(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK && errMsg) {
      std::string error_detail = std::format("Error creating TODO table: {}", errMsg);
      sqlite3_free(errMsg);
      throw DatabaseException(error_detail); 
    }

    rc = sqlite3_exec(db.get(), Queries::COMPLETED_TABLE_QUERY.data(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK && errMsg) {
      std::string error_detail = std::format("Error creating COMPLETED table: {}", errMsg);
      sqlite3_free(errMsg);
      throw DatabaseException(error_detail);
    }

  }

  bool addTask(const ParsedCommand& pc) {
    try {
      auto db = openDatabase(); 

      sqlite3_stmt* raw_stmt = nullptr;

      int rc = sqlite3_prepare_v2(db.get(), Queries::INSERT_TASK_QUERY.data(), -1, &raw_stmt, nullptr);

      if (rc != SQLITE_OK) {
        throw DatabaseException(std::format("Error preparing statement (code: {}): {}", rc, sqlite3_errmsg(db.get())));
      }

      StatementPtr stmt(raw_stmt); 
      sqlite3_bind_text(stmt.get(), 1, pc.description.c_str(), -1, SQLITE_TRANSIENT);
      rc = sqlite3_step(stmt.get());

      return rc == SQLITE_DONE;

    } catch (const DatabaseException& e) {
      std::println(stderr, "DB Error adding task: {}", e.what());
      return false;
    } catch (const std::exception& e) {
      std::println(stderr, "General Error in addTask: {}", e.what());
      return false;
    }
  } 

  bool deleteTask(const ParsedCommand& pc) {
    try {
      if (pc.description.empty()) {
        throw DatabaseException("Deletion failed: No task ID provided.");
      }

      auto db = openDatabase(); 
      int task_id = stringToId(pc.description); // Convert ID string to int

      sqlite3_stmt* raw_stmt = nullptr;
      int rc = sqlite3_prepare_v2(db.get(), Queries::DELETE_TASK_QUERY.data(), -1, &raw_stmt, nullptr);
      if (rc != SQLITE_OK) {
        throw DatabaseException(std::format("Error preparing DELETE statement: {}", sqlite3_errmsg(db.get())));
      }

      StatementPtr stmt(raw_stmt); 
      sqlite3_bind_int(stmt.get(), 1, task_id);

      rc = sqlite3_step(stmt.get());

      if (rc != SQLITE_DONE) {
        throw DatabaseException(std::format("Execution failed (code: {}): {}", rc, sqlite3_errmsg(db.get())));
      }

      if (sqlite3_changes(db.get()) == 0) {
        std::println(stderr, "Warning: No task found with ID {}.", task_id);
        return false;
      }

      return true;
    } catch (const DatabaseException& e) {
      std::println(stderr, "DB Error deleting task: {}", e.what());
      return false;
    } catch (const std::exception& e) {
      std::println(stderr, "General Error in deleteTask: {}", e.what());
      return false;
    }
  }

  bool listAllCommands(const ParsedCommand& pc) {
    try {
      auto db = openDatabase(); 
      sqlite3_stmt* raw_stmt = nullptr;

      int rc = sqlite3_prepare_v2(db.get(), Queries::SELECT_ALL_TASKS_QUERY.data(), -1, &raw_stmt, nullptr);
      if (rc != SQLITE_OK) {
        throw DatabaseException(std::format("Error preparing LIST statement: {}", sqlite3_errmsg(db.get())));
      }

      StatementPtr stmt(raw_stmt); 

      bool tasks_found = false;
      std::println("--- Task List ---");
      std::println(" ID | Status  | Task");
      std::println("----|---------|-------------------------------------------------------");

      while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
        tasks_found = true;
        int id = sqlite3_column_int(stmt.get(), 0);
        const char* task_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        const char* status = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));

        std::println("{:<3} | {:<7} | {}", id, status, task_text ? task_text : "(No Description)");
      }

      if (rc != SQLITE_DONE) {
        throw DatabaseException(std::format("Error stepping through results (code: {}): {}", rc, sqlite3_errmsg(db.get())));
      }

      if (!tasks_found) {
        std::println("No tasks found.");
      }

      return true;
    } catch (const DatabaseException& e) {
      std::println(stderr, "DB Error listing tasks: {}", e.what());
      return false;
    } catch (const std::exception& e) {
      std::println(stderr, "General Error in listAllCommands: {}", e.what());
      return false;
    }
  }
} // Database
