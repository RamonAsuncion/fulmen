#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include <string>
#include <vector>

struct container_state {
  std::string temp_dir;
  std::string meta_dir;
  std::string rootfs_dir;
  std::string upper_dir;
  std::string work_dir;
  std::string merged_dir;
  std::string effective_root;
  std::string tar_path;
  bool created_temp_tar = false;
  bool used_overlay = false;
};

int run(const std::string &cmd);
bool exists(const char *path);
int extract(const std::string &tarball, const std::string &dest);
std::vector<std::string> parse_layers(const std::string &meta_dir);
int fetch_image(const std::string &image_ref, struct container_state &s);
int assemble_rootfs(struct container_state &s);
int try_overlay(struct container_state &s);
int prepare_namespaces();


#endif /* _INTERNAL_H_ */
