#ifndef PAGE_H
#define PAGE_H

struct ppage {
  struct ppage *next;
  struct ppage *prev;
  void *physical_addr;
};

// The head of the free list (declared in page.c)
extern struct ppage *free_page_list;

void init_pfa_list(void); // Initialize the free physical page list
struct ppage *allocate_physical_pages(unsigned int npages); // Allocate 'npages' physical pages and return a list
void free_physical_pages(struct ppage *ppage_list); // Free a list of physical pages back to the free list


#endif
