#include <iostream>

#include "container.h"

int container_start(const char *name)
{
  std::cout << "[backend] Starting container: " << name << "\n";
  return 0;
}

int container_stop(const char *name)
{
  std::cout << "[backend] Stopping container: " << name << "\n";
  return 0;
}

