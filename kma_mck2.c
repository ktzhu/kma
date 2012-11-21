/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the McKusick-Karels
 *              algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_mck2.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_mck2.c,v $
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
#ifdef KMA_MCK2
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
} listheader_struct;

typedef struct {
	void* head;
  listheader_struct fl[10];
  int page_cnt;
  int alloc_cnt;
  int size;
  int empty;
} pageheader_struct;

/************Global Variables*********************************************/

pageheader_struct* g_pg=0;
#define G_MAXSIZE 8192 // Assume requests for space < 8000

/************Function Prototypes******************************************/

buffer_struct* clear_buff(buffer_struct* addr, listheader_struct* fl);
void add_buff(buffer_struct* buff, listheader_struct* fl);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
  // tmp vars to add buffers l8r
  buffer_struct* tmp_buff;
  listheader_struct* tmp_fl;

	if ((sizeof(void*) + size) > PAGESIZE){ // requested size too large
		return NULL;
	}

	if(!g_pg) {
	  // Initialize the page
    kpage_t* init_pg = get_page();
    int i;
    int j = 0;
    int used_pg = 16;
    pageheader_struct* pg_result = (pageheader_struct*)((*init_pg).ptr);
    buffer_struct* buff_tmp;
    buffer_struct* buff_head;
		buffer_struct* buff_tail;
    *((kpage_t**)init_pg->ptr) = init_pg;

    if (g_pg == 0) { g_pg = pg_result; }

    for (i = 0; i < 10; i++) { ((*pg_result).fl[i]).buff = 0; }

    (*pg_result).empty = 0;
    (*pg_result).alloc_cnt = 0;
    (*pg_result).size = used_pg;
    (*pg_result).page_cnt = 0;

    if (used_pg == G_MAXSIZE / 2) {
      tmp_buff = (buffer_struct*)((long int)pg_result + sizeof(pageheader_struct));
      tmp_fl = (&((*g_pg).fl[8]));
      add_buff(tmp_buff, tmp_fl);
    }

    else if (used_pg == G_MAXSIZE) {
      tmp_buff = (buffer_struct*)((long int)pg_result + sizeof(pageheader_struct));
      tmp_fl = (&((*g_pg).fl[9]));
      add_buff(tmp_buff, tmp_fl);
    }

    else {
  		buff_head = (buffer_struct*)((long int)pg_result + sizeof(pageheader_struct));
  		buff_tail = (buffer_struct*)((long int)pg_result + PAGESIZE - used_pg);
  		for(buff_tmp = buff_head; buff_tmp < buff_tail; buff_tmp = (buffer_struct*)((long int)buff_tmp + used_pg)) {
  			(*buff_tmp).next_buff = (void*)((long int)buff_tmp + used_pg);
  		}
  		buff_tmp = (buffer_struct*)((long int)buff_tmp - used_pg);
  		(*buff_tmp).next_buff = 0;

  		while((16 << j) != used_pg) { j++; }
      tmp_fl = &((*g_pg).fl[j]);

  		// Add the last buffer
  		add_buff(buff_tmp, tmp_fl);

  		// Add the first buffer
  		add_buff(buff_head, tmp_fl);

  		// Link all the buffers
  		(*buff_head).next_buff = (void*)((long int)buff_head + used_pg);
    }

		g_pg = pg_result;
	}

  // Round up the size, store in size_rounded
  kma_size_t fl_s = 16;
  while(size > fl_s) { fl_s = fl_s << 1; }
  int size_rounded = fl_s;

	int indx=0;
	while((16 << indx)!=size_rounded) { indx++; }
	listheader_struct* lst;
	lst=&((*g_pg).fl[indx]);

	buffer_struct* result;

  // Try the free list first
  void* fl_ptr;
  // fl_ptr = (void *)(((*g_pg).page_cnt + 1) * PAGESIZE + (long int)g_pg);
  fl_ptr = (void *)((long int)g_pg + ((*g_pg).page_cnt + 1) * PAGESIZE);
  int j = 0;
  while((16 << j) != size_rounded) { j++; }
  buffer_struct* new_buff;
  new_buff = (*g_pg).fl[j].buff;

  if(((void*)new_buff < (void *)fl_ptr) && (new_buff != 0)) {
    j += 1;
  }
  else {
    j = 0;
  }

  int i;
	if((i = j)) {
		pageheader_struct* pg;

    // Unlink the buffer from lst
    buffer_struct* new_buff;
    new_buff = (*lst).buff;
    (*lst).buff = (*new_buff).next_buff;
    result = new_buff;

		pg=(pageheader_struct*)(((long int)(((long int)result-(long int)g_pg)/PAGESIZE))*PAGESIZE + (long int)g_pg);
		(*g_pg).alloc_cnt++;
		(*pg).alloc_cnt++;
		return result;
	}

	else {
  	// Try an empty page
    int acc;
    int is_empty_pg = 0;
    pageheader_struct* tmp_pg;
    for(acc = 0; acc < (*g_pg).page_cnt; acc++) {
      tmp_pg = (pageheader_struct*)(acc * PAGESIZE + (long int)g_pg);
      if((*tmp_pg).empty == 1) {
        is_empty_pg = acc;
        break;
      }
    }

  	if((i = is_empty_pg)){
  		pageheader_struct* curr_pg;
  		curr_pg=(pageheader_struct*)((long int)g_pg + i * PAGESIZE);

  		// Initialize the page
      kpage_t* init_pg = (*curr_pg).head;
      int i;
      int j = 0;
      pageheader_struct* pg_result = (pageheader_struct*)((*init_pg).ptr);
      buffer_struct* buff_tmp;
      buffer_struct* buff_head;
  		buffer_struct* buff_tail;
      *((kpage_t**)init_pg->ptr) = init_pg;

      if (g_pg == 0) { g_pg = pg_result; }

      for (i = 0; i < 10; i++) { ((*pg_result).fl[i]).buff = 0; }

      (*pg_result).empty = 0;
      (*pg_result).alloc_cnt = 0;
      (*pg_result).size = size_rounded;
      (*pg_result).page_cnt = 0;

      tmp_buff = (buffer_struct*)((long int)pg_result + sizeof(pageheader_struct));
      if (size_rounded == G_MAXSIZE / 2) {
        tmp_fl = &((*g_pg).fl[8]);
        add_buff(tmp_buff, tmp_fl);
      }

      else if (size_rounded == G_MAXSIZE) {
        tmp_fl = &((*g_pg).fl[9]);
        add_buff(tmp_buff, tmp_fl);
      }

      else {
    		buff_head = (buffer_struct*)((long int)pg_result + sizeof(pageheader_struct));
    		buff_tail = (buffer_struct*)((long int)pg_result + PAGESIZE - size_rounded);
    		for(buff_tmp = buff_head; buff_tmp < buff_tail; buff_tmp = (buffer_struct*)((long int)buff_tmp + size_rounded)) {
    			(*buff_tmp).next_buff = (void*)((long int)buff_tmp + size_rounded);
    		}
    		buff_tmp = (buffer_struct*)((long int)buff_tmp - size_rounded);
    		(*buff_tmp).next_buff = 0;

    		while((16 << j) != size_rounded) { j++; }

        tmp_fl = &((*g_pg).fl[j]);

    		// Add the last buffer
    		add_buff(buff_tmp, tmp_fl);

    		// Add the first buffer
    		add_buff(buff_head, tmp_fl);

    		// Link all the buffers
    		(*buff_head).next_buff = (void*)((long int)buff_head + size_rounded);
      }

  		curr_pg = pg_result;

  		// Unlink the buffer from lst
  		buffer_struct* new_buff;
      new_buff = (*lst).buff;
      (*lst).buff = (*new_buff).next_buff;
      result = new_buff;

      (*curr_pg).alloc_cnt++;
  		(*g_pg).alloc_cnt++;

  		return result;
  	}
  	else {
    	// Initialize the page
      kpage_t* init_pg = get_page();
      int i;
      int j = 0;
      pageheader_struct* pg_result = (pageheader_struct*)((*init_pg).ptr);
      buffer_struct* buff_tmp;
      buffer_struct* buff_head;
  		buffer_struct* buff_tail;
      *((kpage_t**)init_pg->ptr) = init_pg;

      if (g_pg == 0) { g_pg = pg_result; }

      for (i = 0; i < 10; i++) { ((*pg_result).fl[i]).buff = 0; }

      (*pg_result).empty = 0;
      (*pg_result).alloc_cnt = 0;
      (*pg_result).size = size_rounded;
      (*pg_result).page_cnt = 0;

      tmp_buff = (buffer_struct*)((long int)pg_result + sizeof(pageheader_struct));
      if (size_rounded == G_MAXSIZE / 2) {
        tmp_fl = &((*g_pg).fl[8]);
        add_buff(tmp_buff, tmp_fl);
      }

      else if (size_rounded == G_MAXSIZE) {
        tmp_fl = &((*g_pg).fl[9]);
        add_buff(tmp_buff, tmp_fl);
      }

      else {
    		buff_head = (buffer_struct*)((long int)pg_result + sizeof(pageheader_struct));
    		buff_tail = (buffer_struct*)((long int)pg_result + PAGESIZE - size_rounded);
    		for(buff_tmp = buff_head; buff_tmp < buff_tail; buff_tmp = (buffer_struct*)((long int)buff_tmp + size_rounded)) {
    			(*buff_tmp).next_buff = (void*)((long int)buff_tmp + size_rounded);
    		}
    		buff_tmp = (buffer_struct*)((long int)buff_tmp - size_rounded);
    		(*buff_tmp).next_buff = 0;

    		while((16 << j) != size_rounded) { j++; }
        tmp_fl = &((*g_pg).fl[j]);

    		// Add the last buffer
    		add_buff(buff_tmp, tmp_fl);

    		// Add the first buffer
    		add_buff(buff_head, tmp_fl);

    		// Link all the buffers
    		(*buff_head).next_buff = (void*)((long int)buff_head + size_rounded);
      }

  		pageheader_struct* new_pg = pg_result;
  		(*g_pg).page_cnt++;

  		// Unlink the buffer from lst
  		buffer_struct* new_buff;
      new_buff = (*lst).buff;
      (*lst).buff = (*new_buff).next_buff;
      result = new_buff;

      (*new_pg).alloc_cnt++;
  		(*g_pg).alloc_cnt++;

  		return result;
  	}
  }
  return NULL;
}

