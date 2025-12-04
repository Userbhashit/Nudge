#include <print>
#include <format> 
#include <cstdlib>
#include <sstream>
#include <iostream>
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

  bool listAllTasks() {
    try {
      auto db = openDatabase(); 
      sqlite3_stmt* raw_stmt = nullptr;

      int rc = sqlite3_prepare_v2(db.get(), Queries::SELECT_ALL_TASKS_QUERY.data(), -1, &raw_stmt, nullptr);
      if (rc != SQLITE_OK) {
        throw DatabaseException(std::format("Error preparing LIST statement: {}", sqlite3_errmsg(db.get())));
      }

      StatementPtr stmt(raw_stmt); 

      bool tasks_found = false;
      std::println(" ID | Task");
      std::println("----|-------------------------------------------------------");

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

  bool markTaskComplete(const ParsedCommand& pc) {
    try {
      auto db = openDatabase();

      // If no description provided, mark the first pending task as completed
      std::string desc = pc.description;
      auto ltrim = [](std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
      };
      auto rtrim = [](std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
      };
      ltrim(desc);
      rtrim(desc);

      if (desc.empty()) {
        // find the first pending task (oldest)
        sqlite3_stmt* first_stmt_raw = nullptr;
        int rc = sqlite3_prepare_v2(db.get(), "SELECT id, task FROM tasks ORDER BY created_at ASC LIMIT 1;", -1, &first_stmt_raw, nullptr);
        if (rc != SQLITE_OK) {
          throw DatabaseException(std::format("Failed to prepare select-first statement: {}", sqlite3_errmsg(db.get())));
        }
        StatementPtr first_stmt(first_stmt_raw);
        if (sqlite3_step(first_stmt.get()) == SQLITE_ROW) {
          int found_id = sqlite3_column_int(first_stmt.get(), 0);
          const char* txt = reinterpret_cast<const char*>(sqlite3_column_text(first_stmt.get(), 1));
          desc = std::to_string(found_id);
        } else {
          throw DatabaseException("No pending tasks to complete.");
        }
      }

      std::string lower_desc = desc;
      std::transform(lower_desc.begin(), lower_desc.end(), lower_desc.begin(), [](unsigned char c){ return std::tolower(c); });

      // Begin transaction
      sqlite3_exec(db.get(), Queries::BEGIN_TRANSACTION_QUERY.data(), 0, 0, 0);

      if (lower_desc.rfind("like ", 0) == 0) {
        // Pattern-based completion: move all matching tasks
        std::string pattern = desc.substr(5); // after "LIKE "
        ltrim(pattern);
        rtrim(pattern);
        if (pattern.empty()) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException("LIKE pattern is empty.");
        }

        std::string likePattern = "%" + pattern + "%";

        // Prepare statements
        sqlite3_stmt* select_stmt_raw = nullptr;
        int rc = sqlite3_prepare_v2(db.get(), "SELECT id, task FROM tasks WHERE task LIKE ?;", -1, &select_stmt_raw, nullptr);
        if (rc != SQLITE_OK) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Failed to prepare select LIKE statement: {}", sqlite3_errmsg(db.get())));
        }
        StatementPtr select_stmt(select_stmt_raw);
        sqlite3_bind_text(select_stmt.get(), 1, likePattern.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_stmt* insert_stmt_raw = nullptr;
        rc = sqlite3_prepare_v2(db.get(), Queries::INSERT_COMPLETED_TASK_QUERY.data(), -1, &insert_stmt_raw, nullptr);
        if (rc != SQLITE_OK) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Failed to prepare insert statement: {}", sqlite3_errmsg(db.get())));
        }
        StatementPtr insert_stmt(insert_stmt_raw);

        sqlite3_stmt* delete_stmt_raw = nullptr;
        rc = sqlite3_prepare_v2(db.get(), Queries::DELETE_TASK_QUERY.data(), -1, &delete_stmt_raw, nullptr);
        if (rc != SQLITE_OK) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Failed to prepare delete statement: {}", sqlite3_errmsg(db.get())));
        }
        StatementPtr delete_stmt(delete_stmt_raw);

        bool anyMoved = false;
        while ((rc = sqlite3_step(select_stmt.get())) == SQLITE_ROW) {
          anyMoved = true;
          int id = sqlite3_column_int(select_stmt.get(), 0);
          const char* task_text_c = reinterpret_cast<const char*>(sqlite3_column_text(select_stmt.get(), 1));
          std::string task_text = task_text_c ? task_text_c : "";

          // Insert into completed
          sqlite3_reset(insert_stmt.get());
          sqlite3_clear_bindings(insert_stmt.get());
          sqlite3_bind_text(insert_stmt.get(), 1, task_text.c_str(), -1, SQLITE_TRANSIENT);
          if (sqlite3_step(insert_stmt.get()) != SQLITE_DONE) {
            sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
            throw DatabaseException("Failed to insert into completed table during LIKE operation.");
          }

          // Delete from tasks
          sqlite3_reset(delete_stmt.get());
          sqlite3_clear_bindings(delete_stmt.get());
          sqlite3_bind_int(delete_stmt.get(), 1, id);
          if (sqlite3_step(delete_stmt.get()) != SQLITE_DONE) {
            sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
            throw DatabaseException("Failed to delete from tasks table during LIKE operation.");
          }
        }

        if (rc != SQLITE_DONE) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Error iterating LIKE results: {}", sqlite3_errmsg(db.get())));
        }

        if (!anyMoved) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("No tasks matched pattern '{}'.", pattern));
        }

        sqlite3_exec(db.get(), "COMMIT;", 0, 0, 0);
        return true;
      }

      // Otherwise try to parse an ID; if parsing fails, treat the input as a pattern
      int task_id = -1;
      bool is_id = true;
      try {
        task_id = stringToId(desc);
      } catch (const DatabaseException&) {
        is_id = false;
      }

      if (!is_id) {
        // Treat desc as a substring pattern and move matching tasks (same as LIKE behaviour)
        std::string pattern = desc;
        ltrim(pattern);
        rtrim(pattern);
        if (pattern.empty()) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException("Pattern is empty.");
        }

        std::string likePattern = "%" + pattern + "%";

        sqlite3_stmt* select_stmt_raw = nullptr;
        int rc = sqlite3_prepare_v2(db.get(), "SELECT id, task FROM tasks WHERE task LIKE ?;", -1, &select_stmt_raw, nullptr);
        if (rc != SQLITE_OK) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Failed to prepare select pattern statement: {}", sqlite3_errmsg(db.get())));
        }
        StatementPtr select_stmt(select_stmt_raw);
        sqlite3_bind_text(select_stmt.get(), 1, likePattern.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_stmt* insert_stmt_raw = nullptr;
        rc = sqlite3_prepare_v2(db.get(), Queries::INSERT_COMPLETED_TASK_QUERY.data(), -1, &insert_stmt_raw, nullptr);
        if (rc != SQLITE_OK) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Failed to prepare insert statement: {}", sqlite3_errmsg(db.get())));
        }
        StatementPtr insert_stmt(insert_stmt_raw);

        sqlite3_stmt* delete_stmt_raw = nullptr;
        rc = sqlite3_prepare_v2(db.get(), Queries::DELETE_TASK_QUERY.data(), -1, &delete_stmt_raw, nullptr);
        if (rc != SQLITE_OK) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Failed to prepare delete statement: {}", sqlite3_errmsg(db.get())));
        }
        StatementPtr delete_stmt(delete_stmt_raw);

        bool anyMoved = false;
        while ((rc = sqlite3_step(select_stmt.get())) == SQLITE_ROW) {
          anyMoved = true;
          int id = sqlite3_column_int(select_stmt.get(), 0);
          const char* task_text_c = reinterpret_cast<const char*>(sqlite3_column_text(select_stmt.get(), 1));
          std::string task_text = task_text_c ? task_text_c : "";

          sqlite3_reset(insert_stmt.get());
          sqlite3_clear_bindings(insert_stmt.get());
          sqlite3_bind_text(insert_stmt.get(), 1, task_text.c_str(), -1, SQLITE_TRANSIENT);
          if (sqlite3_step(insert_stmt.get()) != SQLITE_DONE) {
            sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
            throw DatabaseException("Failed to insert into completed table during pattern operation.");
          }

          sqlite3_reset(delete_stmt.get());
          sqlite3_clear_bindings(delete_stmt.get());
          sqlite3_bind_int(delete_stmt.get(), 1, id);
          if (sqlite3_step(delete_stmt.get()) != SQLITE_DONE) {
            sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
            throw DatabaseException("Failed to delete from tasks table during pattern operation.");
          }
        }

        if (rc != SQLITE_DONE) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Error iterating pattern results: {}", sqlite3_errmsg(db.get())));
        }

        if (!anyMoved) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("No tasks matched pattern '{}'.", pattern));
        }

        sqlite3_exec(db.get(), "COMMIT;", 0, 0, 0);
        return true;
      }

      // 1. Select the task from the tasks table by id
      sqlite3_stmt* select_stmt_raw = nullptr;
      int rc = sqlite3_prepare_v2(db.get(), Queries::SELECT_TASK_BY_ID_QUERY.data(), -1, &select_stmt_raw, nullptr);
      if (rc != SQLITE_OK) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Failed to prepare select statement: {}", sqlite3_errmsg(db.get())));
      }
      StatementPtr select_stmt(select_stmt_raw);
      sqlite3_bind_int(select_stmt.get(), 1, task_id);

      std::string task_text;
      if (sqlite3_step(select_stmt.get()) == SQLITE_ROW) {
          const char* txt = reinterpret_cast<const char*>(sqlite3_column_text(select_stmt.get(), 0));
          task_text = txt ? txt : "";
      } else {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Task with ID {} not found.", task_id));
      }

      // Insert into completed table
      sqlite3_stmt* insert_stmt_raw = nullptr;
      rc = sqlite3_prepare_v2(db.get(), Queries::INSERT_COMPLETED_TASK_QUERY.data(), -1, &insert_stmt_raw, nullptr);
      if (rc != SQLITE_OK) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Failed to prepare insert statement: {}", sqlite3_errmsg(db.get())));
      }
      StatementPtr insert_stmt(insert_stmt_raw);
      sqlite3_bind_text(insert_stmt.get(), 1, task_text.c_str(), -1, SQLITE_TRANSIENT);
      if (sqlite3_step(insert_stmt.get()) != SQLITE_DONE) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException("Failed to insert into completed table.");
      }

      // Delete from tasks
      sqlite3_stmt* delete_stmt_raw = nullptr;
      rc = sqlite3_prepare_v2(db.get(), Queries::DELETE_TASK_QUERY.data(), -1, &delete_stmt_raw, nullptr);
      if (rc != SQLITE_OK) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException(std::format("Failed to prepare delete statement: {}", sqlite3_errmsg(db.get())));
      }
      StatementPtr delete_stmt(delete_stmt_raw);
      sqlite3_bind_int(delete_stmt.get(), 1, task_id);
      if (sqlite3_step(delete_stmt.get()) != SQLITE_DONE) {
          sqlite3_exec(db.get(), "ROLLBACK;", 0, 0, 0);
          throw DatabaseException("Failed to delete from tasks table.");
      }

      // Commit transaction
      sqlite3_exec(db.get(), "COMMIT;", 0, 0, 0);

      return true;

    } catch (const DatabaseException& e) {
      std::println(stderr, "DB Error marking task complete: {}", e.what());
      return false;
    } catch (const std::exception& e) {
      std::println(stderr, "General Error in markTaskComplete: {}", e.what());
      return false;
    }
  }

  bool listAllCompletedCommands(const ParsedCommand& ) {
     try {
      auto db = openDatabase(); 
      sqlite3_stmt* raw_stmt = nullptr;

      int rc = sqlite3_prepare_v2(db.get(), Queries::SELECT_COMPLETED_TASK_QUERY.data(), -1, &raw_stmt, nullptr);
      if (rc != SQLITE_OK) {
        throw DatabaseException(std::format("Error preparing LIST statement: {}", sqlite3_errmsg(db.get())));
      }

      StatementPtr stmt(raw_stmt); 

      bool tasks_found = false;
      std::println("--- Task List ---");
      std::println("    completed at    |                       Task");
      std::println("--------------------|-------------------------------------------------------");

      while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
        tasks_found = true;
        const char* taskText = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        const char* completedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));

        std::println("{} |      {}", completedAt, taskText ? taskText : "(No Description)");
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

  bool listAllBoth() {
    // Print pending first, then completed
    bool okPending = listAllTasks();
    std::println("");
    std::println("--- Completed Tasks ---");
    bool okCompleted = listAllCompletedCommands(ParsedCommand{Flag::SHOW_COMPLETE_TASKS, ""});
    return okPending && okCompleted;
  }
} // Database
