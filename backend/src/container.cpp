#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <signal.h>

#include <string>
#include <vector>
#include <iostream>

#include "container.h"
#include "internal.h"

int run(const std::string &cmd)
{
  int rc = system(cmd.c_str());
  if (rc == -1) {
    perror("system");
    return -1;
  }
  if (WIFEXITED(rc)) {
    return WEXITSTATUS(rc);
  }
  if (WIFSIGNALED(rc)) {
    return 128 + WTERMSIG(rc);
  }
  return -1;
}

bool exists(const char *path)
{
  struct stat st;
  return stat(path, &st) == 0;
}

int extract(const std::string &tarball, const std::string &dest)
{
  std::string cmd = "tar -xf '" + tarball + "' -C '" + dest + "'";
  return system(cmd.c_str());
}

std::vector<std::string> parse_layers(const std::string &meta_dir)
{
  std::vector<std::string> layers;
  std::string script_path = "./backend/scripts/parse_manifest.py";
  std::string manifest_path = meta_dir + "/manifest.json";
  std::string cmd = "python3 '" + script_path + "' '" + manifest_path + "'";
  FILE *fp = popen(cmd.c_str(), "r");
  if (!fp) return layers;

  char buf[READ_BUF];
  while (fgets(buf, sizeof(buf), fp)) {
    std::string line(buf);
    while (!line.empty() && (line.back()=='\n' || line.back()=='\r')) line.pop_back();
    if (!line.empty()) layers.push_back(line);
  }

  int pcr = pclose(fp);
  if (pcr == -1)
    perror("pclose");
  else if (WIFEXITED(pcr) && WEXITSTATUS(pcr) != 0)
    std::cerr << "manifest parser exited with status " << WEXITSTATUS(pcr) << "\n";
  else if (WIFSIGNALED(pcr))
    std::cerr << "manifest parser killed by signal " << WTERMSIG(pcr) << "\n";
  return layers;
}

int fetch_image(const std::string &image_ref, struct container_state &s)
{
  if (exists(image_ref.c_str())) {
    s.tar_path = image_ref;
    s.created_temp_tar = false;
    return 0;
  }

  char tar_template[] = "/tmp/imageXXXXXX.tar";
  int fd = mkstemp(tar_template);
  if (fd < 0) return -1;
  close(fd);
  s.tar_path = tar_template;

  std::string skopeo_cmd = "skopeo copy docker://" + image_ref + " docker-archive:" + s.tar_path;
  int rc = run(skopeo_cmd);
  if (rc != 0) {
    unlink(s.tar_path.c_str());
    return -2;
  }
  s.created_temp_tar = true;
  return 0;
}

int assemble_rootfs(struct container_state &s)
{
  s.meta_dir = s.temp_dir + "/meta";
  s.rootfs_dir = s.temp_dir + "/rootfs";
  if (mkdir(s.meta_dir.c_str(), DIR_MODE) != 0) return -1;
  if (mkdir(s.rootfs_dir.c_str(), DIR_MODE) != 0) return -1;
  if (extract(s.tar_path, s.meta_dir) != 0) return -2;
  auto layers = parse_layers(s.meta_dir);
  if (layers.empty()) return -3;
  for (const auto &layer : layers) {
    std::string layer_path = s.meta_dir + "/" + layer;
    if (!exists(layer_path.c_str())) continue;
    std::string cmd = "tar -xf '" + layer_path + "' -C '" + s.rootfs_dir + "'";
    int rc = run(cmd);
    (void)rc; // continue even if extraction of a layer fails
  }
  return 0;
}