void
kma_free(void* ptr, kma_size_t size)
{
  // Round up the size, store in size_rounded
  kma_size_t fl_s = 16;
  while(size > fl_s) { fl_s = fl_s << 1; }
  int size_rounded = fl_s;

	int indx=0;
	while((16<<indx) != size_rounded) {
		indx++;
	}
	listheader_struct* lst;
	pageheader_struct* pg;
	lst=&((*g_pg).fl[indx]);
	pg=(pageheader_struct*)(((long int)(((long int)ptr-(long int)g_pg)/PAGESIZE))*PAGESIZE + (long int)g_pg);

	add_buff((buffer_struct*)ptr, lst);
	(*pg).alloc_cnt--;
	(*g_pg).alloc_cnt--;
	if((*pg).alloc_cnt == 0){
		(*pg).empty = 1;

		// tmps for buffer clearing
    buffer_struct* tmp_fl;
    listheader_struct* tmp_addr;

		if(size_rounded == G_MAXSIZE) {
      tmp_fl = (buffer_struct*)((long int)pg + sizeof(pageheader_struct));
      tmp_addr = &((*g_pg).fl[9]);
			clear_buff(tmp_fl, tmp_addr);
		}
		else if(size_rounded == G_MAXSIZE / 2){
      tmp_fl = (buffer_struct*)((long int)pg + sizeof(pageheader_struct));
      tmp_addr = &((*g_pg).fl[8]);
			clear_buff(tmp_fl, tmp_addr);
		}
		else {
			buffer_struct* buff_head;
			buffer_struct* buff_tail;
			buffer_struct* buff_tmp;
			buff_head = (buffer_struct*)((long int)pg + sizeof(pageheader_struct));
			buff_tail = (buffer_struct*)((long int)pg + PAGESIZE - size_rounded);
      for(buff_tmp = buff_head; buff_tmp < buff_tail; buff_tmp = (buffer_struct*)((long int)buff_tmp + size_rounded))
			{
				//(*tempbuf).nextbuffer = (void*)((long int)tempbuf + roundsize);
			}
			buff_tmp = (buffer_struct*)((long int)buff_tmp - size_rounded);
      tmp_addr = &((*g_pg).fl[indx]);
			clear_buff(buff_tmp, tmp_addr);
			(*buff_head).next_buff = (*buff_tmp).next_buff;
			clear_buff(buff_head, tmp_addr);
		}
	}

	pageheader_struct* pg_end;
	pg_end=(pageheader_struct*)((long int)g_pg + ((*g_pg).page_cnt) * PAGESIZE);
	if(pg == pg_end)
	{
		while((*pg_end).alloc_cnt==0){
			(*g_pg).page_cnt--;
			free_page((*pg_end).head);
			if(g_pg == pg_end){
				g_pg = 0;
				break;
			}
			pg_end = (pageheader_struct*)((long int)g_pg + ((*g_pg).page_cnt) * PAGESIZE);
		}
	}
}

