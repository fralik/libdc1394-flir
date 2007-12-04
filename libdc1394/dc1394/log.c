
#include "log.h"

static void default_errorlog_handler(dc1394log_t type, const char *message, const char *file, int line, const char *function, void* user) {
  fprintf(stderr, "libdc1394 error in %s (%s, line %d): %s\n", function, file, line, message);
  return;
}

static void default_warninglog_handler(dc1394log_t type, const char *message, const char *file, int line, const char *function, void* user) {
  fprintf(stderr, "libdc1394 warning in %s (%s, line %d): %s\n", function, file, line, message);
  return;
}

static void default_debuglog_handler(dc1394log_t type, const char *message, const char *file, int line, const char *function, void* user) {
#ifdef DC1394_DBG
  fprintf(stderr, "libdc1394 debug in %s (%s, line %d): %s\n", function, file, line, message);
#endif
  return;
}

void(*system_errorlog_handler)(dc1394log_t type, const char *message, const char *file, int line, const char *function, void* user) = default_errorlog_handler;
void(*system_warninglog_handler)(dc1394log_t type, const char *message, const char *file, int line, const char *function, void* user) = default_warninglog_handler;
void(*system_debuglog_handler)(dc1394log_t type, const char *message, const char *file, int line, const char *function, void* user) = NULL;

dc1394error_t
dc1394_log_register_handler(dc1394log_t type, void(*log_handler)(dc1394log_t type, const char *message, const char *file, int line, const char *function, void* user)) {
  switch (type) {
    case DC1394_LOG_ERROR:
      system_errorlog_handler = log_handler;
      return DC1394_SUCCESS;
    case DC1394_LOG_WARNING:
      system_warninglog_handler = log_handler;
      return DC1394_SUCCESS;
    case DC1394_LOG_DEBUG:
      system_debuglog_handler = log_handler;
      return DC1394_SUCCESS;
    default:
      return DC1394_INVALID_LOG_TYPE;
  }
}

dc1394error_t
dc1394_log_set_default_handler(dc1394log_t type) {
  switch (type) {
    case DC1394_LOG_ERROR:
      system_errorlog_handler = default_errorlog_handler;
      return DC1394_SUCCESS;
    case DC1394_LOG_WARNING:
      system_warninglog_handler = default_warninglog_handler;
      return DC1394_SUCCESS;
    case DC1394_LOG_DEBUG:
      system_debuglog_handler = default_debuglog_handler;
      return DC1394_SUCCESS;
    default:
      return DC1394_INVALID_LOG_TYPE;
  }
}

/*
void dc1394_log_error(const char *message) {
  if (system_errorlog_handler != NULL) {
	system_errorlog_handler(message);
  }
  return;
}

void dc1394_log_warning(const char *message) {
  if (system_warninglog_handler != NULL) {
	system_warninglog_handler(message);
  }
  return;
}

void dc1394_log_debug(const char *message) {
  if (system_debuglog_handler != NULL) {
	system_debuglog_handler(message);
  }
  return;
}
*/
