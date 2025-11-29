#include "setup.hpp"
#include "flags.hpp"

int main(int argc, char* argv[]) {
  
  // Setup database and config directory. 
  initializeApplication();

  // Get command and description.
  ParsedCommand pc = parseCommand(argc, argv);
  executeCommand(pc);

  return 0;
}

