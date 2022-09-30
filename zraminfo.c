#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ZRAM(a) "/sys/block/zram0/" a
#define MB(v) (double) (v / (1024. * 1024.))

char buf[4096];

void fail_with_perror() {
  perror("Error:");
  exit(1);
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
  int fd = open(name, O_WRONLY);
  if (fd == -1) {
    fprintf(stderr, "Failed to read %s\n", name);
    fail_with_perror();
  }

  write(fd, content, strlen(content));
  close(fd);
}

size_t readnum(const char* filename) {
  readfile(filename);
  return atol(buf);
}

void read_ints(size_t* output) {
  bool was_digit = false;
  int offset = 0;

  for (int i = 0; buf[i]; i++) {
    bool is_digit = buf[i] >= '0' && buf[i] <= '9';
    if (is_digit && !was_digit) {
      output[offset++] = atol(buf + i);
    }
    was_digit = is_digit;
  }
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

/*
 ============== =============================================================
 bd_count       size of data written in backing device.
                Unit: 4K bytes
 bd_reads       the number of reads from backing device
                Unit: 4K bytes
 bd_writes      the number of writes to backing device
                Unit: 4K bytes
*/

struct BdStats {
  size_t bd_count;
  size_t reads;
  size_t writes;
};

Stats readstats() {
  readfile(ZRAM("mm_stat"));
  Stats stats;
  read_ints((size_t*)&stats);
  return stats;
}

BdStats readbdstats() {
  readfile(ZRAM("bd_stat"));
  BdStats stats;
  read_ints((size_t*)&stats);
  return stats;
}

void control_loop() {
  while (true) {
    Stats stats = readstats();
    printf("Orig: %0.1f MB\n", MB(stats.orig_data_size));
    printf("Compr: %0.1f MB\n", MB(stats.compr_data_size));
    printf("Mem used: %0.1f MB\n", MB(stats.mem_used_total));
    printf("Mem used max: %0.1f MB\n", MB(stats.mem_used_max));
    printf("Mem limit: %0.1f MB\n", MB(stats.mem_limit));
    printf("Same pages: %ld\n", stats.same_pages);
    printf("Compacted pages: %ld\n", stats.pages_compacted);
    printf("Huge pages: %ld\n", stats.huge_pages);

    BdStats bdstats = readbdstats();
    printf("Stored on  backing device: %0.1f MB\n", MB(bdstats.bd_count * 4096));
    printf("Written to backing device: %0.1f MB\n", MB(bdstats.writes * 4096));
    printf("Read from  backing device: %0.1f MB\n", MB(bdstats.reads * 4096));
    /*if (stats.mem_used_total - stats.compr_data_size > (1L << 30)) {*/
    /*  printf("Request compaction...\n");*/
    /*  writefile(ZRAM("compact"), "1");*/
    /*}*/
    /*if (stats.compr_data_size * 3 > stats.mem_limit) {*/
    /*  printf("Request writeback 1h old pages...\n");*/
    /*  writefile(ZRAM("idle"), "3600");*/
    /*  writefile(ZRAM("writeback"), "idle");*/
    /*}*/
    printf("\n");
    sleep(60);
  }
}

int main(int arch, char** argv) {
  /*mlockall(MCL_CURRENT|MCL_FUTURE |MCL_ONFAULT);*/
  control_loop();
}
