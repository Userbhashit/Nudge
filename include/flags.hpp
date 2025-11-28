#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <print>

enum class Flag {
  ADD,
  DEL,
  LIST_ALL,
  COMPLETE,
  SHOW_COMPLETE,
  ERROR,
};

void lower(std::string& str);
Flag getFlag(int argc, char* argv[], std::string& description);
void executeFlag(Flag command, std::string& description);


