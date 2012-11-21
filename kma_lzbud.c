/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the SVR4 lazy budy
 *             algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_lzbud.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_lzbud.c,v $
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_LZBUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>

/************Private include**********************************************/
#include "kpage.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

typedef struct {
  void* next_buff;
  int islocal;
  int size;
} buffer_struct;

typedef struct {
  int size;
  int s;
  buffer_struct* buffer;
} listheader_struct;

typedef struct {
  kpage_t* page_ptr;
  void* addr;
  int allocnum;
  unsigned char bitmap[64];
} pageheader_struct;

typedef struct {
  kpage_t* head;
  void* page_next;
  int pagesnum;
  int allocnum;
  listheader_struct free_list[10];
  pageheader_struct page[91];
} mainheader_struct;

/************Global Variables*********************************************/

#define G_MAXSIZE 8192 // Assume requests for space < 8000
mainheader_struct* g_pagemain=0;

/************Function Prototypes******************************************/
listheader_struct*
coalesce(void* ptr, listheader_struct* list, pageheader_struct* page);
void
free_buff(mainheader_struct* temp, mainheader_struct* prev, void* address, listheader_struct* list);
buffer_struct*
buff_unlink(listheader_struct* list, buffer_struct* addr);
void
add_buff (buffer_struct* buff, listheader_struct* list);
listheader_struct*
split(pageheader_struct* page, listheader_struct* list, kma_size_t size);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

