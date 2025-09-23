#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/utsname.h>

#define EXIT_USAGE 2
#define EXIT_FETCH_FAIL 3
#define EXIT_LIB_NOT_FOUND 4
#define EXIT_DLOPEN 5
#define EXIT_SYMBOL 6
#define EXIT_NO_ENGINE 7
#define EXIT_IMAGE_NAME 8

#define LARGE_BUF 4096
#define CMD_BUF 8192
#define IMAGE_NAME_BUF 512
#define RECENT_CMD_BUF 256
#define TAR_SUFFIX_LEN 4

static int file_exists(const char *path)
{
  struct stat st;
  return stat(path, &st) == 0;
}

static char *fetch_with_skopeo(const char *image_ref)
{
  if (access("/usr/bin/skopeo", X_OK) != 0 &&
    access("/usr/local/bin/skopeo", X_OK) != 0) {
    fprintf(stderr, "skopeo not found on PATH; please install skopeo or pass a tarball path\n");
    return NULL;
  }

  char tmpl[] = "/tmp/fulmen_image_XXXXXX.tar";
  int fd = mkstemps(tmpl, TAR_SUFFIX_LEN); /* leave .tar suffix */
  if (fd < 0) {
    perror("mkstemps");
    return NULL;
  }
  close(fd);

  char cmd[LARGE_BUF];
  snprintf(cmd, sizeof(cmd), "skopeo copy docker://%s docker-archive:%s", image_ref, tmpl);
  int rc = system(cmd);
  if (rc != 0) {
    fprintf(stderr, "skopeo failed to fetch %s (rc=%d)\n", image_ref, rc);
    unlink(tmpl);
    return NULL;
  }

  return strdup(tmpl);
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s run <image-or-tar> [-c 'command'] [--keep-tar]\n", argv[0]);
    return EXIT_USAGE;
  }

  if (strcmp(argv[1], "run") != 0) {
    fprintf(stderr, "Only 'run' subcommand is supported in this minimal CLI.\n");
    return EXIT_USAGE;
  }

  const char *image = NULL;
  const char *command = "";
  int keep_tar = 0;
  struct utsname uts;
  int debug = 0;

  for (int i = 2; i < argc; ++i) {
    if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--command") == 0) {
      if (i + 1 < argc) {
        command = argv[++i];
      }
    } else if (strcmp(argv[i], "--keep-tar") == 0) {
      keep_tar = 1;
    } else if (!image) {
      image = argv[i];
    } else {
      /* ignore */
    }
  }

  if (getenv("FULMEN_DEBUG") != NULL) debug = 1;

  if (!image) {
    fprintf(stderr, "No image specified.\n");
    return EXIT_USAGE;
  }

  char *tar_path = NULL;
  int created_temp = 0;

  if (uname(&uts) != 0) {
    perror("uname");
    return EXIT_NO_ENGINE;
  }
  if (strcmp(uts.sysname, "Linux") != 0) {
    fprintf(stderr, "Only Linux hosts are supported.\n");
    return EXIT_NO_ENGINE;
  }

  if (file_exists(image)) {
    tar_path = strdup(image);
  } else {
    tar_path = fetch_with_skopeo(image);
    if (!tar_path)
      return EXIT_FETCH_FAIL;
    created_temp = 1;
  }

  if (debug) {
    fprintf(stderr, "[fulmen debug] image='%s' tar_path='%s' created_temp=%d\n", image, tar_path ? tar_path : "(null)", created_temp);
  }

  if (debug) {
    fprintf(stderr, "[fulmen debug] host sysname='%s'\n", uts.sysname);
  }
  /* locate backend library */
  const char *candidates[] = {
    "../backend/build/libfulmen_backend.so",
    NULL
  };

  const char *libpath = NULL;
  for (int i = 0; candidates[i]; ++i) {
    if (file_exists(candidates[i])) {
      libpath = candidates[i];
      break;
    }
  }
  if (debug) fprintf(stderr, "[fulmen debug] libpath=%s\n", libpath ? libpath : "(not found)");


  if (!libpath) {
    fprintf(stderr, "Could not find backend library in ../backend/build (expected libfulmen_backend.so)\n");
    if (created_temp && !keep_tar)
      unlink(tar_path);
    free(tar_path);
    return 4;
  }

  void *h = dlopen(libpath, RTLD_NOW | RTLD_LOCAL);
  if (!h) {
    fprintf(stderr, "dlopen failed: %s\n", dlerror());
    if (created_temp && !keep_tar)
      unlink(tar_path);
    free(tar_path);
    return 5;
  }
  if (debug) fprintf(stderr, "[fulmen debug] dlopen succeeded\n");

  typedef int (*run_fn_t)(const char *, const char *);
  run_fn_t run_container = (run_fn_t)dlsym(h, "run_container");
  if (!run_container) {
    fprintf(stderr, "Could not find symbol run_container: %s\n", dlerror());
    dlclose(h);
    if (created_temp && !keep_tar)
      unlink(tar_path);
    free(tar_path);
    return 6;
  }
  if (debug) fprintf(stderr, "[fulmen debug] found run_container symbol\n");

  int rc = run_container(tar_path, command);
  if (debug) fprintf(stderr, "[fulmen debug] run_container returned %d\n", rc);

  dlclose(h);
  if (created_temp && !keep_tar)
    unlink(tar_path);
  free(tar_path);
  return rc;
}
