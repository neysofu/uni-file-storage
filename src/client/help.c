#include "help.h"
#include <stdio.h>

void
print_help(void)
{
	puts("Client for file storage server. Options:");
	puts("");
	puts("-h");
	puts("    Prints this message and exits.");
	puts("-f filename");
	puts("    Specifies the AF_UNIX socket name.");
	puts("-w dirname,[n=0]");
	puts("    Sends recursively all files in `dirname`, up to n.");
	puts("    If n = 0 or unspecified, there is no limit.");
	puts("-W file1[,file2...]");
	puts("    Sends a list of files to the server.");
	puts("-D dirname");
	puts("    Writes evicted files to `dirname`.");
	puts("-r files");
	puts("    Reads a list of files from the server.");
	puts("-R[n=0]");
	puts("    Reads a certain amount (or all, if N is unspecified or 0) of files");
	puts("    from the server.");
	puts("-d dirname");
	puts("    Specifies the directory where to write files that have been");
	puts("    read.");
	puts("-t msec");
	puts("    Time between subsequent server requests.");
	puts("-l file1[,file2]");
	puts("    Locks some files.");
	puts("-u file1[,file2]");
	puts("    Unlocks some files.");
	puts("-c file1[,file2]");
	puts("    Removes some files from the server.");
	puts("-p");
	puts("    Enables logging to standard output.");
	puts("-Z");
	puts("    Sets the time in milliseconds between connection attempts to the");
	puts("    file storage server. 300 (msec) by default.");
}
