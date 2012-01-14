#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#define MAX_FILE_NAME 10000
#define BUFFER_SIZE (4096 * 100)

long total_bytes = 0, total_files = 0;

int read_file(char *file_name) {
  static char buffer[BUFFER_SIZE];
  int fd;
  int bytes;
  int total_read = 0;
  fd = open(file_name, 0);
  if (fd <= 0)
    return;
  while (1) {
    bytes = read(fd, buffer, BUFFER_SIZE);
    if (bytes > 0)
      total_read += bytes;
    if (bytes < BUFFER_SIZE) {
      close(fd);
      return total_read;
    }
  }
}

#define MAX_FILE_NAMES (200000 * 30)

void input_directory(const char* dir_name) {
  DIR *dirp;
  struct dirent *dp;
  char file_name[MAX_FILE_NAME];
  char *all_files, *files;
  struct stat stat_buf;

  //printf("%s\n", dir_name);
  if ((dirp = opendir(dir_name)) == NULL)
    return;

  if (fstat(dirfd(dirp), &stat_buf) == -1) {
    closedir(dirp);
    return;
  }

  all_files = malloc(stat_buf.st_size + 100);
  files = all_files;
  bzero(all_files, stat_buf.st_size + 100);

  while ((dp = readdir(dirp)) != NULL) {
    snprintf(file_name, sizeof(file_name), "%s/%s", dir_name,
	     dp->d_name);

    if (strcmp(dp->d_name, ".") &&
	strcmp(dp->d_name, "..")) {
      if (lstat(file_name, &stat_buf) != -1) {
	if (S_ISDIR(stat_buf.st_mode))
	  input_directory(file_name);
	else if (S_ISREG(stat_buf.st_mode)) {
	  total_files++;
	  strcpy(files, dp->d_name);
	  files += strlen(dp->d_name) + 1;
	}
      }
    }
  }
  closedir(dirp);

  files = all_files;
  while (*files) {
    chdir(dir_name);
    total_bytes += read_file(files);
    files += strlen(files) + 1;
  }
  free(all_files);
}

int main(int argc, char **argv) {
  struct timeval tv;
  double start_time, stop_time;

  gettimeofday(&tv, NULL); 
  start_time = tv.tv_sec + (double)tv.tv_usec / 1000000;

  if (argc != 2) {
    printf("Usage: %s <directory>\n", argv[0]);
    exit(-1);
  }
  
  input_directory(argv[1]);

  gettimeofday(&tv, NULL); 
  stop_time = tv.tv_sec + (double)tv.tv_usec / 1000000;
  
  printf("Elapsed %.1f, files: %ld, megabytes: %.1f\n",
	 (stop_time - start_time),
	 total_files, (double)total_bytes/(1000 * 1000));

  printf("Files per second: %.2f, megabytes per second: %.2f\n",
	 (double)total_files / (stop_time - start_time),
	 (double)total_bytes / (stop_time - start_time) / (1000 * 1000));
  exit(0);
}