/* kma_malloc */
void*
kma_malloc(kma_size_t size) {
  int i, size_rounded;
  /* Round up size */
  kma_size_t ret_size = 16;
  while (size>ret_size) {
    ret_size = ret_size<<1;
  }
  size_rounded = ret_size;
  /* Round up size */
  buffer_struct* ret;

  if (!g_pagemain) {
    /* Initialize header */
    kpage_t* page = get_page();
    mainheader_struct* ret_header;
    ret_header=(mainheader_struct*)(page->ptr);
    (*ret_header).head = page;
    (*ret_header).allocnum = 0;
    (*ret_header).page_next = 0;
    (*ret_header).pagesnum = 0;
    for(i = 0; i < 91; i++) {
      (*ret_header).page[i].page_ptr=0;
      (*ret_header).page[i].addr=0;
    }
    for(i = 0; i < 10; i++) {
      (*ret_header).free_list[i].buffer=0;
      (*ret_header).free_list[i].s=0;
      (*ret_header).free_list[i].size=16<<i;
    }
    g_pagemain = ret_header;
    /* Initialize header */
  }

  if ((size + sizeof(void*)) > PAGESIZE){
    return NULL;
  }

  // Get size of the free list
  int j, res = 0;
  for (j = 0; j < 10; j++) {
    if ((size_rounded <= (*g_pagemain).free_list[j].size) && (*g_pagemain).free_list[j].buffer != 0) {
      res = j+1;
      break;
    }
  }

  if (!(i = res)) {
    pageheader_struct* page;
    /* Check to see if there is a free page */
    pageheader_struct* page_ret=0;
    mainheader_struct* temp = g_pagemain;

    // find the available page header
    while (!page_ret) {
      for(i = 0; i < 91; i++) {
        if ((*temp).page[i].page_ptr==0) {
          page_ret=&((*temp).page[i]);
          break;
        }
      }
      // If we find the page or there is no next page, break
      if(page_ret || ((*temp).page_next==0)) {
        break;
      }
      temp=(*temp).page_next;
    }

    if (page_ret != 0) {
      /* initialize page */
      kpage_t* page = get_page();
      (*page_ret).page_ptr=page;
      (*page_ret).addr=(void*)(page->ptr);
      (*page_ret).allocnum=0;
      for(i = 0; i < 64; i++) { (*page_ret).bitmap[i]=0; }
      buffer_struct* temp_buff;
      temp_buff = (*page_ret).addr;
      (*temp_buff).size = G_MAXSIZE;
      (*temp_buff).islocal = 0;
      /* tmp var for adding buff */
      listheader_struct* tmp_list = &((*g_pagemain).free_list[9]);
      add_buff(temp_buff, tmp_list);
      /* initialize page */
      (*g_pagemain).pagesnum++;
      (*temp).pagesnum++;
    }
    else {
      /* Initialize temp page */
      kpage_t* page = get_page();
      mainheader_struct* ret_header_tmp;
      ret_header_tmp=(mainheader_struct*)(page->ptr);
      (*ret_header_tmp).head = page;
      (*ret_header_tmp).allocnum = 0;
      (*ret_header_tmp).page_next = 0;
      (*ret_header_tmp).pagesnum = 0;
      for(i = 0; i < 91; i++) {
        (*ret_header_tmp).page[i].page_ptr=0;
        (*ret_header_tmp).page[i].addr=0;
      }
      for(i = 0; i < 10; i++) {
        (*ret_header_tmp).free_list[i].buffer=0;
        (*ret_header_tmp).free_list[i].s=0;
        (*ret_header_tmp).free_list[i].size=16<<i;
      }
      (*temp).page_next = ret_header_tmp;
      /* Initialize temp page */
      temp=(*temp).page_next;
      page_ret = &((*temp).page[0]);
      /* initialize page */
      kpage_t* curr_page = get_page();
      (*page_ret).page_ptr = curr_page;
      (*page_ret).addr=(void*)(curr_page->ptr);
      (*page_ret).allocnum=0;
      for(i = 0; i < 64; i++) {
        (*page_ret).bitmap[i]=0;
      }
      buffer_struct* temp_buff;
      temp_buff = (*page_ret).addr;
      (*temp_buff).size = G_MAXSIZE;
      (*temp_buff).islocal = 0;
      /* tmp var for adding buff */
      listheader_struct* tmp_list = &((*g_pagemain).free_list[9]);
      add_buff(temp_buff, tmp_list);
      /* initialize page */
      (*g_pagemain).pagesnum++;
      (*temp).pagesnum++;
    }

    page = page_ret;
    /* Check to see if there is a free page */
    listheader_struct* list;

    list = split (page ,&((*g_pagemain).free_list[9]), size_rounded);
    /* unlink the buffer */
    ret = (*list).buffer;
    (*list).buffer=(*ret).next_buff;
    /* unlink the buffer */

    /* fill bitmap */
    unsigned char* bm = (*page).bitmap;
    int i, end, os = (int)((void*)ret-(*page).addr);
    end = os + size_rounded;
    os /= 16;
    end /= 16;

    for(i = os; i < end; ++i) { bm[i/8] |= (1<<(i%8)); }
    /* fill bitmap */

    if ((*ret).islocal == 1) {
      (*list).s = (*list).s + 2;
    }
    else {
      (*list).s++;
      (*page).allocnum++;
      (*g_pagemain).allocnum++;
    }
    return (void*)ret;
  }

  // Create a new page if there aren't enough pages available
  else {
    i--;
    pageheader_struct* page = 0;
    listheader_struct* list = &((*g_pagemain).free_list[i]);
    mainheader_struct* temp = g_pagemain;
    void* address = (void*)(((long int)(((long int)(((*g_pagemain).free_list[i]).buffer)-(long int)g_pagemain)/PAGESIZE))*PAGESIZE+(long int)g_pagemain);

    // Get the page header
    while (!page) {
      for(i = 0; i < 91; ++i) {
        if ((*temp).page[i].addr == address){
          page = &((*temp).page[i]);
          break;
        }
      }
      // If we find the page, break
      if (((*temp).page_next==0) || page) { break; }
      temp = (*temp).page_next;
    }

    list = split (page, list, size_rounded);
    /* unlink the buffer */
    ret = (*list).buffer;
    (*list).buffer=(*ret).next_buff;
    /* unlink the buffer */

    /* fill bitmap */
    unsigned char* bm = (*page).bitmap;
    int i, end, os = (int)((void*)ret-(*page).addr);
    end = os + size_rounded;
    os /= 16;
    end /= 16;

    for(i = os; i < end; ++i) {
      bm[i/8] |= (1<<(i%8));
    }
    /* fill bitmap */

    if ((*ret).islocal == 0) {
      (*g_pagemain).allocnum++;
       (*list).s++;
      (*page).allocnum++;
    }
    else {
      (*list).s = (*list).s + 2;
    }
    return (void*)ret;
  }
  return NULL;
} /* kma_malloc */

