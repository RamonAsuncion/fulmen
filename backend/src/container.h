#ifndef _CONTAINER_H_
#define _CONTAINER_H_

#ifdef _WIN32
  #define API __declspec(dllexport)
#else
  #define API __attribute__((visibility("default"))) // default visibilty already but portability
#endif

#ifdef __cplusplus
extern "C" {
#endif

API int run_container(const char* image_path, const char* command);

#ifdef __cplusplus
}
#endif

#endif /* _CONTAINER_H_ */
