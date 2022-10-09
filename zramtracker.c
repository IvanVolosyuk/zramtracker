#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ZRAM(a) "/sys/block/zram0/" a
#define MB(v) (double) (v / (1024. * 1024.))

char buf[4096];

void fail_with_perror() {
  perror("Error:");
  exit(1);
}

void print_timestamp() {
  time_t t = time(NULL);
  char *s = ctime(&t);
  s[strlen(s)-1] = '\0';
  fprintf(stderr, "%s: ", s);
}

void readfile(const char* name) {
  int fd = open(name, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Failed to read %s\n", name);
    fail_with_perror();
  }

  int res = read(fd, buf, 4095);
  if (res <= 0) {
    fprintf(stderr, "Failed to read %s\n", name);
    fail_with_perror();
  }
  buf[res] = 0;
  close(fd);
}

void writefile(const char* name, const char* content) {
  print_timestamp();
  printf("Writting to %s ", name);
  int fd = open(name, O_WRONLY | O_TRUNC | O_APPEND, 0777);
  if (fd == -1) {
    print_timestamp();
    fprintf(stderr, "Failed to read %s\n", name);
    fail_with_perror();
  }

  printf("content: [%s]\n", content);
  int res = write(fd, content, strlen(content));
  if (res < strlen(content)) {
    print_timestamp();
    fprintf(stderr, "Failed to write command: %s -> %s\n", content, name);
    perror("Error");
  }
  close(fd);
}

size_t readnum(const char* filename) {
  readfile(filename);
  return atol(buf);
}

/*
 ================ =============================================================
 orig_data_size   uncompressed size of data stored in this disk.
                  Unit: bytes
 compr_data_size  compressed size of data stored in this disk
 mem_used_total   the amount of memory allocated for this disk. This
                  includes allocator fragmentation and metadata overhead,
                  allocated for this disk. So, allocator space efficiency
                  can be calculated using compr_data_size and this statistic.
                  Unit: bytes
 mem_limit        the maximum amount of memory ZRAM can use to store
                  the compressed data
 mem_used_max     the maximum amount of memory zram has consumed to
                  store the data
 same_pages       the number of same element filled pages written to this disk.
                  No memory is allocated for such pages.
 pages_compacted  the number of pages freed during compaction
 huge_pages       the number of incompressible pages
 ================ =============================================================
*/

struct Stats {
  size_t orig_data_size;
  size_t compr_data_size;
  size_t mem_used_total;
  size_t mem_limit;
  size_t mem_used_max;
  size_t same_pages;
  size_t pages_compacted;
  size_t huge_pages;
};

Stats readstats() {
  readfile(ZRAM("mm_stat"));
  bool was_digit = false;
  Stats stats;
  int offset = 0;

  for (int i = 0; buf[i]; i++) {
    bool is_digit = buf[i] >= '0' && buf[i] <= '9';
    if (is_digit && !was_digit) {
      ((size_t*)&stats)[offset++] = atol(buf + i);
    }
    was_digit = is_digit;
  }
  return stats;
}


void control_loop() {
  while (true) {
    Stats stats = readstats();
    /*printf("Orig: %0.1f MB\n", MB(stats.orig_data_size));*/
    /*printf("Compr: %0.1f MB\n", MB(stats.compr_data_size));*/
    /*printf("Mem used total: %0.1f MB\n", MB(stats.mem_used_total));*/
    /*printf("Mem limit: %0.1f MB\n", MB(stats.mem_limit));*/
    /*printf("Mem used max: %0.1f MB\n", MB(stats.mem_used_max));*/
    /*printf("Same pages: %ld\n", stats.same_pages);*/
    /*printf("Compacted pages: %ld\n", stats.pages_compacted);*/
    /*printf("Huge pages: %ld\n", stats.huge_pages);*/
    /*printf("\n");*/
    if (stats.mem_used_total - stats.compr_data_size > (1L << 30)) {
      print_timestamp();
      fprintf(stderr, "Request compaction...\n");
      writefile(ZRAM("compact"), "1");
    }
    if (stats.compr_data_size * 4 > stats.mem_limit) {
      print_timestamp();
      fprintf(stderr, "Request writeback 1h old pages...\n");
      writefile(ZRAM("writeback"), "idle");
      sleep(60);
      writefile(ZRAM("idle"), "all");
    }
    /*printf("\n");*/
    sleep(5);
  }
}

int main(int arch, char** argv) {
  print_timestamp();
  fprintf(stderr, "Started...\n");

  mlockall(MCL_CURRENT|MCL_FUTURE |MCL_ONFAULT);
  // Kernel 5.16 supports writting only old pages, 5.10 doesn't
  writefile(ZRAM("idle"), "all");
  control_loop();
}
