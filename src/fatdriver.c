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
  // Read boot sector
  sector_read(2048, boot_sector);
  struct boot_sector *bs = (struct boot_sector*)boot_sector;

  printf("===== BOOT SECTOR INFO =====\n");
  printf("Bytes per sector: %d\n", bs->bytes_per_sector);
  printf("Sectors per cluster: %d\n", bs->num_sectors_per_cluster);
  printf("Reserved sectors: %d\n", bs->num_reserved_sectors);
  printf("Number of FAT tables: %d\n", bs->num_fat_tables);
  printf("Sectors per FAT: %d\n", bs->num_sectors_per_fat);
  printf("Root directory entries: %d\n", bs->num_root_dir_entries);

  // Validate boot signature
  if (bs->boot_signature != 0xAA55) {
	  printf("WARNING: Invalid boot signature: 0x%X\n", bs->boot_signature);
  }

  // Validate filesystem type
  if (strncmp(bs->fs_type, "FAT12", 5) != 0 && strncmp(bs->fs_type, "FAT16", 5) != 0) {
	  printf("WARNING: Unsupported FS type: %.8s\n", bs->fs_type);
  }

  // Compute important region starts
  int first_fat_sector = 2048 + bs->num_reserved_sectors;
  root_dir_region_start = first_fat_sector + (bs->num_fat_tables * bs->num_sectors_per_fat);

  printf("First FAT starts at sector: %d\n", first_fat_sector);
  printf("Root directory starts at sector: %d\n", root_dir_region_start);

  // Read the first FAT table (into memory)
  int fat_table_bytes = bs->num_sectors_per_fat * bs->bytes_per_sector;
  char *fat_table = malloc(fat_table_bytes);
  if (!fat_table) {
	  printf("ERROR: Failed to allocate memory for FAT table\n");
	  return;
  }

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

  char name[9], ext[4];
  memset(name, ' ', 8);
  memset(ext, ' ', 3);
  name[8] = ext[3] = '\0';

  char *dot = strchr(path, '.');
  if (dot) {
      int len = dot - path;
      if (len > 8) len = 8;
      strncpy(name, path, len);
      strncpy(ext, dot + 1, 3);
  } else {
      strncpy(name, path, 8);
  }

  // Convert to uppercase
  for (int i = 0; i < 8; i++) name[i] = toupper((unsigned char)name[i]);
  for (int i = 0; i < 3; i++) ext[i] = toupper((unsigned char)ext[i]);

  // iterate through the RDE region searching for a file's RDE.
  for(int k = 0; k < 10; k++) {
    printf("File name: \"%s.%s\"\n", rde[k].file_name, rde[k].file_extension);
    printf("Data cluster: %d\n", rde[k].cluster);
    printf("File size: %d\n", rde[k].file_size);

    // Check for match
    if (strncmp(rde[k].file_name, name, 8) == 0 &&
	strncmp(rde[k].file_extension, ext, 3) == 0) {
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

  // Proper data region start calculation	
  int root_dir_sectors = (bs->num_root_dir_entries * 32 / bs->bytes_per_sector - 1) / bs->bytes_per_sector;
  int data_region_start = root_dir_region_start + root_dir_sectors;

  // Correct sector offset for file cluster
  int sector = data_region_start + (rde->cluster - 2) * bs->num_sectors_per_cluster;

  // Read all sectors in the file's first cluster
  int total_bytes = 0;
  for (int i = 0; i < bs->num_sectors_per_cluster && total_bytes < n; i++) {
      sector_read(sector + i, buf + i * bs->bytes_per_sector);
	  total_bytes += bs->bytes_per_sector;
  }
	
  return total_bytes;

}

//int main() {
  //struct root_directory_entry *rde = root_directory_region;
  //char dataBuf[100];

  //driver_init("disk.img");
  //fatInit();
  //struct rde *rde = fatOpen("file.txt");
  //fatRead(rde, dataBuf, sizeof(dataBuf));

  //printf("data read from file = %s\n", dataBuf);

  //return 0;
}
