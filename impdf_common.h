#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32)
#include <io.h>
#define PATH_DELIM "\\"
#else
#include <unistd.h>
#include <fcntl.h>
#define PATH_DELIM "/"
#endif

#define IMPDF_DIR "impdf_dir"

/* if not exist dir, create it. */
#define IMPDF_DIR_CREATE if (access(IMPDF_DIR,0) != 0) system("mkdir " IMPDF_DIR);

#define ARG_EXIST(a) (!strcmp(a, argv[i]))
#define ARG_EXIST_PARAM(a) (ARG_EXIST(a) && (i+1 < argc))