/* clear_buff */
buffer_struct* clear_buff(buffer_struct* addr, listheader_struct* fl){
  void* buff_tmp;
  buff_tmp=(*fl).buff;
	buffer_struct* result = 0;

	if(addr==buff_tmp) {
		result = addr;
		(*fl).buff=(*result).next_buff;
		return result;
	}

	while(buff_tmp) {
		if((*(buffer_struct*)buff_tmp).next_buff == addr) {
			result = addr;
			(*(buffer_struct*)buff_tmp).next_buff = (*result).next_buff;
			return result;
		}
		buff_tmp = (*(buffer_struct*)buff_tmp).next_buff;
	}
	return 0;
} /* clear_buff */

/* add_buff */
void add_buff (buffer_struct* buff, listheader_struct* fl) {
  if ((*fl).buff == 0) {
    (*fl).buff = buff;
    (*buff).next_buff = 0;
  }

  else if (buff < (*fl).buff) {
    (*buff).next_buff = (*fl).buff;
    (*fl).buff = buff;
  }

  else {
    void* next = (*fl).buff;
    buffer_struct* tmp;
    while ((*(buffer_struct*)next).next_buff) {
      if((*(buffer_struct*)next).next_buff > (void*)buff) {
        break;
      }
      next = (*(buffer_struct*)next).next_buff;
    }
    tmp = (*(buffer_struct*)next).next_buff;
    (*(buffer_struct*)next).next_buff = buff;
    (*buff).next_buff = tmp;
  }
} /* add_buff */

#endif // KMA_MCK2
