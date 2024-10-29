/* clog: Extremely simple logger for C.
 *
 * Features:
 * - Implemented purely as a single header file.
 * - Five log levels (none, debug, info, warn, error).
 * - Custom formats.
 * - Fast.
 *
 * Dependencies:
 * - Should conform to C89, C++98 (but requires vsnprintf, unfortunately).
 * - POSIX environment.
 *
 * USAGE:
 *
 * Include this header in any file that wishes to write to logger. In
 * exactly one file (per executable), define CLOG_MAIN first (e.g. in your
 * main .c file).
 *
 *     #define CLOG_MAIN
 *     #include "clog.h"
 *
 * This will define the actual objects that all the other units will use.
 *
 * In every file you want to use logger add define a logger module with the
 * following statement:
 *
 *     #include "clog.h"
 *     clog_create_module(MY_MODULE, LEVEL_DEBUG);
 *
 * You can define a unique logger for each file, by setting its name and message level:
 *     clog_create_module(MY_MODULE, LEVEL_INFO);
 *
 * By default logger uses standard console output using printf() function.
 * You can override this by setting a custom function for console output by
 * calling in main():
 *    int clog_init_console(int (*fun)(const char *format, ...))
 *
 * Logger can also provide logging to a file.
 * Define CLOG_FILE_SUPPORT to enable this feature and provide file path by calling:
 *    int clog_init_file(const char* const path)
 * In this case log file should be closed by:
 *  void clog_close_file(void)
 *
 * #define CLOG_DISABLED added before #include "clog.h" or globally will exclude logger's macros from compillation
 * #define CLOG_TIME_SUPPORT added before #include "clog.h"  or globally will turn on timestamp support
 * #define CLOG_FILE_SUPPORT added before #include "clog.h"  or globally will turn on log file support
 * 
 * Examples:
 *
 *
 *  #define CLOG_MAIN
 *  #include "clog.h"
 *
 *  clog_create_module(MY_MAIN, LEVEL_DEBUG);
 *
 *  int main() {
 *
 *    LOG_D(MY_MAIN, "This is %s message", "debugging");
 *    LOG_I(MY_MAIN, "This is %s message", "information");
 *    LOG_W(MY_MAIN, "This is %s message", "warning");
 *    LOG_E(MY_MAIN, "This is %s message", "error");
 *
 *    return 0;
 *  }
 *
 *
 *  #define CLOG_MAIN
 *  #define CLOG_FILE_SUPPORT
 *  #include "clog.h"
 *
 *  clog_create_module(MY_MAIN, LEVEL_DEBUG);
 *
 *  int main() {
 *
 *    clog_init_file("logs/log.txt");
 *    LOG_I(MY_MAIN, "Logging to a file");
 *    clog_close();
 *    return 0;
 *  }
 *
 *
 *  #define CLOG_MAIN
 *  #include "clog.h"
 *
 *  clog_create_module(MY_MAIN, LEVEL_DEBUG);
 *
 *  int con_output(const char *format, ...)
 *  {
 *    int ret = 0;
 *    va_list ap;
 *
 *    va_start(ap, format);
 *    ret = serial_print(format, ap);
 *    va_end(ap);
 *    return ret;
 *  }
 *
 *  int main() {
 *
 *    clog_init_console(con_output);
 *    LOG_I(MY_MAIN, "Using custom console output");

 *    return 0;
 *  }
 *
 *
 *  #include "clog.h"
 *
 *  clog_create_module(MY_MODULE, LEVEL_DEBUG);
 *
 *  int my_function() {
 *
 *    LOG_I(MY_MODULE, "My function has been called");
 *
 *    return 0;
 *  }
 *
 *
 * Errors encountered by clog will be printed to stderr.  You can suppress
 * these by defining a macro called CLOG_SILENT before including clog.h.
 *
 *
 * This implementation is based on Mike Mueller's work:
 * Copyright 2013 Mike Mueller <mike@subfocal.net>.
 * Refer to original clog here:
 *   https://github.com/mmueller/clog
 * 
 * Copyright 2024 Tomasz Kamaszewski <tomasz.kama@gmail.com>.
 * License: Do whatever you want. It would be nice if you contribute
 * improvements as pull requests here:
 *
 *   https://github.com/tomekk88/clog
 * 
 *
 * As is; no warranty is provided; use at your own risk.
 */

#ifndef __CLOG_H__
#define __CLOG_H__

#ifndef CLOG_DISABLED

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Format strings cannot be longer than this. */
#define CLOG_FORMAT_LENGTH 256

/* Formatted times and dates should be less than this length. If they are not,
 * they will not appear in the log. */
#define CLOG_DATETIME_LENGTH 256

