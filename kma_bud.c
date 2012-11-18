/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_bud.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_bud.c,v $
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
#ifdef KMA_BUD
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
} buffer_struct;

typedef struct {
  int size;
  buffer_struct* buffer;
} listheader_struct;

typedef struct {
  kpage_t* page_ptr;
  void* addr;
  int allocnum;
  unsigned char bm[64];
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

mainheader_struct* g_pagemain=0;

/************Function Prototypes******************************************/
int
free_list_size(kma_size_t size);
listheader_struct*
coalesce(listheader_struct* list, pageheader_struct* page);
listheader_struct*
split(listheader_struct* list, kma_size_t size);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size) {
  // Round size up
  kma_size_t s=16;
  while(size > s){
    s = s<<1;
  }
  int size_rounded = s, i;

  void* ret;

  if ((size + sizeof(void*)) > PAGESIZE) {
    // The size of our request is too big
    return NULL;
  }
  // Initialize
  if (!g_pagemain) {
    kpage_t* page = get_page();
    mainheader_struct* ret_page = (mainheader_struct*)(page->ptr);
    (*ret_page).head = page;
    (*ret_page).page_next=0;
    (*ret_page).pagesnum=0;
    (*ret_page).allocnum=0;
    for (i = 0; i < 91; i++) {
      (*ret_page).page[i].page_ptr=0;
      (*ret_page).page[i].addr=0;
    }
    for (i = 0; i < 10; i++) {
      (*ret_page).free_list[i].buffer = 0;
      (*ret_page).free_list[i].size=16<<i;
    }
    g_pagemain = ret_page;
  }

  if (!(i=free_list_size(size))) {
    listheader_struct* list;
    pageheader_struct* page;
    pageheader_struct* ret_page = 0;
    mainheader_struct* temp = g_pagemain;

    while (!ret_page) {
      // Want to find page header, 91 = number of pages
      for(i = 0; i < 91; i++) {
        if ((*temp).page[i].page_ptr==0) {
          ret_page=&((*temp).page[i]);
          break;
        }
      }
      // Break if the page is found
      if (((*temp).page_next==0) || ret_page) {
        break;
      }

      temp = (*temp).page_next;
    }
    
    if (ret_page == 0) {
      kpage_t* n_page = get_page();
      mainheader_struct* temp_ret = (mainheader_struct*)(n_page->ptr);
      (*temp_ret).head = n_page;
      (*temp_ret).page_next=0;
      (*temp_ret).pagesnum=0;
      (*temp_ret).allocnum=0;
      for (i = 0; i < 91; i++) {
        (*temp_ret).page[i].page_ptr=0;
        (*temp_ret).page[i].addr=0;
      }
      for (i = 0; i < 10; i++) {
        (*temp_ret).free_list[i].buffer = 0;
        (*temp_ret).free_list[i].size=16<<i;
      }
      (*temp).page_next = temp_ret;

      temp=(*temp).page_next;
      ret_page=&((*temp).page[0]);
      kpage_t* page = get_page();

      (*ret_page).allocnum=0;
      (*ret_page).page_ptr = page;
      (*ret_page).addr = (void*)(page->ptr);

      int j;
      for(j = 0; j < 64; j++) {
        (*ret_page).bm[j]=0;
      }

      *((buffer_struct**)((*ret_page).addr)) = 0;

      listheader_struct* list = &((*g_pagemain).free_list[9]);
      buffer_struct* buff = (buffer_struct*)((*ret_page).addr);

      buffer_struct* temp_buff = (*list).buffer;
      (*list).buffer = buff;
      (*buff).next_buff = temp_buff;

      (*g_pagemain).pagesnum++;
      (*temp).pagesnum++;
    }
    else {
      kpage_t* page = get_page();

      (*ret_page).allocnum=0;
      (*ret_page).page_ptr = page;
      (*ret_page).addr = (void*)(page->ptr);

      int j;
      for(j = 0; j < 64; j++) {
        (*ret_page).bm[j]=0;
      }

      *((buffer_struct**)((*ret_page).addr)) = 0;

      listheader_struct* list = &((*g_pagemain).free_list[9]);
      buffer_struct* buff = (buffer_struct*)((*ret_page).addr);

      buffer_struct* temp_buff = (*list).buffer;
      (*list).buffer = buff;
      (*buff).next_buff = temp_buff;

      (*g_pagemain).pagesnum++;
      (*temp).pagesnum++;
    }
    page = ret_page;

    unsigned char* bm = (*page).bm;
    int i, end;

    list = split(&((*g_pagemain).free_list[9]), size_rounded);
    buffer_struct* ret_buff = (*list).buffer;
    (*list).buffer=(*ret_buff).next_buff;
    ret = (buffer_struct*)ret_buff;

    // Round up
    int os = (int)(ret - (*page).addr);
    end = (size_rounded + os)/16;
    os /= 16;

    for (i = os; i < end; i++) {
      bm[i/8] |= (1<<(i%8));
    }

    (*page).allocnum++;
    (*g_pagemain).allocnum++;
    return (void*)ret;
  }
  // We have the new page that we want
  else {
    pageheader_struct* page=0;
    listheader_struct* list;
    mainheader_struct* temp = g_pagemain;
    unsigned char* bm;
    int end;
    i--;

    list = split(&((*g_pagemain).free_list[i]), size_rounded);
    buffer_struct* ret_buff = (*list).buffer;
    (*list).buffer=(*ret_buff).next_buff;
    ret = (buffer_struct*)ret_buff;

    void* address = (void*)(((long int)(((long int)ret - (long int)g_pagemain)/PAGESIZE)) * PAGESIZE + (long int)g_pagemain);

    // Unlink the first buffer in the free list 
    while(!page){
      for(i = 0; i < 91; i++) {
        if((*temp).page[i].addr == address){
          page = &((*temp).page[i]);
          break;
        }
      }
      if (page || ((*temp).page_next == 0)) {
        break;
      }
      temp = (*temp).page_next;
    }

    // Fill bitmap
    bm = (*page).bm;
    int j, os = (int)(ret - (*page).addr);
    end = (size_rounded + os)/16;
    os /= 16;

    for (j = os; j < end; j++) {
      bm[j/8] |= (1<<(j%8));
    }

    (*page).allocnum++;
    (*g_pagemain).allocnum++;
    return (void*)ret;
  }

  return NULL;
}

