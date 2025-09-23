#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <string>
#include <iostream>

#include "container.h"

static int extract_tarball(const char *tarball, const char *dest) 
{
  std::string cmd = "tar -xf ";
  cmd += tarball;
  cmd += " -C ";
  cmd += dest;
  return system(cmd.c_str());
}

extern "C" int run_container(const char *image_path, const char *command) 
{
  char temp_dir[] = "/tmp/containerXXXXXX";

  if (!mkdtemp(temp_dir)) {
    perror("mkdtemp failed");
    return -1;
  }

  if (extract_tarball(image_path, temp_dir) != 0) {
    std::cerr << "Failed to extract image tarball\n";
    return -2;
  }

  pid_t pid = fork();

  if (pid < 0) {
    perror("fork failed");
    return -3;
  }

  if (pid == 0) {
    if (chroot(temp_dir) != 0) {
      perror("chroot failed");
      exit(1);
    }

    if (chdir("/") != 0) {
      perror("chdir failed");
      exit(1);
    }

    execl("/bin/sh", "sh", "-c", command, (char*)NULL);
    
    perror("execl failed");
    exit(1);
  } else {
    int status = 0;
    waitpid(pid, &status, 0);
    std::string cleanup = "rm -rf ";
    cleanup += temp_dir;
    system(cleanup.c_str());
    return WIFEXITED(status) ? WEXITSTATUS(status) : -4;
  }
}


