#include <unistd.h>
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
int sequential = 0;

int read_file_old(char *file_name) {
  static char buffer[BUFFER_SIZE];
  struct stat stat_buf;
  int fd;
  int bytes;
  int total_read = 0;
  size_t buf_size, size;

  fd = open(file_name, 0);
  if (fd <= 0)
    return 0;

  fstat(fd, &stat_buf);
  size = stat_buf.st_size;
  if (size == 0)
    return 0;

  if (size > BUFFER_SIZE)
    buf_size = BUFFER_SIZE;
  else
    buf_size = size;

  while (1) {
    bytes = read(fd, buffer, buf_size);
    if (bytes > 0)
      total_read += bytes;
    if (total_read == size || bytes == 0) {
      close(fd);
      return total_read;
    }
  }
}

int read_file(char *file_name) {
  static char buffer[BUFFER_SIZE];
  FILE *fp;
  int bytes;
  int total_read = 0;
  struct stat stat_buf;
  int size, buf_size;

  fp = fopen(file_name, "r");
  if (! fp)
    return 0;

  fstat(fileno(fp), &stat_buf);
  size = stat_buf.st_size;
  if (size == 0)
    return 0;

  if (size > BUFFER_SIZE)
    buf_size = BUFFER_SIZE;
  else
    buf_size = size;

  
  while (1) {
    bytes = fread(buffer, 1, buf_size, fp);
    if (bytes > 0)
      total_read += bytes;
    if (total_read == size || bytes == 0) {
      fclose(fp);
      return total_read;
    }
  }
}

void input_directory(const char* dir_name) {
  DIR *dirp;
  struct dirent *dp;
  char file_name[MAX_FILE_NAME];
  char *all_files, *files;
  char *all_dirs, *dirs;
  struct stat stat_buf;
  int dir_size;

  //printf("%s\n", dir_name);
  if ((dirp = opendir(dir_name)) == NULL)
    return;

  if (fstat(dirfd(dirp), &stat_buf) == -1) {
    closedir(dirp);
    return;
  }

  if (sequential) {
    dir_size = stat_buf.st_size + 100;
    all_files = malloc(dir_size);
    files = all_files;
    bzero(all_files, dir_size);
    
    all_dirs = malloc(dir_size);
    dirs = all_dirs;
    bzero(all_dirs, dir_size);

    chdir(dir_name);
  }

  while ((dp = readdir(dirp)) != NULL) {
    if (strcmp(dp->d_name, ".") &&
	strcmp(dp->d_name, "..")) {
      if (sequential)
	strcpy(file_name, dp->d_name);
      else
	snprintf(file_name, sizeof(file_name), "%s/%s", dir_name,
		 dp->d_name);

      if (lstat(file_name, &stat_buf) != -1) {
	if (S_ISDIR(stat_buf.st_mode)) {
	  if (sequential) {
	    strcpy(dirs, dp->d_name);
	    dirs += strlen(dp->d_name) + 1;
	  } else {
	    input_directory(file_name);
	  }
	} else if (S_ISREG(stat_buf.st_mode)) {
	  total_files++;
	  if (sequential) {
	    strcpy(files, dp->d_name);
	    files += strlen(dp->d_name) + 1;
	  } else {
	    total_bytes += read_file(file_name);
	  }
	}
      }
    }
  }
  closedir(dirp);

  if (sequential) {
    files = all_files;
    while (*files) {
      total_bytes += read_file(files);
      files += strlen(files) + 1;
    }

    dirs = all_dirs;
    while (*dirs) {
      snprintf(file_name, sizeof(file_name), "%s/%s", dir_name,
	       dirs);
      input_directory(file_name);
      dirs += strlen(dirs) + 1;
    }
    free(all_files);
    free(all_dirs);
  }
}

int main(int argc, char **argv) {
  struct timeval tv;
  double start_time, stop_time;
  char *dir;

  gettimeofday(&tv, NULL); 
  start_time = tv.tv_sec + (double)tv.tv_usec / 1000000;

  if (argc < 2) {
    printf("Usage: %s [-s] <directory>\n", argv[0]);
    exit(-1);
  }

  if (! strcmp(argv[1], "-s")) {
    sequential = 1;
    dir = argv[2];
  } else {
    dir = argv[1];
  }
  
  input_directory(dir);

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
