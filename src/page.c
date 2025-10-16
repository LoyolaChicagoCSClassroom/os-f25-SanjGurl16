#include "page.h"

struct ppage physical_page_array[128]; // 128 pages, each 2mb in length covers 256 megs of memory
struct ppage *free_physical_pages = 0;

void init_pfa_list(void) {
  for (int i = 0; i < 128; i++) {
      physical_page_array[i].physical_addr = (void *) (i * 0x200000); // each page = 2MB
      physical_page_array[i].next = (i < 127) ? &physical_page_array[i + 1]: 0;
      physical_page_array[i].prev = (i > 0) ? &physical_page_array[i - 1] : 0;
  }
  free_physical_pages = &physical_page_array[0];

}

struct ppage *allocate_physical_pages(unsigned int npages) {
  if (free_physical_pages == 0 || npages == 0)
      return 0;

  struct ppage *allocd_list= free_physical_pages;
  struct ppage *tail = allocd_list;

  for (unsigned int i = 1; i < npages && tail->next; i++) 
      tail = tail->next;

  // unlink allocated pages
  free_physical_pages = tail->next;
  if (free_physical_pages)
      free_physical_pages->prev = 0;

  tail->next = 0; // terminate allocated list
  allocd_list->prev= 0;

  return allocd_list;
}

void free_physical_pages(struct ppage *ppage_list) {
  if (ppage_list == 0)
      return;

  struct ppage *tail = ppage_list;
  while (tail->next)
      tail = tail->next;

  // append freed list to the front of the free list
  tail->next = free_physical_pages;
  if (free_physical_pages)
      free_physical_pages->prev = tail;

  free_physical_pages = ppage_list;
  ppage_list->prev = 0;
}
