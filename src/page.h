#ifndef PAGE_H
#define PAGE_H

struct ppage {
  struct ppage *next;
  struct ppage *prev;
  void *physical_addr;
};

// Page directory entry for i386
struct page_directory_entry {
  unsigned int present : 1;
  unsigned int rw : 1;
  unsigned int user : 1;
  unsigned int writethru : 1;
  unsigned int cachedisabled : 1;
  unsigned int accessed : 1;
  unsigned int reserved : 1;
  unsigned int pagesize : 1;
  unsigned int ignored : 1;
  unsigned int os_specific : 3;
  unsigned int frame : 20;
};

// Page table entry for i386
struct page {
  unsigned int present : 1;
  unsigned int rw : 1;
  unsigned int user : 1;
  unsigned int writethru : 1;
  unsigned int cachedisabled : 1;
  unsigned int accessed : 1;
  unsigned int dirty : 1;
  unsigned int unused : 5;
  unsigned int frame : 20;
};

// The head of the free list (declared in page.c)
extern struct ppage *free_page_list;

void init_pfa_list(void); // Initialize the free physical page list
struct ppage *allocate_physical_pages(unsigned int npages); // Allocate 'npages' physical pages and return a list
void free_physical_pages(struct ppage *ppage_list); // Free a list of physical pages back to the free list


#endif
