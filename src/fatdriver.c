#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "fat.h"
int fd = 0;
int root_dir_region_start = 0;
char boot_sector[512];
char root_directory_region[512];

// ide.s
void driver_init(char *disk_img_path) {
  fd = open(disk_img_path, O_RDONLY);
}
// ide.s
void sector_read(unsigned int sector_num, char *buf) {
  lseek(fd, sector_num * 512, SEEK_SET);	
  size_t nbytes = read(fd, buf, 512);
}

// fatInit()
void fatInit() {
  // Read boot sector and RDE region
  sector_read(2048, boot_sector);

  struct boot_sector *bs = (struct boot_sector*)boot_sector;

  printf("===== BOOT SECTOR INFO =====\n");
  printf("Bytes per sector: %d\n", bs->bytes_per_sector);
  printf("Sectors per cluster: %d\n", bs->num_sectors_per_cluster);
  printf("Reserved sectors: %d\n", bs->num_reserved_sectors);
  printf("Number of FAT tables: %d\n", bs->num_fat_tables);
  printf("Sectors per FAT: %d\n", bs->num_sectors_per_fat);
  printf("Root directory entries: %d\n", bs->num_root_dir_entries);

  // Compute important region starts
  int first_fat_sector = 2048 + bs->num_reserved_sectors;
  root_dir_region_start = first_fat_sector + (bs->num_fat_tables * bs->num_sectors_per_fat);

  printf("First FAT starts at sector: %d\n", first_fat_sector);
  printf("Root directory starts at sector: %d\n", root_dir_region_start);

  // Read the first FAT table (into memory)
  int fat_table_bytes = bs->num_sectors_per_fat * bs->bytes_per_sector;
  char *fat_table = malloc(fat_table_bytes);

  for (int i = 0; i < bs->num_sectors_per_fat; i++) {
      sector_read(first_fat_sector + i, fat_table + (i * bs->bytes_per_sector));
  }

  printf("FAT table loaded (%d bytes)\n", fat_table_bytes);

  // Read the root directory region
  int root_dir_bytes = bs->num_root_dir_entries * 32;
  int root_dir_sectors = (root_dir_bytes + bs->bytes_per_sector - 1) / bs->bytes_per_sector;

  for (int i = 0; i < root_dir_sectors; i++) {
      sector_read(root_dir_region_start + i, root_directory_region + (i * bs->bytes_per_sector));
  }

  printf("Root directory region loaded (%d bytes)\n", root_dir_bytes);
}

// fatOpen()
// Find the RDE for a file given a path
struct rde * fatOpen( char *path ) {
  struct rde *rde = (struct rde *)root_directory_region;

  // iterate through the RDE region searching for a file's RDE.
  for(int k = 0; k < 10; k++) {
    printf("File name: \"%s.%s\"\n", rde[k].file_name, rde[k].file_extension);
    printf("Data cluster: %d\n", rde[k].cluster);
    printf("File size: %d\n", rde[k].file_size);

    // Check for match
    if (strncmp(rde[k].file_name, "FILE    ", 8) == 0 &&
	strncmp(rde[k].file_extension, "TXT", 3) == 0) {
      return &rde[k];
    } 
  }

  return NULL;
}	

int fatRead(struct rde *rde, char * buf, int n) {
  // read file data into buf from file described by rde.
  if (!rde) {
    return -1;
  }

  struct boot_sector *bs = (struct boot_sector*)boot_sector;
  int data_region_start = root_dir_region_start + (bs->num_root_dir_entries * 32 / bs->bytes_per_sector);
  int sector = data_region_start + (rde->cluster - 2) * bs->num_sectors_per_cluster;

  sector_read(sector, buf);
  return n;

}

int main() {
  struct root_directory_entry *rde = root_directory_region;
  char dataBuf[100];

  driver_init("disk.img");
  fatInit();
  struct rde *rde = fatOpen("file.txt");
  fatRead(rde, dataBuf, sizeof(dataBuf));

  printf("data read from file = %s\n", dataBuf);

  return 0;
}
