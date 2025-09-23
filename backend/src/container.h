#ifndef _CONTAINER_H_
#define _CONTAINER_H_

#ifdef _WIN32
#define API __declspec(dllexport)
#else
#define API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

API int run_container(const char* image_path, const char* command);

#ifdef __cplusplus
}
#endif

#define DIR_MODE 0755
#define READ_BUF 4096

enum return_code {
  ERR_MKDTEMP = -1,
  ERR_MKSTEMP = -2,
  ERR_FETCH = -3,
  ERR_EXTRACT_META = -4,
  ERR_NO_LAYERS = -5,
  ERR_FORK = -6,
  ERR_CHILD = -7,
  ERR_NOT_SUPPORTED = -8
};

#endif
