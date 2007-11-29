#include "control.h"
#include <stdio.h>
#include <stdlib.h>

static void default_errorlog_handler(const char *message) {
  fprintf(stderr, "ERROR: %s\n", message);
  return;
}

static void default_warninglog_handler(const char *message) {
  fprintf(stderr, "WARNING: %s\n", message);
  return;
}

static void default_debuglog_handler(const char *message) {
  fprintf(stderr, "DEBUG: %s\n", message);
  return;
}

static void(*system_errorlog_handler)(const char *message) = default_errorlog_handler;
static void(*system_warninglog_handler)(const char *message) = default_warninglog_handler;
static void(*system_debuglog_handler)(const char *message) = NULL;

void dc1394_log_register_handler(dc1394log_t type, void(*log_handler)(const char *message)) {
  switch (type) {
    case DC1394_LOG_ERROR:
      system_errorlog_handler = log_handler;
      return;
    case DC1394_LOG_WARNING:
      system_warninglog_handler = log_handler;
      return;
    case DC1394_LOG_DEBUG:
      system_debuglog_handler = log_handler;
      return;
  }
}

void dc1394_log_set_default_handler(dc1394log_t type) {
  switch (type) {
    case DC1394_LOG_ERROR:
      system_errorlog_handler = default_errorlog_handler;
      return;
    case DC1394_LOG_WARNING:
      system_warninglog_handler = default_warninglog_handler;
      return;
    case DC1394_LOG_DEBUG:
      system_debuglog_handler = default_debuglog_handler;
      return;
  }
}

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
