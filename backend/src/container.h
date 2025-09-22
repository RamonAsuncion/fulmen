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

API int container_start(const char *name);
API int container_stop(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* _CONTAINER_H_ */
