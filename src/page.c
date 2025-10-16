#include "page.h"
#include <stdint.h> // for uintptr_t

struct ppage physical_page_array[128]; // 128 pages, each 2mb in length covers 256 megs of memory
struct ppage *free_page_list = 0; // Head of the free physical pages list

extern int _end_kernel; // Linker symbol for the end of the kernel

// Initialize the free physical page list
void init_pfa_list(void) {
  uintptr_t addr = (uintptr_t)&_end_kernel; // start after kernel

  for (int i = 0; i < 128; i++) {
      physical_page_array[i].physical_addr = (void *)addr;
      physical_page_array[i].next = (i < 128 - 1) ? &physical_page_array[i + 1]: 0;
      physical_page_array[i].prev = (i > 0) ? &physical_page_array[i - 1] : 0;

      addr += 0x200000; // move to next 2MB page
  }

  free_page_list = &physical_page_array[0];

}

// Allocate 'npages' physical pages and return a new list
struct ppage *allocate_physical_pages(unsigned int npages) {
  if (free_page_list == 0 || npages == 0)
      return 0;

  struct ppage *allocd_list= free_page_list;
  struct ppage *tail = allocd_list;

  // Traverse to the last page of the allocation
  for (unsigned int i = 1; i < npages && tail->next; i++) 
      tail = tail->next;

  // Unlink allocated pages from the free list
  free_page_list = tail->next;
  if (free_page_list)
      free_page_list->prev = 0;

  tail->next = 0; // terminate allocated list
  allocd_list->prev= 0;

  return allocd_list;
}

// Free a list of physical pages back to the free list
void free_physical_pages(struct ppage *ppage_list) {
  if (ppage_list == 0)
      return;

  struct ppage *tail = ppage_list;
  while (tail->next)
      tail = tail->next;

  // Append freed list to the front of the free list
  tail->next = free_page_list;
  if (free_page_list)
      free_page_list->prev = tail;

  free_page_list = ppage_list;
  ppage_list->prev = 0;
}
