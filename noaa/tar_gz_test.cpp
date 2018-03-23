/*
 * This file is in the public domain.
 *
 * Feel free to use it as you wish.
 */

/*
 * This example program reads an archive from stdin (which can be in
 * any format recognized by libarchive) and writes certain entries to
 * an uncompressed ustar archive on stdout.  This is a template for
 * many kinds of archive manipulation: converting formats, resetting
 * ownership, inserting entries, removing entries, etc.
 *
 * To compile:
 * gcc -Wall -o tarfilter tarfilter.c -larchive -lz -lbz2
 */

#include <archive.h>
#include <archive_entry.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

static void die(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

void test_targz_in_memory(void *buff, int buffSize) {
  struct archive *a = archive_read_new();
  archive_read_support_compression_gzip(a);
  archive_read_support_format_tar(a);
  archive_read_support_format_tar(a);
  archive_read_support_format_cpio(a);
  int r = archive_read_open_memory(a, buff, buffSize);
}

void test_compress_to_memory() {
  struct archive *a = archive_write_new();
  char buff[1024];
  int buffSize = 1024, outUsed;

  archive_write_set_bytes_per_block(a, 1024);
  archive_write_set_compression_bzip2(a);
//  archive_write_open_memory(a, buff, buffSize, &outUsed);
}

void write_archive(const char *outname, const char **filename) {
  struct archive *a;
  struct archive_entry *entry;
  struct stat st;
  char buff[8 * 1024];
  int len;
  int fd;
  size_t buff_size = 100 * 1024;
  size_t used;
  char *buffer;

  a = archive_write_new();
  archive_write_add_filter_gzip(a);
  archive_write_set_format_ustar(a);
  buffer = (char *)malloc(buff_size);
  if (buffer == NULL) {
    printf("Error allocating memory!");
    exit(1);
  }
  archive_write_open_memory(a, buffer, buff_size, &used);
  while (*filename) {
    stat(*filename, &st);
    entry = archive_entry_new();
    archive_entry_copy_stat(entry, &st);
    archive_entry_set_pathname(entry, *filename);
    archive_write_header(a, entry);
    if ((fd = open(*filename, O_RDONLY)) != -1) {
      len = read(fd, buff, sizeof(buff));
      while (len > 0) {
        archive_write_data(a, buff, len);
        len = read(fd, buff, sizeof(buff));
      }
      close(fd);
    }
    archive_entry_free(entry);
    filename++;
  }
  archive_write_free(a);
  fd = open(outname, O_WRONLY | O_CREAT, 0644);
  write(fd, buffer, used);
  close(fd);
  free(buffer);
}

int extract(const char *filename) {
  struct archive *a;

  struct archive_entry *entry;
  int flags;
  int len, err = 0;
  char buff[8192];

  // Select which attributes we want to restore.
  flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL |
          ARCHIVE_EXTRACT_FFLAGS;

  // create archive
  a = archive_read_new();
  archive_read_support_format_all(a);
  archive_read_support_compression_all(a);

  // open file
  if (archive_read_open_filename(a, filename, 10240) == ARCHIVE_OK) {
    // read all headers
    for (int r = 0; (r = archive_read_next_header(a, &entry)) != ARCHIVE_EOF;) {
      if (r < ARCHIVE_OK || r < ARCHIVE_WARN) {
        err = -2;
        break;
      }

      if (!S_ISREG(archive_entry_mode(entry))) {
        // fprintf(stderr, "skipped\n");
        continue;
      }

      //      fprintf(stderr, "%s\n", archive_entry_pathname(entry));

      // read it
      if (archive_entry_size(entry) > 0) {
        for (len = 0; (len = archive_read_data(a, buff, sizeof(buff)) > 0);) {
        }
        if (len < 0) {
          err = -1;
          break;
        }
      }
    }
  } else
    err = -3;  // file not found

  // close & free archive
  archive_read_close(a);
  archive_read_free(a);
  return err;
}

int read_targz(const char *fnme) {
  struct archive *ina;
  struct archive_entry *entry;
  int fh;
  int r;
  const int blkSize = 1024;
  char buff[8192];

  ina = archive_read_new();
  if (archive_read_open_filename(ina, fnme, blkSize) != ARCHIVE_OK) {
    if (archive_read_support_filter_all(ina) == ARCHIVE_OK) {
      if (archive_read_support_format_all(ina) == ARCHIVE_OK) {
        if ((fh = archive_read_open_fd(ina, 0, blkSize)) != ARCHIVE_OK) {
          while ((r = archive_read_next_header(ina, &entry)) == ARCHIVE_OK) {
            fprintf(stderr, "%s: ", archive_entry_pathname(entry));
            // Skip anything that isn't a regular file.
            if (!S_ISREG(archive_entry_mode(entry))) {
              fprintf(stderr, "skipped\n");
              continue;
            }
            if (archive_entry_size(entry) > 0) {
              int len = archive_read_data(ina, buff, sizeof(buff));
              while (len > 0) {
                len = archive_read_data(ina, buff, sizeof(buff));
              }
              if (len < 0) die("Error reading input archive");
            }
          }
        }
        // Close the archives.
        if (archive_read_free(ina) != ARCHIVE_OK)
          die("Error closing input archive");
      }
    }
  }
}

int test_targz(int argc, char **argv) {
  char buff[8192];
  ssize_t len;
  int r;
  mode_t m;
  struct archive *ina;
  struct archive *outa;
  struct archive_entry *entry;

  /* Read an archive from stdin, with automatic format detection. */
  ina = archive_read_new();
  if (ina == NULL) die("Couldn't create archive reader.");
  if (archive_read_support_filter_all(ina) != ARCHIVE_OK)
    die("Couldn't enable decompression");
  if (archive_read_support_format_all(ina) != ARCHIVE_OK)
    die("Couldn't enable read formats");
  if (archive_read_open_fd(ina, 0, 10240) != ARCHIVE_OK)
    die("Couldn't open input archive");

  /* Write an uncompressed ustar archive to stdout. */
  outa = archive_write_new();
  if (outa == NULL) die("Couldn't create archive writer.");
  if (archive_write_set_compression_none(outa) != ARCHIVE_OK)
    die("Couldn't enable compression");
  if (archive_write_set_format_ustar(outa) != ARCHIVE_OK)
    die("Couldn't set output format");
  if (archive_write_open_fd(outa, 1) != ARCHIVE_OK)
    die("Couldn't open output archive");

  /* Examine each entry in the input archive. */
  while ((r = archive_read_next_header(ina, &entry)) == ARCHIVE_OK) {
    fprintf(stderr, "%s: ", archive_entry_pathname(entry));

    /* Skip anything that isn't a regular file. */
    if (!S_ISREG(archive_entry_mode(entry))) {
      fprintf(stderr, "skipped\n");
      continue;
    }

    /* Make everything owned by root/wheel. */
    archive_entry_set_uid(entry, 0);
    archive_entry_set_uname(entry, "root");
    archive_entry_set_gid(entry, 0);
    archive_entry_set_gname(entry, "wheel");

    /* Make everything permission 0744, strip SUID, etc. */
    m = archive_entry_mode(entry);
    archive_entry_set_mode(entry, (m & ~07777) | 0744);

    /* Copy input entries to output archive. */
    if (archive_write_header(outa, entry) != ARCHIVE_OK)
      die("Error writing output archive");
    if (archive_entry_size(entry) > 0) {
      len = archive_read_data(ina, buff, sizeof(buff));
      while (len > 0) {
        if (archive_write_data(outa, buff, len) != len)
          die("Error writing output archive");
        len = archive_read_data(ina, buff, sizeof(buff));
      }
      if (len < 0) die("Error reading input archive");
    }
    fprintf(stderr, "copied\n");
  }
  if (r != ARCHIVE_EOF) die("Error reading archive");
  /* Close the archives.  */
  if (archive_read_free(ina) != ARCHIVE_OK) die("Error closing input archive");
  if (archive_write_free(outa) != ARCHIVE_OK)
    die("Error closing output archive");
  return (0);
}
