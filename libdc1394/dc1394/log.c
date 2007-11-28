#include "log.h"
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

void dc1394_register_errorlog_handler(void(*errorlog_handler)(const char *message)) {
  system_errorlog_handler = errorlog_handler;
  return;
}

void dc1394_register_warninglog_handler(void(*warninglog_handler)(const char *message)) {
  system_warninglog_handler = warninglog_handler;
  return;
}

void dc1394_register_debuglog_handler(void(*debuglog_handler)(const char *message)) {
  system_debuglog_handler = debuglog_handler;
  return;
}

void dc1394_logerror(const char *message) {
  if (system_errorlog_handler != NULL) {
	system_errorlog_handler(message);
  }
  return;
}

void dc1394_logwarning(const char *message) {
  if (system_warninglog_handler != NULL) {
	system_warninglog_handler(message);
  }
  return;
}

void dc1394_logdebug(const char *message) {
  if (system_debuglog_handler != NULL) {
	system_debuglog_handler(message);
  }
  return;
}

void dc1394_activate_debuglogging(void) {
  system_debuglog_handler = default_debuglog_handler;
  return;
}

void dc1394_reset_errorlog_handler(void) {
  system_errorlog_handler = default_errorlog_handler;
  return;
}

void dc1394_reset_warninglog_handler(void) {
  system_warninglog_handler = default_warninglog_handler;
  return;
}

void dc1394_reset_debuglog_handler(void) {
  system_debuglog_handler = NULL;
  return;
}
