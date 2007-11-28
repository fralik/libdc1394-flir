/*
 * logging facility for libdc1394
 *
 * This module provides the logging for the libdc1394 library
 * It allows registering of custom logging functions or the use
 * of the builtin loggers which redirect output to stderr.
 * Three log levels are supported:
 * error:   Indicates that an error has been detected which mandates
 *          shutdown of the program as soon as feasible
 * warning: Indicates that something happened which prevents libdc1394
 *          from working but which could possibly be resolved by the
 *          application or the user: plugging in a camera, resetting the
 *          firewire bus, ....
 * debug:   A sort of way point for the library. This log level is supposed
 *          to report that a specific function has been entered or has 
 *          passed a certain stage. This log level is turned off by default
 *          and may produce a lot of output during regular operation.
 *          The main purpose for this log level is for debugging libdc1394
 *          and for generating meaningful problem reports.
 */

/**
 * dc1394_register_errorlog_handler: register log handler for reporting fatal errors
 * This function allows registering of an error reporter for fatal errors.
 * Passing NULL as argument turns off this log level.
 * @param [in] errorlog_handler: pointer to a function which takes a character string as argument
 */
void dc1394_register_errorlog_handler(void(*errorlog_handler)(const char *message));

/**
 * dc1394_register_warninglog_handler: register log handler for reporting nonfatal errors
 * This function allows registering of an error reporter for nonfatal errors.
 * Passing NULL as argument turns off this log level.
 * @param [in] warninglog_handler: pointer to a function which takes a character string as argument
 */
void dc1394_register_warninglog_handler(void(*warninglog_handler)(const char *message));

/**
 * dc1394_register_debuglog_handler: register log handler for reporting debug statements
 * This function allows registering of an error reporter for debug statements.
 * Passing NULL as argument turns off this log level.
 * @param [in] debuglog_handler: pointer to a function which takes a character string as argument
 */
void dc1394_register_debuglog_handler(void(*debuglog_handler)(const char *message));

/**
 * dc1394_reset_errorlog_handler: set the log handler for fatal errors to the default handler
 */
void dc1394_reset_errorlog_handler(void);

/**
 * dc1394_reset_warninglog_handler: set the log handler for nonfatal errors to the default handler
 */
void dc1394_reset_warninglog_handler(void);

/**
 * dc1394_reset_debuglog_handler: set the log handler for debug statements to the default handler
 */
void dc1394_reset_debuglog_handler(void);

/**
 * dc1394_activate_debuglogging: turn on logging of debug statements
 * Per default, logging of debug statements is deactivated. This function
 * sets the log handler for debug statements to a function which outputs
 * the debug statements to stderr. In order to turn debug logging off
 * again, one may call dc1394_reset_debuglog_handler()
 */
void dc1394_activate_debuglogging(void);

/**
 * dc1394_logerror: logs a fatal error condition to the registered facility
 * This function shall be invoked if a fatal error condition is encountered.
 * The message passed as argument is delivered to the registered error reporting
 * function registered before.
 * @param [in] message: error message to be logged
 */
void dc1394_logerror(const char *message);

/**
 * dc1394_logwarning: logs a nonfatal error condition to the registered facility
 * This function shall be invoked if a nonfatal error condition is encountered.
 * The message passed as argument is delivered to the registered warning reporting
 * function registered before.
 * @param [in] message: error message to be logged
 */
void dc1394_logwarning(const char *message);

/**
 * dc1394_logdebug: logs a debug statement to the registered facility
 * This function shall be invoked if a debug statement is to be logged.
 * The message passed as argument is delivered to the registered debug reporting
 * function registered before.
 * @param [in] message: debug statement to be logged
 */
void dc1394_logdebug(const char *message);
