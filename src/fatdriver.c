#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
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

  //esp_printf("===== BOOT SECTOR INFO =====\n");
  esp_printf("Bytes per sector: %d\n", bs->bytes_per_sector);
  esp_printf("Sectors per cluster: %d\n", bs->num_sectors_per_cluster);
  esp_printf("Reserved sectors: %d\n", bs->num_reserved_sectors);
  esp_printf("Number of FAT tables: %d\n", bs->num_fat_tables);
  esp_printf("Sectors per FAT: %d\n", bs->num_sectors_per_fat);
  esp_printf("Root directory entries: %d\n", bs->num_root_dir_entries);

  // Validate boot signature
  if (bs->boot_signature != 0xaa55) {
	  esp_printf("WARNING: Invalid boot signature: 0x%X\n", bs->boot_signature);
  }

  // Validate filesystem type
  if (strncmp(bs->fs_type, "FAT12", 5) != 0 && strncmp(bs->fs_type, "FAT16", 5) != 0) {
	  esp_printf("WARNING: Unsupported FS type: %.8s\n", bs->fs_type);
  }

  // Compute important region starts
  int first_fat_sector = 2048 + bs->num_reserved_sectors;
  root_dir_region_start = first_fat_sector + (bs->num_fat_tables * bs->num_sectors_per_fat);

  esp_printf("First FAT starts at sector: %d\n", first_fat_sector);
  esp_printf("Root directory starts at sector: %d\n", root_dir_region_start);

  // Read the first FAT table (into memory)
  int fat_table_bytes = bs->num_sectors_per_fat * bs->bytes_per_sector;
  char *fat_table = malloc(fat_table_bytes);
  if (!fat_table) {
	  esp_printf("ERROR: Failed to allocate memory for FAT table\n");
	  return;
  }

  for (int i = 0; i < bs->num_sectors_per_fat; i++) {
      sector_read(first_fat_sector + i, fat_table + (i * bs->bytes_per_sector));
  }
  esp_printf("FAT table loaded (%d bytes)\n", fat_table_bytes);

  // Read the root directory region
  int root_dir_bytes = bs->num_root_dir_entries * 32;
  int root_dir_sectors = (root_dir_bytes + bs->bytes_per_sector - 1) / bs->bytes_per_sector;

  for (int i = 0; i < root_dir_sectors; i++) {
      sector_read(root_dir_region_start + i, root_directory_region + (i * bs->bytes_per_sector));
  }

  esp_printf("Root directory region loaded (%d bytes)\n", root_dir_bytes);
}

// fatOpen()
// Find the RDE for a file given a path
struct root_directory_entry * fatOpen( char *path ) {
  struct root_directory_entry *rde = (struct root_directory_entry *)root_directory_region;
  char name[9], ext[4];
  memset(name, ' ', 8);
  memset(ext, ' ', 3);
  name[8] = ext[3] = '\0';

  // Split path into name and ext
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

  // Use actual number of root entries from boot sector
  int num_entries = ((struct boot_sector*)boot_sector)->num_root_dir_entries;

  // iterate through the RDE region searching for a file's RDE.
  for(int k = 0; k < num_entries; k++) {
    esp_printf("File name: \"%8.8s.%3.3s\"\n", rde[k].file_name, rde[k].file_extension);
    esp_printf("Data cluster: %u\n", (unsigned int) rde[k].cluster);
    esp_printf("File size: %u\n", (unsigned int) rde[k].file_size);

    // Check for match
    if (strncmp(rde[k].file_name, name, 8) == 0 &&
	strncmp(rde[k].file_extension, ext, 3) == 0) {
      return &rde[k];
    } 
  }

  return NULL;
}	

int fatRead(struct root_directory_entry *rde, char * buf, int n) {
  // read file data into buf from file described by rde.
  if (!rde) {
    return -1;
  }

  struct boot_sector *bs = (struct boot_sector*)boot_sector;

  // Calculate root dir sectors correctly	
  int root_dir_sectors = (bs->num_root_dir_entries * 32 + bs->bytes_per_sector - 1) / bs->bytes_per_sector;

  // Data region start
  int data_region_start = bs->num_reserved_sectors + (bs->num_fat_tables * bs->num_sectors_per_fat) + root_dir_sectors;

  // Compute sector offset for file's starting cluster
  int sector = data_region_start + (rde->cluster - 2) * bs->num_sectors_per_cluster;

  // Read all sectors in the file's first cluster
  int total_bytes = 0;
  for (int i = 0; i < bs->num_sectors_per_cluster && total_bytes < n; i++) {
	  int bytes_to_read = bs->bytes_per_sector;
	  if (total_bytes + bytes_to_read > n)
		  bytes_to_read = n - total_bytes;
	  
      sector_read(sector + i, buf + total_bytes);
	  total_bytes += bytes_to_read;
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
//}