/* kma_free */
void kma_free(void* ptr, kma_size_t size) {
  int size_rounded, i;
  listheader_struct* list = 0;
  listheader_struct* combined_list;
  pageheader_struct* page = 0;
  void* address = (void*)(((long int)(((long int)ptr - (long int)g_pagemain)/PAGESIZE)) * PAGESIZE+(long int)g_pagemain);
  mainheader_struct* temp = g_pagemain;
  mainheader_struct* prev = 0;

  /* Round up size */
  kma_size_t ret_size = 16;
  while (size>ret_size) {
    ret_size = ret_size<<1;
  }
  size_rounded = ret_size;
  /* Round up size */

  // Get the page and get the page header
  while (!page) {
    for(i = 0; i < 91; i++) {
      if ((*temp).page[i].addr == address) {
        page = &((*temp).page[i]);
        break;
      }
    }
    // If we find the page or there is no next page, break
    if (((*temp).page_next==0) || page) {
      break;
    }
    prev = temp;
    temp = (*temp).page_next;
  }
  for(i = 0; i < 10; i++) {
    if ((*g_pagemain).free_list[i].size == size_rounded){
      break;
    }
  }
  list = &((*g_pagemain).free_list[i]);
  add_buff(ptr, list);
  (*((buffer_struct*)ptr)).size = size_rounded;

  if ((*list).s == 1) {
    (*((buffer_struct*)ptr)).islocal = 0;
    /* Empty bitmap */
    unsigned char* bitmap=(*page).bitmap;
    int i, end, os = (int)(ptr-(*page).addr);
    end = os + size_rounded;
    os /= 16;
    end /= 16;
    for(i = os; i < end; i++) { bitmap[i/8] &= (~(1<<(i%8))); }
    /* end empty bitmap */
    (*g_pagemain).allocnum--;
    (*page).allocnum--;
    (*list).s = 0;
    combined_list = coalesce(ptr, list, page);

    if ((*page).allocnum == 0) {
      buff_unlink(&((*g_pagemain).free_list[9]), (*page).addr);
      free_page((*page).page_ptr);
      (*page).page_ptr=0;
      (*page).addr=0;
      (*temp).pagesnum--;
      (*g_pagemain).pagesnum--;
    }

    if ((*temp).pagesnum == 0) {
      temp = g_pagemain;
      prev = 0;
      while(temp) {
        if((prev != 0) && ((*temp).pagesnum==0)) {
          (*prev).page_next=(*temp).page_next;
          free_page((*temp).head);
        }
        else {
          prev = temp;
        }
        temp = (*temp).page_next;
      }
      if ((*g_pagemain).pagesnum==0) {
        free_page((*g_pagemain).head);
        g_pagemain=0;
      }
    }
  }
  else if ((*list).s >= 2) {
    (*list).s=(*list).s-2;
    (*((buffer_struct*)ptr)).islocal = 1;
  }
  else {
    (*((buffer_struct*)ptr)).islocal = 0;
    /* Empty bitmap */
    unsigned char* bitmap=(*page).bitmap;
    int i, end, os = (int)(ptr-(*page).addr);
    end = os + size_rounded;
    os /= 16;
    end /= 16;
    for(i = os; i < end; i++) { bitmap[i/8] &= (~(1<<(i%8))); }
    /* end empty bitmap */
    (*page).allocnum--;
    (*g_pagemain).allocnum--;
    (*list).s = 0;
    combined_list = coalesce(ptr, list, page);

    if ((*page).allocnum == 0) {
      buff_unlink(&((*g_pagemain).free_list[9]), (*page).addr);
      free_page((*page).page_ptr);
      (*g_pagemain).pagesnum--;
      (*temp).pagesnum--;
      (*page).page_ptr=0;
      (*page).addr=0;
    }
    free_buff(temp, prev, address, list);
  }
} /* kma_free */

