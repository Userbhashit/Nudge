#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <print>

enum class Flag {
  ADD,
  DEL,
  COMPLETE,
  LIST_ALL,      // -a : show all (pending + completed)
  LIST_PENDING,  // default: show pending tasks
  MARK_COMPLETE,
  SHOW_COMPLETE_TASKS,
  ERROR,
};

struct ParsedCommand {
  Flag flag;
  std::string description;
};

void lower(std::string& str);
std::string joinArguments(int argc, char* argv[], int startIndex);
ParsedCommand parseCommand(int argc, char* argv[]);
void executeCommand(const ParsedCommand& command);