// Check the size of the free list
int free_list_size (kma_size_t size) {
  kma_size_t ret=16;
  while(size > ret){
    ret = ret<<1;
  }
  int size_rounded = ret, i;

  for(i = 0; i < 10; i++) {
    if((size_rounded <= (*g_pagemain).free_list[i].size) && (*g_pagemain).free_list[i].buffer!=0) {
      return i+1;
    }
  }
  return 0;
}

void kma_free(void* ptr, kma_size_t size) {
  // Round up
  kma_size_t s=16;
  while(size > s){
    s = s<<1;
  }
  // init vars
  int size_rounded = s, i, end;
  listheader_struct* list=0;
  pageheader_struct* page=0;
  mainheader_struct* prev=0;
  mainheader_struct* temp=g_pagemain;
  void* address = (void*)(((long int)(((long int)ptr - (long int)g_pagemain)/PAGESIZE)) * PAGESIZE + (long int)g_pagemain);

  while(!page){
    // get the page header, 91 = num of pages
    for(i = 0; i < 91; i++) {
      if ((*temp).page[i].addr == address) {
        page = &((*temp).page[i]);
        break;
      }
    }
    if (((*temp).page_next == 0) || page) {
      break;
    }

    prev = temp;
    temp = (*temp).page_next;
  }

  for (i = 0; i < 10; ++i) {
    // get the header
    if ((*g_pagemain).free_list[i].size == size_rounded) {
      break;
    }
  }

  list = &((*g_pagemain).free_list[i]);
  // Insert buffer
  buffer_struct* buff = ptr;
  buffer_struct* temp_buffer = (*list).buffer;
  (*list).buffer = buff;
  (*buff).next_buff = temp_buffer;
  // Empty bitmap
  unsigned char* bm = (*page).bm;
  int j, os = (int)(ptr - (*page).addr);
  end = (size_rounded + os)/16;
  os /= 16;

  for(j = os; j < end; j++) {
    bm[j/8] &= (~(1<<(j%8)));
  }

  (*page).allocnum--;
  (*g_pagemain).allocnum--;

  listheader_struct* combinedlist = coalesce(list, page);

  if ((*page).allocnum == 0) {
    buffer_struct* ret_buff = (*combinedlist).buffer;
    (*combinedlist).buffer=(*ret_buff).next_buff;
    free_page((*page).page_ptr);
    (*page).addr=0;
    (*page).page_ptr=0;
    (*temp).pagesnum--;
    (*g_pagemain).pagesnum--;
  }

  if ((*g_pagemain).pagesnum == 0) {
    free_page((*g_pagemain).head);
    g_pagemain=0;
  }
  if (((prev != 0) && (*temp).pagesnum==0)) {
    (*prev).page_next = (*temp).page_next;
    free_page((*temp).head);
  }


}