int try_overlay(struct container_state &s)
{
  s.upper_dir = s.temp_dir + "/upper";
  s.work_dir = s.temp_dir + "/work";
  s.merged_dir = s.temp_dir + "/merged";
  mkdir(s.upper_dir.c_str(), DIR_MODE);
  mkdir(s.work_dir.c_str(), DIR_MODE);
  mkdir(s.merged_dir.c_str(), DIR_MODE);

#ifdef __linux__
  std::string options = "lowerdir=" + s.rootfs_dir + ",upperdir=" + s.upper_dir + ",workdir=" + s.work_dir;
  if (mount("overlay", s.merged_dir.c_str(), "overlay", 0, options.c_str()) == 0) {
    s.effective_root = s.merged_dir;
    s.used_overlay = true;
    return 0;
  }
#endif
  s.effective_root = s.rootfs_dir;
  s.used_overlay = false;
  return -1;
}

int prepare_namespaces()
{
#ifdef __linux__
  if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) return -1;
  int un_flags = CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWIPC;
  if (unshare(un_flags) != 0) return -2;
  return 0;
#else
  (void)0; // not supported on non-Linux hosts
  return -1;
#endif
}

static int create_temp_dir(struct container_state &s)
{
  char temp_dir_template[] = "/tmp/containerXXXXXX";
  if (!mkdtemp(temp_dir_template)) return -1;
  s.temp_dir = temp_dir_template;
  return 0;
}

static int setup_rootfs(struct container_state &s, const std::string &image_path)
{
  if (fetch_image(image_path, s) != 0) return -1;
  if (assemble_rootfs(s) != 0) {
    if (s.created_temp_tar) unlink(s.tar_path.c_str());
    return -2;
  }
  try_overlay(s); // best-effort
  return 0;
}

static int prepare_mounts_dirs(struct container_state &s)
{
  std::string dev_dir = s.effective_root + "/dev";
  std::string proc_dir = s.effective_root + "/proc";
  std::string sys_dir = s.effective_root + "/sys";
  if (mkdir(dev_dir.c_str(), DIR_MODE) != 0 && errno != EEXIST) return -1;
  if (mkdir(proc_dir.c_str(), DIR_MODE) != 0 && errno != EEXIST) return -1;
  if (mkdir(sys_dir.c_str(), DIR_MODE) != 0 && errno != EEXIST) return -1;
  return 0;
}

[[noreturn]] static void child_run(struct container_state &s, const std::string &command)
{
  std::string old_root = s.effective_root + "/.old_root";
  if (mkdir(old_root.c_str(), DIR_MODE) != 0 && errno != EEXIST)
    std::cerr << "mkdir(.old_root) failed: " << strerror(errno) << "\n";

  if (chdir(s.effective_root.c_str()) != 0)
    std::cerr << "chdir to new root failed: " << strerror(errno) << "\n";

#ifdef __linux__
  if (syscall(SYS_pivot_root, ".", ".old_root") != 0) {
    std::cerr << "pivot_root failed: " << strerror(errno) << "; falling back to chroot\n";
    if (chroot(s.rootfs_dir.c_str()) != 0) {
      std::cerr << "chroot failed: " << strerror(errno) << "\n";
      _exit(1);
    }
    if (chdir("/") != 0) {
      std::cerr << "chdir after chroot failed: " << strerror(errno) << "\n";
      _exit(1);
    }
  } else {
    if (chdir("/") != 0) {
      std::cerr << "chdir after pivot_root failed: " << strerror(errno) << "\n";
    }
    if (umount2("/.old_root", MNT_DETACH) != 0) {
      std::cerr << "umount old_root failed: " << strerror(errno) << "\n";
    }
    rmdir("/.old_root");
  }

  if (mount("proc", "/proc", "proc", 0, NULL) != 0)
    std::cerr << "mount /proc failed: " << strerror(errno) << "\n";
  if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0)
    std::cerr << "mount /sys failed: " << strerror(errno) << "\n";
  if (mount("/dev", "/dev", NULL, MS_BIND | MS_REC, NULL) != 0)
    std::cerr << "bind mount /dev failed: " << strerror(errno) << "\n";
#else
  // Non-Linux platforms cannot pivot_root/mount namespaces so continue with
  // best-effort fallbacks (chdir only)
  if (chdir(s.effective_root.c_str()) != 0)
    std::cerr << "chdir to new root failed: " << strerror(errno) << "\n";
