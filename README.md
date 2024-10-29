# clog
Simple logger for C

Features:
- Implemented purely as a single header file.
- Five log levels (none, debug, info, warn, error).
- Custom formats.
- Fast.

# 1. Usage

Include this header in any file that wishes to write to logger. In
exactly one file (per executable), define CLOG_MAIN first (e.g. in your
main.c file).

    #define CLOG_MAIN
    #include "clog.h"

This will define the actual objects that all the other units will use.

In every file you want to use logger add define a logger module with the
following statement:

    #include "clog.h"
    clog_create_module(MY_MODULE, LEVEL_DEBUG);

You can define a unique logger for each file, by setting its name and message level:
    clog_create_module(MY_MODULE, LEVEL_INFO);

By default logger uses standard console output using printf() function.
You can override this by setting a custom function for console output by
calling in main():
   int clog_init_console(int (*fun)(const char *format, ...))

Logger can also provide logging to a file.
Define CLOG_FILE_SUPPORT to enable this feature and provide file path by calling:
   int clog_init_file(const char* const path)
In this case log file should be closed by:
 void clog_close_file(void)

#define CLOG_DISABLED added before #include "clog.h" or globally will exclude logger's macros from compillation
#define CLOG_TIME_SUPPORT added before #include "clog.h"  or globally will turn on timestamp support
#define CLOG_FILE_SUPPORT added before #include "clog.h"  or globally will turn on log file support

# 2. Examples

## Simple usage example in main()

	#define CLOG_FILE_SUPPORT
  	#define CLOG_TIME_SUPPORT
	#define CLOG_MAIN
	#include "clog.h"

	clog_create_module(MY_MAIN, LEVEL_DEBUG);
	
	int main() {
	   LOG_D(MY_MAIN, "This is %s message", "debugging");
	   LOG_I(MY_MAIN, "This is %s message", "information");
	   LOG_W(MY_MAIN, "This is %s message", "warning");
	   LOG_E(MY_MAIN, "This is %s message", "error");

	   return 0;
	 }

## Enabling log file support

	 #define CLOG_MAIN
	 #define CLOG_FILE_SUPPORT
	 #include "clog.h"

	 clog_create_module(MY_MAIN, LEVEL_DEBUG);

	 int main() {

	   clog_init_file("logs/log.txt");
	   LOG_I(MY_MAIN, "Logging to a file");
	   clog_close();
	   return 0;
	 }

## Using custom console output function

	 #define CLOG_MAIN
	 #define CLOG_TIME_SUPPORT
	 #include "clog.h"

	 clog_create_module(MY_MAIN, LEVEL_DEBUG);

	 int con_output(const char *format, ...)
	 {
	   int ret = 0;
	   va_list ap;

	   va_start(ap, format);
	   ret = serial_print(format, ap);
	   va_end(ap);
	   return ret;
	 }

	 int main() {

	   clog_init_console(con_output);
	   LOG_I(MY_MAIN, "Using custom console output");

	   return 0;
	 }

## Simple example of usage in regular files

	 #include "clog.h"

	 clog_create_module(MY_MODULE, LEVEL_DEBUG);

	 int my_function() {

	   LOG_I(MY_MODULE, "My function has been called");

	   return 0;
	 }


Errors encountered by clog will be printed to stderr.  You can suppress
these by defining a macro called CLOG_SILENT before including clog.h.

# 3. License

Do whatever you want.

As is; no warranty is provided; use at your own risk.

This implementation is based on Mike Mueller's work.

Copyright 2013 Mike Mueller <mike@subfocal.net>.
Refer to original clog here:
  https://github.com/mmueller/clog

Copyright 2024 Tomasz Kamaszewski <tomasz.kama@gmail.com>.
It would be nice if you contribute
improvements as pull requests here:
  https://github.com/tomekk88/clog
