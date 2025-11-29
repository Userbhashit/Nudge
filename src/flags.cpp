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
        bool showCompleted = (argc >= 3 && std::string(argv[2]) == "-c");
        return {showCompleted ? Flag::SHOW_COMPLETE : Flag::LIST_ALL, ""};
    }

    static const std::unordered_map<std::string, Flag> lookup = {
        {"add", Flag::ADD},
        {"delete", Flag::DEL},
        {"complete", Flag::COMPLETE},
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
        case Flag::SHOW_COMPLETE:
            std::println("Showing completed tasks...");
            break;
        case Flag::LIST_ALL:
            std::println("Listing all tasks...");
            break;
        case Flag::DEL:
            std::println("Deleting: {}", pc.description);
            break;
        case Flag::ADD:
            std::println("Adding: {}", pc.description);
            break;
        case Flag::COMPLETE:
            std::println("Marking complete: {}", pc.description);
            break;
        case Flag::ERROR:
            std::println(stderr, "Unknown command.");
            break;
    }
}