#endif

  // Basic PID 1 forwarder
  pid_t wpid = fork();
  if (wpid < 0) {
    perror("fork for workload failed");
    _exit(ERR_FORK);
  }

  if (wpid == 0) {
    execl("/bin/sh", "sh", "-c", command.c_str(), (char *)NULL);
    perror("execl failed");
    _exit(ERR_CHILD);
  }

  // Forward termination signals and reap
  static pid_t static_workload_pid = 0;
  static_workload_pid = wpid;

  auto make_forward = [](int signo) {
    struct sigaction sa;
    sa.sa_handler = [](int s){ if (static_workload_pid > 0) kill(static_workload_pid, s); };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(signo, &sa, NULL);
  };

  make_forward(SIGTERM);
  make_forward(SIGINT);
  make_forward(SIGHUP);
  make_forward(SIGQUIT);

  int status = 0;
  pid_t w;
  while ((w = waitpid(-1, &status, 0)) != -1) {
    if (w == static_workload_pid) {
      if (WIFEXITED(status)) _exit(WEXITSTATUS(status));
      if (WIFSIGNALED(status)) {
        int s = WTERMSIG(status);
        signal(s, SIG_DFL);
        kill(getpid(), s);
        _exit(128 + s);
      }
      _exit(0);
    }
    // just continue
  }
  _exit(0);
}

static int parent_wait_and_cleanup(struct container_state &s, pid_t child_pid)
{
  std::string dev_dir = s.effective_root + "/dev";
  std::string proc_dir = s.effective_root + "/proc";
  std::string sys_dir = s.effective_root + "/sys";

  int status = 0;
  waitpid(child_pid, &status, 0);

  std::string umount_proc = "umount -l '" + proc_dir + "'";
  std::string umount_sys = "umount -l '" + sys_dir + "'";
  std::string umount_dev = "umount -l '" + dev_dir + "'";
  system(umount_proc.c_str());
  system(umount_sys.c_str());
  system(umount_dev.c_str());

  if (s.used_overlay) {
    std::string umount_merged = "umount -l '" + s.merged_dir + "'";
    system(umount_merged.c_str());
    std::string rmdir_cmd = "rm -rf '" + s.upper_dir + "' '" + s.work_dir + "' '" + s.merged_dir + "'";
    system(rmdir_cmd.c_str());
  }

  std::string cleanup = "rm -rf '" + s.temp_dir + "'";
  system(cleanup.c_str());
  if (s.created_temp_tar) unlink(s.tar_path.c_str());

  return WIFEXITED(status) ? WEXITSTATUS(status) : -4;
}

extern "C" int run_container(const char *image_path_c, const char *command_c)
{
#ifdef __linux__
  std::string image_path = image_path_c ? image_path_c : "";
  std::string command = command_c ? command_c : "";

  struct container_state s;

  if (create_temp_dir(s) != 0) {
    perror("mkdtemp failed");
    return ERR_MKDTEMP;
  }

  if (setup_rootfs(s, image_path) != 0) {
    return ERR_EXTRACT_META;
  }

  if (prepare_namespaces() != 0) {
    std::cerr << "namespace preparation failed\n";
  }

  if (prepare_mounts_dirs(s) != 0) {
    std::cerr << "failed to create mount dirs\n";
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork failed");
    std::string cleanup = "rm -rf '" + std::string(s.temp_dir) + "'";
    system(cleanup.c_str());
    if (s.created_temp_tar) unlink(s.tar_path.c_str());
    return ERR_FORK;
  }

  if (pid == 0)
    child_run(s, command); // does not return

  return parent_wait_and_cleanup(s, pid);
}
#else
  (void)image_path_c; (void)command_c;
  fprintf(stderr, "fulmen backend native runtime is only supported on Linux hosts\n");
  return ERR_NOT_SUPPORTED;
#endif
}
