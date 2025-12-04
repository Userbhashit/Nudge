#include <array>
#include <print>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <cstdlib>
#include <format>

#include "flags.hpp"
#include "database.hpp"

void lower(std::string& str) {
  std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::tolower(c); });
}

std::string joinArguments(int argc, char* argv[], int startIndex) {
    std::string description;
    for (int i = startIndex; i < argc; i++) {
        if (!description.empty()) description.push_back(' ');
        description += argv[i];
    }
    return description;
}

ParsedCommand parseCommand(int argc, char* argv[]) {

    if (argc < 2) {
        return {Flag::ERROR, ""};
    }

    std::string cmd = argv[1];
    lower(cmd);

    if (cmd == "list") {
      if (argc >= 3) {
        std::string opt = argv[2];
        if (opt == "-c") return {Flag::SHOW_COMPLETE_TASKS, ""};
        if (opt == "-a") return {Flag::LIST_ALL, ""};
      }
      return {Flag::LIST_PENDING, ""};
    }

    static const std::unordered_map<std::string, Flag> lookup = {
        {"add", Flag::ADD},
        {"delete", Flag::DEL},
      {"complete", Flag::COMPLETE},
      {"comeplete", Flag::COMPLETE},
      {"notification", Flag::NOTIFY},
      {"notify", Flag::NOTIFY},
    };

    auto it = lookup.find(cmd);
    if (it != lookup.end()) {
      std::string desc = joinArguments(argc, argv, 2);
      return {it->second, desc};
    }

    return {Flag::ERROR, ""};
}

void executeCommand(const ParsedCommand& pc) {
  switch (pc.flag) {
    case Flag::SHOW_COMPLETE_TASKS:
      std::println("Completed tasks:");
      if(!database::listAllCompletedCommands(pc)) {
        std::println(stderr, "Failed to list completed tasks");
      }
      break;
    case Flag::LIST_ALL:
      std::println("Listing all tasks (pending + completed):");
      if (!database::listAllBoth()) {
        std::println(stderr, "Failed to list all tasks.");
      }
      break;
    case Flag::LIST_PENDING:
      std::println("Pending tasks:");
      if (!database::listAllTasks()) {
        std::println(stderr, "Failed to list pending tasks.");
      }
      break;
    case Flag::DEL:
      if (database::deleteTask(pc)) {
        std::println("Successfully deleted task.");
      } else {
        std::println(stderr, "Deletion failed.");
      }
      break;
    case Flag::ADD:
      if (database::addTask(pc)) {
        std::println("Task: \"{}\" was added.", pc.description);
      } else {
        std::println(stderr, "Failed to add task.");
      }
      break;
    case Flag::MARK_COMPLETE:
    case Flag::COMPLETE: {
      if (database::markTaskComplete(pc)) {
        if (pc.description.empty()) {
          std::println("Marked first pending task as complete.");
        } else {
          std::println("Task '{}' marked as complete.", pc.description);
        }
      } else {
        std::println(stderr, "Failed to mark task as complete.");
      }
      } break;
    case Flag::NOTIFY: {
      int pending = database::countPendingTasks();
      std::string msg;
      if (pending <= 0) {
        msg = "All done for today ;)";
      } else {
        msg = std::format("{} left for today", pending);
      }

#if defined(__APPLE__)
      // Escape double quotes in message for AppleScript
      std::string safe = msg;
      size_t pos = 0;
      while ((pos = safe.find('"', pos)) != std::string::npos) {
        safe.replace(pos, 1, "\\\"");
        pos += 2;
      }
      std::string cmd = std::format("osascript -e 'display notification \"{}\" with title \"Nudge\"'", safe);
      std::system(cmd.c_str());
#elif defined(__linux__)
      std::string cmd = std::format("notify-send \"Nudge\" \"{}\"", msg);
      std::system(cmd.c_str());
#else
      std::println("{}", msg);
#endif
    } break;
    case Flag::ERROR:
      std::println(stderr, "Unknown command.");
      break;
  }
}