/* Default format strings.
  %f: Source file name generating the log call.
  %n : Source line number where the log call was made.
  %m : The message text sent to the logger(after printf formatting).
  %d : The current date, formatted using the logger's date format.
  %e : Module name.
  %g : Function name where the log call was made.
  %h : The current tick value.
  %t : The current time, formatted using the logger's time format.
  %l : The log level(one of "DEBUG", "INFO", "WARN", or "ERROR").
  %%: A literal percent sign.
*/

#ifdef CLOG_TIME_SUPPORT
#define CLOG_DEFAULT_FORMAT "%d %t.%h %l: %e: %f(%n): %g: %m\n"
#else /* CLOG_TIME_SUPPORT */
#define CLOG_DEFAULT_FORMAT "%l: %e: %f(%n): %g: %m\n"
#endif /* CLOG_TIME_SUPPORT */

#define CLOG_DEFAULT_DATE_FORMAT "%Y-%m-%d"
#define CLOG_DEFAULT_TIME_FORMAT "%H:%M:%S"

/* Default console output funciton */
#define CLOG_DEFAULT_MESSAGE_FUNCTION printf

#ifdef __cplusplus
extern "C" {
#endif

  enum clog_level {
    LEVEL_DEBUG,
    LEVEL_INFO,
    LEVEL_WARN,
    LEVEL_ERROR,
    LEVEL_NONE
  };

  typedef struct
  {
    const char* module_name;
    enum clog_level module_level;
  } clog_control_block_t;

  struct clog;

  /**
   * Create a new logger file for writing to the given path.  The file will always
   * be opened in append mode.
   *
   *
   * @param path
   * Path to the file where log messages will be written.
   *
   * @return
   * Zero on success, non-zero on failure.
   */
  int clog_init_file(const char* const path);

  /**
   * Closes the logger file.  You should do this at the end of execution,
   * or when you are done using the logger.
   *
   */
  void clog_close_file(void);

  /**
   * Set a custom console output function.
   *
   *
   * @param fun
   * Function pointer for printing the message.
   *
   * @return
   * Zero on success, non-zero on failure.
   */
  int clog_init_console(int (*fun)(const char* format, ...));

  /*
   * No need to read below this point.
   */

  /*
   * Portability stuff.
   */

  /* This is not portable, but should work on old Visual C++ compilers. Visual
   * Studio 2013 defines va_copy, but older versions do not. */
#ifdef _MSC_VER
#if _MSC_VER < 1800
#define va_copy(a,b) ((a) = (b))
#endif
#endif

#ifdef _WIN32
#define POSIX_OPEN  _open
#define POSIX_CLOSE _close
#define POSIX_WRITE _write
#else
#define POSIX_OPEN  open
#define POSIX_CLOSE close
#define POSIX_WRITE write
#endif

  /**
  * The C logger structure.
  */
  struct clog {
#ifdef CLOG_FILE_SUPPORT
    /* The file being written. */
    int fd;
#endif /* CLOG_FILE_SUPPORT */
    /* Points to a function writing a message. */
    int (*fun)(const char* format, ...);

    /* The format specifier. */
    char fmt[CLOG_FORMAT_LENGTH];

    /* Date format */
    char date_fmt[CLOG_FORMAT_LENGTH];

    /* Time format */
    char time_fmt[CLOG_FORMAT_LENGTH];
  };

  void _clog_err(const char* fmt, ...);

#ifdef CLOG_MAIN
  struct clog _clog_logger = {
#ifdef CLOG_FILE_SUPPORT
    0,
#endif /* CLOG_FILE_SUPPORT */
    CLOG_DEFAULT_MESSAGE_FUNCTION,
    CLOG_DEFAULT_FORMAT,
    CLOG_DEFAULT_DATE_FORMAT,
    CLOG_DEFAULT_TIME_FORMAT
  };
#else
  extern struct clog _clog_logger;
#endif

#ifdef CLOG_MAIN

  const char* const CLOG_LEVEL_NAMES[] = {
      "D",
      "I",
      "W",
      "E",
  };

  int clog_init_file(const char* const path)
  {
#ifdef CLOG_FILE_SUPPORT
    int fd = POSIX_OPEN(path, O_CREAT | O_WRONLY | O_APPEND, 0666);
    if (fd == -1) {
      _clog_err("Unable to open %s: %s\n", path, strerror(errno));
      return 1;
    }

    _clog_logger.fd = fd;
    _clog_logger.fun = NULL;
    return 1;
#else
    _clog_logger.fun = NULL;
#endif
    return 0;
  }


  int clog_init_console(int (*fun)(const char* format, ...))
  {
    if (fun == NULL)
    {
      _clog_err("Invalid pointer\n");
      return 1;
    }
#ifdef CLOG_FILE_SUPPORT
    _clog_logger.fd = 0;
#endif /* CLOG_FILE_SUPPORT */
    _clog_logger.fun = fun;
    return 0;
  }

  void clog_close_file(void)
  {
#ifdef CLOG_FILE_SUPPORT
    if (_clog_logger.fd)
    {
      POSIX_CLOSE(_clog_logger.fd);
    }
#endif
  }

  /* Internal functions */

  size_t _clog_append_str(char** dst, char* orig_buf, const char* src, size_t cur_size)
  {
    size_t new_size = cur_size;

    while (strlen(*dst) + strlen(src) >= new_size) {
      new_size *= 2;
    }
    if (new_size != cur_size) {
      if (*dst == orig_buf) {
        *dst = (char*)malloc(new_size);
        strcpy(*dst, orig_buf);
      }
      else {
        *dst = (char*)realloc(*dst, new_size);
      }
    }

    strcat(*dst, src);
    return new_size;
  }

  size_t _clog_append_int(char** dst, char* orig_buf, long int d, size_t cur_size)
  {
    char buf[40]; /* Enough for 128-bit decimal */
    if (snprintf(buf, 40, "%ld", d) >= 40) {
      return cur_size;
    }
    return _clog_append_str(dst, orig_buf, buf, cur_size);
  }

#ifdef CLOG_TIME_SUPPORT
  size_t _clog_append_time(char** dst, char* orig_buf, struct tm* lt,
      const char* fmt, size_t cur_size)
  {
    char buf[CLOG_DATETIME_LENGTH];
    size_t result = strftime(buf, CLOG_DATETIME_LENGTH, fmt, lt);

    if (result > 0) {
      return _clog_append_str(dst, orig_buf, buf, cur_size);
    }

    return cur_size;
  }
#endif /* CLOG_TIME_SUPPORT */


  const char* _clog_basename(const char* path)
  {
    const char* slash = strrchr(path, '/');
    if (slash) {
      path = slash + 1;
    }
#ifdef _WIN32
    slash = strrchr(path, '\\');
    if (slash) {
      path = slash + 1;
    }
#endif
    return path;
  }

  char*  _clog_format(const struct clog* logger, char buf[], size_t buf_size,
      const char* sfile, int sline, const char* sfunction,
      const char* smodule, const char* level, const char* message)
  {
    size_t cur_size = buf_size;
    char* result = buf;
    enum { NORMAL, SUBST } state = NORMAL;
    size_t fmtlen = strlen(logger->fmt);
    size_t i;
#ifdef CLOG_TIME_SUPPORT
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);
    clock_t ticks = clock();
#endif /* CLOG_TIME_SUPPORT */

    sfile = _clog_basename(sfile);
    result[0] = 0;
    for (i = 0; i < fmtlen; ++i) {
      if (state == NORMAL) {
        if (logger->fmt[i] == '%') {
          state = SUBST;
        }
        else {
          char str[2] = { 0 };
          str[0] = logger->fmt[i];
          cur_size = _clog_append_str(&result, buf, str, cur_size);
        }
      }
      else {
        switch (logger->fmt[i]) {
        case '%':
          cur_size = _clog_append_str(&result, buf, "%", cur_size);
          break;
#ifdef CLOG_TIME_SUPPORT
        case 't':
          cur_size = _clog_append_time(&result, buf, lt,
            logger->time_fmt, cur_size);
          break;
        case 'd':
          cur_size = _clog_append_time(&result, buf, lt,
            logger->date_fmt, cur_size);
          break;
        case 'h':
          cur_size = _clog_append_int(&result, buf, ticks, cur_size);
          break;
#endif /* CLOG_TIME_SUPPORT */
        case 'l':
          cur_size = _clog_append_str(&result, buf, level, cur_size);
          break;
        case 'e':
          cur_size = _clog_append_str(&result, buf, smodule, cur_size);
          break;
        case 'g':
          cur_size = _clog_append_str(&result, buf, sfunction, cur_size);
          break;
        case 'n':
          cur_size = _clog_append_int(&result, buf, sline, cur_size);
          break;
        case 'f':
          cur_size = _clog_append_str(&result, buf, sfile, cur_size);
          break;
        case 'm':
          cur_size = _clog_append_str(&result, buf, message,
            cur_size);
          break;
        }
        state = NORMAL;
      }
    }

    return result;
  }

  void  _clog_log(const char* sfile, int sline, const char* sfunction, const char* smodule,
      enum clog_level level, const char* fmt, ...)
  {
    /* For speed: Use a stack buffer until message exceeds 4096, then switch
     * to dynamically allocated.  This should greatly reduce the number of
     * memory allocations (and subsequent fragmentation). */
    char buf[4096];
    size_t buf_size = 4096;
    char* dynbuf = buf;
    char* message;
    va_list ap;
    int result;
    struct clog* logger = &_clog_logger;

    /* Format the message text with the argument list. */
    va_start(ap, fmt);
    result = vsnprintf(dynbuf, buf_size, fmt, ap);
    if ((size_t)result >= buf_size) {
      buf_size = result + 1;
      dynbuf = (char*)malloc(buf_size);
      result = vsnprintf(dynbuf, buf_size, fmt, ap);
      if ((size_t)result >= buf_size) {
        /* Formatting failed -- too large */
        _clog_err("Formatting failed (1).\n");
        va_end(ap);
        free(dynbuf);
        return;
      }
    }
    va_end(ap);

    /* Format according to log format and write to log */
    {
      char message_buf[4096];
      message = _clog_format(logger, message_buf, 4096, sfile, sline, sfunction,
        smodule, CLOG_LEVEL_NAMES[level], dynbuf);
      if (!message) {
        _clog_err("Formatting failed (2).\n");
        if (dynbuf != buf) {
          free(dynbuf);
        }
        return;
      }

#ifdef CLOG_FILE_SUPPORT
      if (logger->fd)
      {
        result = POSIX_WRITE(logger->fd, message, strlen(message));
      }
      else
#endif        
      if (logger->fun)
      {
        result = logger->fun(message);
      }

      if (result == -1) {
        _clog_err("Unable to write to log file: %s\n", strerror(errno));
      }
      if (message != message_buf) {
        free(message);
      }
      if (dynbuf != buf) {
        free(dynbuf);
      }
    }
  }

  void  _clog_err(const char* fmt, ...)
  {
#ifdef CLOG_SILENT
    (void) fmt;
#else
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#endif
  }