/* coalesce */
listheader_struct* coalesce (void* ptr, listheader_struct* list, pageheader_struct* page) {
  /* Init variables */
  unsigned char* bm = (*page).bitmap;
  listheader_struct* ret = (listheader_struct*)((long int)list + sizeof(listheader_struct));
  kma_size_t size = (*list).size;
  int i, end, os = (int)(ptr-(void*)((*page).addr));
  buffer_struct* temp1;
  buffer_struct* temp2;
  buffer_struct* temp3;

  if(G_MAXSIZE == ((*list).size)) { return list; }

  if (!(os%(size*2)==0)) { // We know buddy was the previous one
    temp2=(buffer_struct*)(ptr - size);
    temp3=(buffer_struct*)ptr;

    os -= size;
    end = os + size;
    os /= 16;
    end /= 16;

    for (i = os; i < end; i++) {
      if (bm[i/8] & (1<<(i%8))) {
        return list;
      }
    }
  }
  else { // We know buddy is the next one
    temp2=(buffer_struct*)ptr;
    temp3=(buffer_struct*)(ptr + size);

    os += size;
    end = os + size;
    os /= 16;
    end /= 16;

    for (i = os; i < end; i++) {
      if (bm[i/8] & (1<<(i%8))){
        return list;
      }
    }
  }
  // Combine buddy
  temp3 = buff_unlink(list,temp3);
  temp2 = buff_unlink(list,temp2);
  temp1 = temp2;
  add_buff(temp1, ret);

  (*temp1).size=(*ret).size;

  if(size < G_MAXSIZE) { ret = coalesce(temp1, ret, page); }

  return ret;
} /* coalesce */

/* free_buff */
void free_buff(mainheader_struct* temp, mainheader_struct* prev, void* address, listheader_struct* list) {
  /* Init variables */
  mainheader_struct* temppage_tmp = g_pagemain;
  mainheader_struct* prevpage_tmp = 0;
  pageheader_struct* page_tmp = 0;
  listheader_struct* list_tmp = list;
  buffer_struct* ptr_tmp = 0;
  buffer_struct* tempptr_tmp;
  tempptr_tmp = (buffer_struct*)((*list_tmp).buffer);
  int i;

  while (tempptr_tmp) {
    if ((*tempptr_tmp).islocal == 1) {
      ptr_tmp = tempptr_tmp;
    }
    tempptr_tmp = (*tempptr_tmp).next_buff;
  }

  if (!(ptr_tmp == 0)) {
    address  = (void*)(((long int)(((long int)ptr_tmp - (long int)g_pagemain)/PAGESIZE)) * PAGESIZE + (long int)g_pagemain);
    // Get page header
    while (!page_tmp) {
      // 91 = num of pages
      for (i = 0; i < 91; ++i) {
        if ((*temppage_tmp).page[i].addr == address) {
          page_tmp=&((*temppage_tmp).page[i]);
          break;
        }
      }
      // If no next page or page is found, break
      if (page_tmp || ((*temppage_tmp).page_next==0)) {
        break;
      }
      prevpage_tmp = temppage_tmp;
      temppage_tmp = (*temppage_tmp).page_next;
    }
    (*list_tmp).s = 0;
    (*ptr_tmp).islocal = 0;
    (*ptr_tmp).size=(*list_tmp).size;

    /* Empty bitmap */
    unsigned char* bitmap=(*page_tmp).bitmap;
    int i, end, os = (int)((void*)ptr_tmp-(*page_tmp).addr);
    end = os + (*ptr_tmp).size;
    os /= 16;
    end /= 16;
    for(i = os; i < end; ++i) {
      bitmap[i/8] &= (~(1<<(i%8)));
    }
    /* end empty bitmap */
    (*page_tmp).allocnum--;
    (*g_pagemain).allocnum--;

    coalesce(ptr_tmp, list_tmp, page_tmp);

    if ((*page_tmp).allocnum == 0) {
      buff_unlink(&((*g_pagemain).free_list[9]), (*page_tmp).addr);
      free_page((*page_tmp).page_ptr);

      (*g_pagemain).pagesnum--;
      (*temppage_tmp).pagesnum--;
      (*page_tmp).page_ptr=0;
      (*page_tmp).addr=0;
    }
  }
  if (((*temppage_tmp).pagesnum==0) || ((*temp).pagesnum==0)) {
    prev = 0;
    temp = g_pagemain;
    while (temp) {
      if (!((prev != 0) && ((*temp).pagesnum==0))) {
        prev = temp;
      }
      else {
        (*prev).page_next = (*temp).page_next;
        free_page((*temp).head);
      }
      temp = (*temp).page_next;
    }
    if ((*g_pagemain).pagesnum == 0) {
      free_page((*g_pagemain).head);
      g_pagemain = 0;
    }
  }
} /* free_buff */

