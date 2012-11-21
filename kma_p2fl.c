/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the power-of-two free list
 *             algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_p2fl.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_p2fl.c,v $
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
#ifdef KMA_P2FL
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
  buffer_struct* buff;
  int size;
} freelist_struct;

typedef struct {
  void* head;
  void* free_mem;
  int page_cnt;
  int alloc_cnt;
  freelist_struct fl[10];
} listheader_struct;

/************Global Variables*********************************************/

kpage_t* g_ptr=0;

/************Function Prototypes******************************************/

void* malloc_buff(listheader_struct* pg, kma_size_t size);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
  // First check to see if we are requesting too large a size
  if ((sizeof(void*) + size) > PAGESIZE){
    return NULL;
  }
  void* result;
  int i;
  listheader_struct* main_pg;
  listheader_struct* tmp_pg;

  if(!g_ptr) {
    // Initialize entry
    kpage_t* pg = get_page();
    listheader_struct* lst_head;
    *((kpage_t**)pg->ptr) = pg;
    int i;
    lst_head = (listheader_struct*)(pg->ptr);

    for(i = 0; i < 10; i++) {
      ((*lst_head).fl[i]).buff = 0;
      ((*lst_head).fl[i]).size = 1 << (i + 4);
    }

    (*lst_head).page_cnt = 0;
    (*lst_head).alloc_cnt = 0;
    (*lst_head).free_mem = (void*)((long int)lst_head+sizeof(listheader_struct));

    g_ptr=pg;
  }

  main_pg = (listheader_struct*)(g_ptr->ptr);

  // Try the free list first
  int fl = 0;
  listheader_struct* fl_main_pg=(listheader_struct*)(g_ptr->ptr);
  listheader_struct* fl_tmp_pg;
  int fl_indx;
  int j;

  // Round up the size, store in size_rounded
  kma_size_t fl_s = 16;
  fl_indx = 4;
  while(size > fl_s) { fl_s = fl_s << 1; }
  int fl_size_rounded = fl_s;

  while((1 << fl_indx) != fl_size_rounded) { fl_indx++; }

  fl_indx = fl_indx - 4;
  for(j = 0; j <= (*fl_main_pg).page_cnt; j++) {
    fl_tmp_pg = (listheader_struct*)((long int) fl_main_pg+j*PAGESIZE);
    if(((*fl_tmp_pg).fl[fl_indx]).buff != 0) {
      fl = j+1;
    }
  }

  // If fl > 1, then we have space in our free list
  if((i = fl)) {
    int indx = 4;
    i--;

    // Round up the size, store in size_rounded
    kma_size_t s = 16;
    while(size > s) { s = s << 1; }
    int size_rounded = s;

    while((1 << indx) != size_rounded) { indx++; }

    indx = indx - 4;
    tmp_pg = (listheader_struct*)((long int)main_pg + i * PAGESIZE);
    result = (void*)(((*tmp_pg).fl[indx]).buff);
    ((*tmp_pg).fl[i]).buff = (buffer_struct*)((*((buffer_struct*)result)).next_buff);
    (*tmp_pg).alloc_cnt++;
    (*main_pg).alloc_cnt++;
    return result;
  }

  else {
    // Check our free space

    int fs = 0;
    listheader_struct* fs_main_pg = (listheader_struct*)(g_ptr->ptr);
    listheader_struct* fs_tmp_pg;
    int k;
    for(k = 0; k <= (*fs_main_pg).page_cnt; k++) {
      fs_tmp_pg = (listheader_struct*)((long int)fs_main_pg + k * PAGESIZE);
      if((PAGESIZE + (long int)fs_tmp_pg - (long int)((*fs_tmp_pg).free_mem))/(sizeof(buffer_struct)+size)) {
        fs = k+1;
      }
    }

    // If fs > 1, then we have free space
    if((i = fs)) {
      i--;
      tmp_pg=(listheader_struct*)((long int)main_pg + i * PAGESIZE);
      result = malloc_buff(tmp_pg, size);
      return result;
    }

    else {
      // Initialize entry
      kpage_t* pg = get_page();
      listheader_struct* lst_head;
      *((kpage_t**)pg->ptr) = pg;
      int i;
      lst_head = (listheader_struct*)(pg->ptr);

      for(i = 0; i < 10; i++) {
        ((*lst_head).fl[i]).buff = 0;
        ((*lst_head).fl[i]).size = 1 << (i + 4);
      }

      (*lst_head).page_cnt = 0;
      (*lst_head).alloc_cnt = 0;
      (*lst_head).free_mem = (void*)((long int)lst_head+sizeof(listheader_struct));
      (*main_pg).page_cnt++;
      i = (*main_pg).page_cnt;
      tmp_pg = (listheader_struct*)((long int)main_pg + i * PAGESIZE);
      result = malloc_buff(tmp_pg, size);
      return result;
    }
  }
  return NULL;
}

