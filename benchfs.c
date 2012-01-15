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
#include <syscall.h>

#define MAX_FILE_NAME 10000
#define BUFFER_SIZE (4096 * 100)

long total_bytes = 0, total_files = 0;

int read_file(char *file_name) {
  static char buffer[BUFFER_SIZE];
  struct stat stat_buf;
  int fd;
  int bytes;
  int total_read = 0;
  size_t buf_size, size;

  fd = open(file_name, O_RDONLY);
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

void input_directory_sequential(const char* dir_name) {
  DIR *dirp;
  struct dirent *dp;
  char file_name[MAX_FILE_NAME];
  char *all_files, *files;
  char *all_dirs, *dirs;
  struct stat stat_buf;
  int dir_size;

  if ((dirp = opendir(dir_name)) == NULL)
    return;

  if (fstat(dirfd(dirp), &stat_buf) == -1) {
    closedir(dirp);
    return;
  }

  dir_size = stat_buf.st_size + 100;
  all_files = malloc(dir_size);
  files = all_files;
  bzero(all_files, dir_size);
  
  all_dirs = malloc(dir_size);
  dirs = all_dirs;
  bzero(all_dirs, dir_size);
  
  chdir(dir_name);

  while ((dp = readdir(dirp)) != NULL) {
    if (strcmp(dp->d_name, ".") &&
	strcmp(dp->d_name, "..")) {
      strcpy(file_name, dp->d_name);

      if (lstat(file_name, &stat_buf) != -1) {
	if (S_ISDIR(stat_buf.st_mode)) {
	  strcpy(dirs, dp->d_name);
	  dirs += strlen(dp->d_name) + 1;
	} else if (S_ISREG(stat_buf.st_mode)) {
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
    total_bytes += read_file(files);
    files += strlen(files) + 1;
  }
  
  dirs = all_dirs;
  while (*dirs) {
    snprintf(file_name, sizeof(file_name), "%s/%s", dir_name,
	     dirs);
    input_directory_sequential(file_name);
    dirs += strlen(dirs) + 1;
  }
  free(all_files);
  free(all_dirs);
}

int compare (const void * a, const void * b) {
  return strcmp((char*) a, (char*)b);
}

void input_directory_relative(const char* dir_name) {
  DIR *dirp;
  struct dirent *dp;
  char file_name[MAX_FILE_NAME];
  char *all_files, *files;
  char *all_dirs, *dirs;
  struct stat stat_buf;
  int dir_size;
  int num_files = 0, i = 0;
  char **file_array;

  if ((dirp = opendir(dir_name)) == NULL)
    return;

  if (fstat(dirfd(dirp), &stat_buf) == -1) {
    closedir(dirp);
    return;
  }

  dir_size = stat_buf.st_size + 100;
  all_files = malloc(dir_size);
  files = all_files;
  bzero(all_files, dir_size);
  
  all_dirs = malloc(dir_size);
  dirs = all_dirs;
  bzero(all_dirs, dir_size);
  
  while ((dp = readdir(dirp)) != NULL) {
    if (strcmp(dp->d_name, ".") &&
	strcmp(dp->d_name, "..")) {
      snprintf(file_name, sizeof(file_name), "%s/%s", dir_name, dp->d_name);
      if (lstat(file_name, &stat_buf) != -1) {
	if (S_ISDIR(stat_buf.st_mode)) {
	  strcpy(dirs, dp->d_name);
	  dirs += strlen(dp->d_name) + 1;
	} else if (S_ISREG(stat_buf.st_mode)) {
	  total_files++;
	  num_files++;
	  strcpy(files, dp->d_name);
	  files += strlen(dp->d_name) + 1;
	}
      }
    }
  }
  closedir(dirp);

  files = all_files;
  file_array = calloc(sizeof(char*), num_files);
  while (*files) {
    file_array[i++] = files;
    files += strlen(files) + 1;
  }
  qsort(file_array, num_files, sizeof(char*), compare);
  for (i = 0; i < num_files; i++) {
    snprintf(file_name, sizeof(file_name), "%s/%s", dir_name, file_array[i]);
    total_bytes += read_file(file_name);
  }
  free(file_array);
  
  dirs = all_dirs;
  while (*dirs) {
    snprintf(file_name, sizeof(file_name), "%s/%s", dir_name, dirs);
    input_directory_relative(file_name);
    dirs += strlen(dirs) + 1;
  }
  free(all_files);
  free(all_dirs);
}

void input_directory_depth_first(const char* dir_name) {
  DIR *dirp;
  struct dirent *dp;
  char file_name[MAX_FILE_NAME];
  struct stat stat_buf;

  if ((dirp = opendir(dir_name)) == NULL)
    return;

  if (fstat(dirfd(dirp), &stat_buf) == -1) {
    closedir(dirp);
    return;
  }

  while ((dp = readdir(dirp)) != NULL) {
    if (strcmp(dp->d_name, ".") &&
	strcmp(dp->d_name, "..")) {
      snprintf(file_name, sizeof(file_name), "%s/%s", dir_name,
	       dp->d_name);

      if (lstat(file_name, &stat_buf) != -1) {
	if (S_ISDIR(stat_buf.st_mode)) {
	  input_directory_depth_first(file_name);
	} else if (S_ISREG(stat_buf.st_mode)) {
	  total_files++;
	  total_bytes += read_file(file_name);
	}
      }
    }
  }
  closedir(dirp);
}

int main(int argc, char **argv) {
  struct timeval tv;
  double start_time, stop_time;

  gettimeofday(&tv, NULL); 
  start_time = tv.tv_sec + (double)tv.tv_usec / 1000000;

  if (argc < 2) {
    printf("Usage: %s [-s] <directory>\n", argv[0]);
    exit(-1);
  }

  if (! strcmp(argv[1], "-s"))
    input_directory_sequential(argv[2]);
  else if (! strcmp(argv[1], "-r")) {
    char *dir = calloc(strlen(argv[2]) + 1, 1);
    strcpy(dir, argv[2]);
    *strrchr(dir, '/') = 0;
    if (strlen(dir) == 0)
      chdir("/");
    else
      chdir(dir);
    free(dir);
    input_directory_relative(strrchr(argv[2], '/') + 1);
  } else
    input_directory_depth_first(argv[1]);

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