/* buff_unlink */
buffer_struct* buff_unlink (listheader_struct* list, buffer_struct* addr) {
  void* buffnext = (*list).buffer;
  buffer_struct* ret = 0;

  if(addr == buffnext) {
    ret = addr;
    (*list).buffer=(*ret).next_buff;
    return ret;
  }
  while(buffnext) {
    if ((*(buffer_struct*)buffnext).next_buff == addr) {
      ret = addr;
      (*(buffer_struct*)buffnext).next_buff=(*ret).next_buff;
      return ret;
    }
    buffnext = (*(buffer_struct*)buffnext).next_buff;
  }
  return 0;
} /* buff_unlink */

/* add_buff */
void add_buff (buffer_struct* buff, listheader_struct* list) {
  if ((*list).buffer > buff) {
    (*buff).next_buff = (*list).buffer;
    (*list).buffer = buff;
  }
  else if ((*list).buffer == 0) {
    (*buff).next_buff = 0;
    (*list).buffer = buff;
  }
  else {
    buffer_struct* temp;
    void* temp_next_buff = (*list).buffer;
    while ((*(buffer_struct*)temp_next_buff).next_buff) {
      if((void*)buff < (*(buffer_struct*)temp_next_buff).next_buff) {
        break;
      }
      temp_next_buff = (*(buffer_struct*)temp_next_buff).next_buff;
    }
    temp = (*(buffer_struct*)temp_next_buff).next_buff;
    (*(buffer_struct*)temp_next_buff).next_buff = buff;
    (*buff).next_buff = temp;
  }
} /* add_buff */

/* split */
listheader_struct* split (pageheader_struct* page, listheader_struct* list, kma_size_t size) {
  listheader_struct* ret = (listheader_struct*)((long int)list-sizeof(listheader_struct));
  buffer_struct* temp1;
  buffer_struct* temp2;
  buffer_struct* temp3;

  if (size==((*list).size)) {
    return list;
  }

  /* unlink the buffer */
  buffer_struct* ret_buffer = (*list).buffer;
  (*list).buffer=(*ret_buffer).next_buff;
  temp1 = ret_buffer;
  /* unlink the buffer */
  temp2 = temp1;
  temp3 = (buffer_struct*)((long int)temp2 + (*ret).size);

  (*temp2).islocal=(*temp1).islocal;
  (*temp2).size=(*ret).size;
  (*temp3).islocal=(*temp1).islocal;
  (*temp3).size=(*ret).size;

  if((*temp1).islocal == 1) {
    (*list).s = (*list).s + 1;
    (*ret).s = (*ret).s - 2;
    (*page).allocnum++;
  }

  add_buff(temp3, ret);
  add_buff(temp2, ret);

  if ((*ret).size > size) {
    ret = split(page, ret, size);
  }
  return ret;
} /* split */

#endif // KMA_LZBUD