// Free and combine buddy
listheader_struct* coalesce (listheader_struct* list, pageheader_struct* page) {

  listheader_struct* ret = (listheader_struct*)((long int)list + sizeof(listheader_struct));
  unsigned char* bm = (*page).bm;
  kma_size_t size = (*list).size;
  buffer_struct* first_temp;
  buffer_struct* second_temp;
  buffer_struct* third_temp;
  int end, i, os = (int)((void*)((*list).buffer) - (void*)((*page).addr));

  if(((*list).size) == 8192) {
    return list;
  }

  if (!(os%(2*size) == 0)) {
    second_temp = (buffer_struct*)((void*)((*list).buffer) - size);
    third_temp = (*list).buffer;

    os -= size;
    end = os + size;
    os /= 16;
    end /= 16;

    for (i = os; i < end; i++) {
      if((1<<(i%8)) & bm[i/8]) {
        return list;
      }
    }

    buffer_struct* ret = 0;
    void* buff_next = (*list).buffer;
    while(buff_next){
      if((*(buffer_struct*)buff_next).next_buff == second_temp) {
        ret = second_temp;
        (*(buffer_struct*)buff_next).next_buff = (*ret).next_buff;
      }
      buff_next = (*(buffer_struct*)buff_next).next_buff;
    }

    buffer_struct* ret_buff = (*list).buffer;
    (*list).buffer=(*ret_buff).next_buff;
    third_temp = (buffer_struct*)ret_buff;
  }

  else {
    // buddy is next
    second_temp =(*list).buffer;
    third_temp = (buffer_struct*)(size + (void*)((*list).buffer));
    os += size;
    end = os + size;
    os /= 16;
    end /= 16;

    for (i = os; i < end; i++) {
      if((1<<(i%8)) & bm[i/8]) {
        return list;
      }
    }

    buffer_struct* ret_list = 0;
    void* buff_next = (*list).buffer;
    while(buff_next){
      if((*(buffer_struct*)buff_next).next_buff == third_temp) {
        ret_list = third_temp;
        (*(buffer_struct*)buff_next).next_buff = (*ret_list).next_buff;
      }
      buff_next = (*(buffer_struct*)buff_next).next_buff;
    }

    buffer_struct* ret_buff = (*list).buffer;
    (*list).buffer=(*ret_buff).next_buff;
    second_temp = (buffer_struct*)ret_buff;
  }

  // Combine buddy
  first_temp = second_temp;
  buffer_struct* temp_buffer = (*ret).buffer;
  (*ret).buffer = second_temp;
  (*second_temp).next_buff = temp_buffer;

  if (size < 8192) {
    ret = coalesce(ret, page);
  }

  return ret;
}

// Split free list block
listheader_struct* split (listheader_struct* list, kma_size_t size) {

  listheader_struct* ret_list = (listheader_struct*)((long int)list - sizeof(listheader_struct));
  buffer_struct* first_temp;
  buffer_struct* second_temp;
  buffer_struct* third_temp;

  if (size == ((*list).size)) {
    return list;
  }

  buffer_struct* ret_buff = (*list).buffer;
  (*list).buffer=(*ret_buff).next_buff;
  first_temp = (buffer_struct*)ret_buff;
  second_temp = first_temp;
  third_temp = (buffer_struct*)((long int)second_temp + (*ret_list).size);

  buffer_struct* tempbuff = (*ret_list).buffer;
  (*ret_list).buffer = third_temp;
  (*third_temp).next_buff = tempbuff;

  buffer_struct* tempbuff1 = (*ret_list).buffer;
  (*ret_list).buffer = second_temp;
  (*second_temp).next_buff = tempbuff1;

  if(size < (*ret_list).size) {
    ret_list = split(ret_list, size);
  }
  return ret_list;
}

#endif // KMA_BUD