#else /* CLOG_MAIN */
  void  _clog_log(const char* sfile, int sline, const char* sfunction, const char* smodule,
                   enum clog_level level, const char* fmt, ...);
#endif /* CLOG_MAIN */

/**
* Create a new logger module
*
* @param module
* Module name
*
* @param level
* Debugmessage level
*
*/
#define clog_create_module(module, level) \
static clog_control_block_t clog_control_block_##module = \
{ \
    #module, \
    (level), \
};

/**
* Prints debugging message for the module
*
* @param module
* Module name
*
*/
#define LOG_D(module, ...) \
do { \
    extern clog_control_block_t clog_control_block_##module; \
    (void)(clog_control_block_##module); \
    if(clog_control_block_##module.module_level <= LEVEL_DEBUG){\
    _clog_log(__FILE__, __LINE__, __FUNCTION__, clog_control_block_##module.module_name, LEVEL_DEBUG, ##__VA_ARGS__);} \
} while (0)

/**
* Prints information message for the module
*
* @param module
* Module name
*
*/
#define LOG_I(module, ...) \
do { \
    extern clog_control_block_t clog_control_block_##module; \
    (void)(clog_control_block_##module); \
    if(clog_control_block_##module.module_level <= LEVEL_INFO){\
    _clog_log(__FILE__, __LINE__, __FUNCTION__, clog_control_block_##module.module_name, LEVEL_INFO, ##__VA_ARGS__);} \
} while (0)

/**
* Prints warning message for the module
*
* @param module
* Module name
*
*/
#define LOG_W(module, ...) \
do { \
    extern clog_control_block_t clog_control_block_##module; \
    (void)(clog_control_block_##module); \
    if(clog_control_block_##module.module_level <= LEVEL_WARN){\
    _clog_log(__FILE__, __LINE__, __FUNCTION__, clog_control_block_##module.module_name, LEVEL_WARN, ##__VA_ARGS__);} \
} while (0)

/**
* Prints error message for the module
*
* @param module
* Module name
*
*/
#define LOG_E(module, ...) \
do { \
    extern clog_control_block_t clog_control_block_##module; \
    (void)(clog_control_block_##module); \
    if(clog_control_block_##module.module_level <= LEVEL_ERROR){\
    _clog_log(__FILE__, __LINE__, __FUNCTION__, clog_control_block_##module.module_name, LEVEL_ERROR, ##__VA_ARGS__);} \
} while (0)

#else

#define clog_init_file(path)
#define clog_close_file()
#define clog_init_console(fun)
#define clog_create_module(module, level)
#define LOG_D(module, ...)
#define LOG_I(module, ...)
#define LOG_W(module, ...)
#define LOG_E(module, ...)

#endif /* CLOG_DISABLED */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __CLOG_H__ */
