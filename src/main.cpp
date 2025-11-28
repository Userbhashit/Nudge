#include "setup.hpp"
#include "flags.hpp"

int main(int argc, char* argv[]) {
  
  // Setup database and config directory. 
  initializeApplication();

  // Get command and description.
  std::string description("");
  Flag command = getFlag(argc, argv, description);
  executeFlag(command, description);


  return 0;
}