void
kma_free(void* ptr, kma_size_t size)
{
  buffer_struct* buff;
  void* next_buff;
  freelist_struct* free_list;
  buff = (buffer_struct*)((long int)ptr-sizeof(buffer_struct*));
  free_list = (freelist_struct*)((*buff).next_buff);

  next_buff = ((*free_list).buff);
  (*buff).next_buff = next_buff;

  listheader_struct* main_pg=(listheader_struct*)(g_ptr->ptr);
  listheader_struct* tmp_pg;

  int i;
  i = ((long int)ptr-(long int)main_pg)/PAGESIZE;

  tmp_pg = (listheader_struct*)((long int)main_pg + i * PAGESIZE);
  (*tmp_pg).alloc_cnt--;
  (*main_pg).alloc_cnt--;

  if ((*tmp_pg).alloc_cnt==0) {
    kpage_t* pg0;
    pg0 = *(kpage_t**)(tmp_pg);
    // Initialize pg0
    listheader_struct* lst_head;
    *((kpage_t**)pg0->ptr) = pg0;
    int i;
    lst_head = (listheader_struct*)(pg0->ptr);
    for(i = 0; i < 10; i++) {
    ((*lst_head).fl[i]).buff = 0;
    ((*lst_head).fl[i]).size = 1 << (i + 4);
    }
    (*lst_head).page_cnt = 0;
    (*lst_head).alloc_cnt = 0;
    (*lst_head).free_mem = (void*)((long int)lst_head+sizeof(listheader_struct));
  }

  if((*main_pg).alloc_cnt==0){
    // We have no more space
    kpage_t* pg1;
    pg1 = *(kpage_t**)(main_pg);
    for(; (*main_pg).page_cnt > 0; (*main_pg).page_cnt--) {
      // Free all the things
      pg1 = *((kpage_t**)((long int)main_pg + (*main_pg).page_cnt * PAGESIZE));
      free_page(pg1);
    }

    pg1 = *((kpage_t**)((long int)main_pg + (*main_pg).page_cnt * PAGESIZE));
    free_page(pg1);

    // Clear ptr
    g_ptr = 0;
  }
}

void* malloc_buff(listheader_struct* pg, kma_size_t size){
  void* result;
  int i = 4;
  buffer_struct* buff;
  freelist_struct* free_list;
  listheader_struct* main_pg=(listheader_struct*)(g_ptr->ptr);

  // Round up the size, store in size_rounded
  kma_size_t s = 16;
  while(size > s) { s = s << 1; }
  int size_rounded = s;
  while((1 << i) != size_rounded) { i++; }
  i -= 4;

  buff = (buffer_struct*)((*pg).free_mem);
  free_list=(freelist_struct*)&((*main_pg).fl[i]);
  result = sizeof(buffer_struct)+(void*)buff;
  (*buff).next_buff=(void*)free_list;
  (*main_pg).alloc_cnt++;
  (*pg).alloc_cnt++;
  (*pg).free_mem = sizeof(buffer_struct)+(void*)buff+size;
  return result;
}

#endif // KMA_P2FL
