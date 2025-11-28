#include <array>
#include <print>
#include <string>
#include <algorithm>
#include <unordered_map>

#include "flags.hpp"

void lower(std::string& str) {
  std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::tolower(c); });
}

Flag getFlag(int argc, char* argv[], std::string& description) {

  if (argc < 2) {
    std::println(stderr, "Usage: Nudge <command> <description>");
    std::exit(4);
  }

  std::string arg = argv[1];
  lower(arg);
  
  if (arg == "list") {
    if (argc >= 3 && std::string(argv[2]) == "-c") {
      return Flag::SHOW_COMPLETE;
    }
    return Flag::LIST_ALL;
  }

  static const std::unordered_map<std::string, Flag> lookup = {
    {"add", Flag::ADD},
    {"delete", Flag::DEL},
    {"complete", Flag::COMPLETE},
  };

  auto flag = lookup.find(arg);
  if (flag != lookup.end()) {
    if (argc >= 3) {
      for (int i = 2; i < argc; i++) {
        description += argv[i];
        if (i < argc - 1) description.push_back(' ');
      }
    }
    return flag->second;
  }

  return Flag::ERROR;
}

void executeFlag(Flag command, std::string& description) {
  switch (command) {
    case Flag::SHOW_COMPLETE:
      std::println("This will show all completed task.");
      break;
    case Flag::LIST_ALL:
      std::println("This will list all uncomepleted task.");
      break;
    case Flag::DEL:
      std::println("This will delete task if exits: {}.", description);
      break;
    case Flag::ADD:
      std::println("This will add task to list: {}", description);
      break;
    case Flag::COMPLETE:
      std::println("This will mark {} as complete", description);
      break;
    case Flag::ERROR:
      std::println(stderr, "Unrecognised Flag.");
      break;
  }
}

