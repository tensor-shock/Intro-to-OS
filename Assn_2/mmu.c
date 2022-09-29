#include "mmu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MB (1024 * 1024)

#define KB (1024)
#define frame_start 18432
#define pfn_base 60

// byte addressable memory
unsigned char RAM[RAM_SIZE];  


// OS's memory starts at the beginning of RAM.
// Store the process related info, page tables or other data structures here.
// do not use more than (OS_MEM_SIZE: 72 MB).
unsigned char* OS_MEM = RAM;  

// memory that can be used by processes.   
// 128 MB size (RAM_SIZE - OS_MEM_SIZE)
unsigned char* PS_MEM = RAM + OS_MEM_SIZE; 


// This first frame has frame number 0 and is located at start of RAM(NOT PS_MEM).
// We also include the OS_MEM even though it is not paged. This is 
// because the RAM can only be accessed through physical RAM addresses.  
// The OS should ensure that it does not map any of the frames, that correspond
// to its memory, to any process's page. 
int NUM_FRAMES = ((RAM_SIZE) / PAGE_SIZE); //=200 MB/4 KB=51200

// Actual number of usable frames by the processes.
int NUM_USABLE_FRAMES = ((RAM_SIZE - OS_MEM_SIZE) / PAGE_SIZE); //=128 MB/ 4 KB =32768

// PHYSICAL FRAME NUMBERS
// 0 TO 18431 is for OS ->total 18432
// 18432 to 51199 is for PS ->total 32768

// To be set in case of errors. 
int error_no; 


void os_init() {
    // TODO student 
    // initialize your data structures.
    for(int index=0;index<RAM_SIZE;index++)
    {
        RAM[index]=0;
    }
}


// ---------------------- Helper functions for Page table entries ------------------ // 

// return the frame number from the pte
int pte_to_frame_num(page_table_entry pte) 
{
    // TODO: student
    //only 15 bits needed for PFN values
    return pte&0x00ffffff;
}

int vmem_addr_to_page_num(int vmem_addr)
{
    return vmem_addr>>12;
}

int vmem_addr_to_offset(int vmem_addr)
{
    return vmem_addr&0x00000fff;
}

// return 1 if read bit is set in the pte
// 0 otherwise
int is_readable(page_table_entry pte) {
    // TODO: student
    return (pte & (1<<28))>>28;
}

// return 1 if write bit is set in the pte
// 0 otherwise
int is_writeable(page_table_entry pte) {
    // TODO: student
    return (pte & (1<<29))>>29; 
}

// return 1 if executable bit is set in the pte
// 0 otherwise
int is_executable(page_table_entry pte) {
    // TODO: student
    return (pte & (1<<30))>>30;
}

// return 1 if present bit is set in the pte
// 0 otherwise
int is_present(page_table_entry pte) {
    // TODO: student
    return (pte & (1<<27))>>27;
}



// ----------------------------------- Functions for managing memory --------------------------------- //

/**
 *  Process Virtual Memory layout: 
 *  ---------------------- (virt. memory start 0x00)
 *        code
 *  ----------------------  
 *     read only data 
 *  ----------------------
 *     read / write data
 *  ----------------------
 *        heap
 *  ----------------------
 *        stack  
 *  ----------------------  (virt. memory end 0x3fffff)
 * 
 * 
 *  code            : read + execute only
 *  ro_data         : read only
 *  rw_data         : read + write only
 *  stack           : read + write only
 *  heap            : (protection bits can be different for each heap page)
 * 
 *  assume:
 *  code_size, ro_data_size, rw_data_size, max_stack_size, are all in bytes
 *  code_size, ro_data_size, rw_data_size, max_stack_size, are all multiples of PAGE_SIZE
 *  code_size + ro_data_size + rw_data_size + max_stack_size < PS_VIRTUAL_MEM_SIZE
 *  
 * 
 *  The rest of memory will be used dynamically for the heap.
 * 
 *  This function should create a new process, 
 *  allocate code_size + ro_data_size + rw_data_size + max_stack_size amount of physical memory in PS_MEM,
 *  and create the page table for this process. Then it should copy the code and read only data from the
 *  given `unsigned char* code_and_ro_data` into processes' memory.
 *   
 *  It should return the pid of the new process.  
 *  
 */
int create_ps(int code_size, int ro_data_size, int rw_data_size,int max_stack_size, unsigned char* code_and_ro_data) 
{   
    // TODO student 
    
    //assign pid
    int process_num;
    for(process_num=1;process_num<=100;process_num++)
    {
        if(RAM[process_num]==0)
        break;
    }
    RAM[process_num]=1;


    unsigned int arr[1024];
    // memset(arr, 0, sizeof(arr));
    for(int i=0;i<1024;i++)
    {
        arr[i]=0;
    }
    

    //RAM 1 to 100 check if pid allocated
    //RAM 200 to 32767+200 for pfn free list
    //RAM page table for pid i from 40KB+10KB*pid 
    // pfn numbers from 0 to 32767

    unsigned int last_pfn_checked=frame_start;
    int data_pointer=0;
    int vpn=0;

    int code_frames=code_size/PAGE_SIZE;
    for(int i=0;i<code_frames;i++)
    {
        unsigned char temp[4*KB];
        int local_frame_data_pointer=0;
        for(;local_frame_data_pointer<4*KB;local_frame_data_pointer++)
        {
            temp[local_frame_data_pointer]=code_and_ro_data[local_frame_data_pointer+data_pointer];
        }
        data_pointer+=4*KB;
        for(;last_pfn_checked<NUM_FRAMES;last_pfn_checked++)
        {
            if(RAM[200+last_pfn_checked]==0)
            {
                //assign pfn 
                
                RAM[200+last_pfn_checked]=1;

                // put in page table 
                arr[vpn]=last_pfn_checked;
                

                //assign value
                memcpy( &OS_MEM[PAGE_SIZE*last_pfn_checked], &temp, sizeof(temp));

                //assign read + execute
                arr[vpn] |= (1<<28);
                arr[vpn] |= (1<<30);
                arr[vpn] |= (1<<27);
                

                break;
            }
            
        }
        
        vpn++;
    }

    int ro_frames=ro_data_size/PAGE_SIZE;
    
    for(int i=0;i<ro_frames;i++)
    {
        unsigned char temp[4*KB];
        int local_frame_data_pointer=0;
        for(;local_frame_data_pointer<4*KB;local_frame_data_pointer++)
        {
            temp[local_frame_data_pointer]=code_and_ro_data[local_frame_data_pointer+data_pointer];
        }
        data_pointer+=4*KB;
        for(;last_pfn_checked<NUM_FRAMES;last_pfn_checked++)
        {
            if(RAM[200+last_pfn_checked]==0)
            {
                //assign pfn 
                
                RAM[200+last_pfn_checked]=1;

                // put in page table 
                arr[vpn]=last_pfn_checked;
                //assign value

                memcpy( &OS_MEM[PAGE_SIZE*last_pfn_checked], &temp, sizeof(temp));

                //assign read 
                arr[vpn] |= (1<<28);
                arr[vpn] |= (1<<27);
                break;
            }
        }
        vpn++;
    }
    

    int rw_frames=rw_data_size/PAGE_SIZE;
    for(int i=0;i<rw_frames;i++)
    {
        for(;last_pfn_checked<NUM_FRAMES;last_pfn_checked++)
        {
            if(RAM[200+last_pfn_checked]==0)
            {
                //assign pfn 
                RAM[200+last_pfn_checked]=1;

                // put in page table 
                arr[vpn]=last_pfn_checked;

                //assign read + write
                arr[vpn] |= (1<<28);
                arr[vpn] |= (1<<29);
                arr[vpn] |= (1<<27);

                break;
            }
        }
        vpn++;
    }

    int stack_frames=max_stack_size/PAGE_SIZE;
    vpn=1023;
    for(int i=0;i<stack_frames;i++)
    {
        for(;last_pfn_checked<NUM_FRAMES;last_pfn_checked++)
        {
            if(RAM[200+last_pfn_checked]==0)
            {
                //assign pfn 
                RAM[200+last_pfn_checked]=1;

                // put in page table 
                arr[vpn]=last_pfn_checked;

                //assign read + write
                arr[vpn] |= (1<<28);
                arr[vpn] |= (1<<29);
                arr[vpn] |= (1<<27);


                break;
            }
        }
        vpn--;
    }
    
    memcpy( &OS_MEM[pfn_base * KB + process_num * 10 * KB], &arr, sizeof(arr));

    
    return process_num;
}

/**
 * This function should deallocate all the resources for this process. 
 * 
 */
void exit_ps(int pid) 
{
   // TODO student
    RAM[pid]=0;
    page_table_entry* page_table_start = (page_table_entry*) (&OS_MEM[pfn_base * KB + pid * 10 * KB]) ;
    for(int i=0;i<KB;i++)
    {
        if(is_present(page_table_start[i]))
        {
            RAM[200+pte_to_frame_num(page_table_start[i])]=0;
            page_table_start[i]=0;
        }
    }
    return;
}

/**
 * Create a new process that is identical to the process with given pid. 
 * 
 */
int fork_ps(int pid) {

    // TODO student:
    page_table_entry* page_table_start = (page_table_entry*) (&OS_MEM[pfn_base * KB + pid * 10 * KB]) ;

    int process_num;
    for(process_num=1;process_num<=100;process_num++)
    {
        if(RAM[process_num]==0)
        break;
    }
    RAM[process_num]=1;

    unsigned int arr[KB];
    for(int i=0;i<KB;i++)
    {
        arr[i]=0;
    }
    int last_pfn_checked=frame_start;
    for(int i=0;i<KB;i++)
    {
        if(is_present(page_table_start[i]))
        {
            
            for(;last_pfn_checked<NUM_FRAMES;last_pfn_checked++)
            {
                if(RAM[200+last_pfn_checked]==0)
                {
                    //assign pfn 
                    RAM[200+last_pfn_checked]=1;

                    // put in page table 
                    arr[i]=last_pfn_checked;

                    //assign protections
                    // R
                    if(is_readable(page_table_start[i]))
                    arr[i] |= (1<<28);
                    // W
                    if((is_writeable(page_table_start[i])))
                    arr[i] |= (1<<29);
                    // E
                    if(is_executable(page_table_start[i]))
                    arr[i] |= (1<<30);
                    // P
                    arr[i] |= (1<<27);


                    break;
                }
            }
            //copy data
            unsigned char temp[4*KB];
            int local_frame_data_pointer=0;
            for(;local_frame_data_pointer<4*KB;local_frame_data_pointer++)
            {
                temp[local_frame_data_pointer]=OS_MEM[PAGE_SIZE* pte_to_frame_num(page_table_start[i])+local_frame_data_pointer];
            }
            memcpy( &OS_MEM[PAGE_SIZE*last_pfn_checked], &temp, sizeof(temp));
        }
    }

    memcpy( &OS_MEM[pfn_base * KB + process_num * 10 * KB], &arr, sizeof(arr));

    return process_num;
}

// dynamic heap allocation
//
// Allocate num_pages amount of pages for process pid, starting at vmem_addr.
// Assume vmem_addr points to a page boundary.  
// Assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE
//
//
// Use flags to set the protection bits of the pages.
// Ex: flags = O_READ | O_WRITE => page should be read & writeable.
//
// If any of the pages was already allocated then kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
void allocate_pages(int pid, int vmem_addr, int num_pages, int flags) 
{
   // TODO student
    
    page_table_entry* page_table_start = (page_table_entry*) (&OS_MEM[pfn_base * KB + pid * 10 * KB]) ;

    int page_number=vmem_addr_to_page_num(vmem_addr);
    for(int i=0;i<num_pages;i++)
    {
        int page_index=page_number+i;
        if(is_present(page_table_start[page_index]))
        {
            exit_ps(pid);
            error_no=ERR_SEG_FAULT;
            return;
        }
        int last_pfn_checked=frame_start;
        for(;last_pfn_checked<NUM_FRAMES;last_pfn_checked++)
        {
            if(RAM[200+last_pfn_checked]==0)
            {
                //assign pfn 
                
                RAM[200+last_pfn_checked]=1;

                // put in page table 
                page_table_start[page_index]=last_pfn_checked;


                //assign protections
                // R
                if((flags&1)!=0)
                page_table_start[page_index] |= (1<<28);
                // W
                if((flags&2)!=0)
                page_table_start[page_index] |= (1<<29);
                // E
                if((flags&4)!=0)
                page_table_start[page_index] |= (1<<30);
                // P
                page_table_start[page_index] |= (1<<27);



                break;
            }
        }
    }
}

// dynamic heap deallocation
//
// Deallocate num_pages amount of pages for process pid, starting at vmem_addr.
// Assume vmem_addr points to a page boundary
// Assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE
// If any of the pages was not already allocated then kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
void deallocate_pages(int pid, int vmem_addr, int num_pages) 
{
   // TODO student
    
    page_table_entry* page_table_start = (page_table_entry*) (&OS_MEM[pfn_base * KB + pid * 10 * KB]) ;

    int page_number=vmem_addr_to_page_num(vmem_addr);
    for(int i=0;i<num_pages;i++)
    {
        int page_index=page_number+i;
        // printf("check page number %d\n",page_index);
        if(is_present(page_table_start[page_index]))
        continue;
        else
        {
            exit_ps(pid);
            error_no=ERR_SEG_FAULT;
            return;
        }
    }
    for(int i=0;i<num_pages;i++)
    {
        int page_index=page_number+i;
        RAM[200+pte_to_frame_num(page_table_start[page_index])]=0;
        page_table_start[page_index]=0;
    }

}

// Read the byte at `vmem_addr` virtual address of the process
// In case of illegal memory access kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
// 
// assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE
unsigned char read_mem(int pid, int vmem_addr) 
{
    // TODO: student
    page_table_entry* page_table_start = (page_table_entry*) (&OS_MEM[pfn_base * KB + pid * 10 * KB]) ;

    int page_number=vmem_addr_to_page_num(vmem_addr);
    int offset=vmem_addr_to_offset(vmem_addr);
    if(is_present(page_table_start[page_number]) && is_readable(page_table_start[page_number]))
    {
        int pfn=pte_to_frame_num(page_table_start[page_number]);
        int ps_phys_add=PAGE_SIZE * pfn + offset;
        return OS_MEM[ps_phys_add];
    }
    else{
        exit_ps(pid);
        error_no=ERR_SEG_FAULT;
    }

    return 0;
}

// Write the given `byte` at `vmem_addr` virtual address of the process
// In case of illegal memory access kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
// 
// assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE
void write_mem(int pid, int vmem_addr, unsigned char byte) 
{
    // TODO: student
    page_table_entry* page_table_start = (page_table_entry*) (&OS_MEM[pfn_base * KB + pid * 10 * KB]) ;

    int page_number=vmem_addr_to_page_num(vmem_addr);
    int offset=vmem_addr_to_offset(vmem_addr);

    if(is_present(page_table_start[page_number]) && is_writeable(page_table_start[page_number]))
    {
        int pfn=pte_to_frame_num(page_table_start[page_number]);
        int ps_phys_add=PAGE_SIZE * pfn + offset;
        OS_MEM[ps_phys_add]=byte;
        memcpy( &OS_MEM[ps_phys_add], &byte, sizeof(byte));
    }
    else{
        exit_ps(pid);
        error_no=ERR_SEG_FAULT;
    }
    return ;
}



// // -------------------  functions to print the state  --------------------------------------------- //

void print_page_table(int pid) 
{
    
    page_table_entry* page_table_start = (page_table_entry*) (&OS_MEM[pfn_base * KB + pid * 10 * KB]) ; // TODO student: start of page table of process pid
    int num_page_table_entries = KB;           // TODO student: num of page table entries


    // Do not change anything below
    puts("------ Printing page table-------");
    for (int i = 0; i < num_page_table_entries; i++) 
    {
        page_table_entry pte = page_table_start[i];
        
        printf("Page num: %d, frame num: %d, R:%d, W:%d, X:%d, P%d\n", 
                i, 
                pte_to_frame_num(pte),
                is_readable(pte),
                is_writeable(pte),
                is_executable(pte),
                is_present(pte)
                );
    }

}

// just a random array to be passed to ps_create
unsigned char code_ro_data[10 * MB];


int main() {

	os_init();
    
	code_ro_data[10 * PAGE_SIZE] = 'c';   // write 'c' at first byte in ro_mem
	code_ro_data[10 * PAGE_SIZE + 1] = 'd'; // write 'd' at second byte in ro_mem

	int p1 = create_ps(10 * PAGE_SIZE, 1 * PAGE_SIZE, 2 * PAGE_SIZE, 1 * MB, code_ro_data);

	error_no = -1; // no error


    
	unsigned char c = read_mem(p1, 10 * PAGE_SIZE);

	assert(c == 'c');

	unsigned char d = read_mem(p1, 10 * PAGE_SIZE + 1);
	assert(d == 'd');

	assert(error_no == -1); // no error


	write_mem(p1, 10 * PAGE_SIZE, 'd');   // write at ro_data

	assert(error_no == ERR_SEG_FAULT);  


	int p2 = create_ps(1 * MB, 0, 0, 1 * MB, code_ro_data);	// no ro_data, no rw_data

	error_no = -1; // no error


	int HEAP_BEGIN = 1 * MB;  // beginning of heap

	// allocate 250 pages
	allocate_pages(p2, HEAP_BEGIN, 250, O_READ | O_WRITE);

	write_mem(p2, HEAP_BEGIN + 1, 'c');

	write_mem(p2, HEAP_BEGIN + 2, 'd');

	assert(read_mem(p2, HEAP_BEGIN + 1) == 'c');

	assert(read_mem(p2, HEAP_BEGIN + 2) == 'd');

	deallocate_pages(p2, HEAP_BEGIN, 10);

	print_page_table(p2); // output should atleast indicate correct protection bits for the vmem of p2.

	write_mem(p2, HEAP_BEGIN + 1, 'd'); // we deallocated first 10 pages after heap_begin

	assert(error_no == ERR_SEG_FAULT);


	int ps_pids[100];

	// requesting 2 MB memory for 64 processes, should fill the complete 128 MB without complaining.   
	for (int i = 0; i < 64; i++) {
    	ps_pids[i] = create_ps(1 * MB, 0, 0, 1 * MB, code_ro_data);
    	print_page_table(ps_pids[i]);	// should print non overlapping mappings.  
	}


	exit_ps(ps_pids[0]);
    

	ps_pids[0] = create_ps(1 * MB, 0, 0, 500 * KB, code_ro_data);

	print_page_table(ps_pids[0]);   

	// allocate 500 KB more
	allocate_pages(ps_pids[0], 1 * MB, 125, O_READ | O_READ | O_EX);

	for (int i = 0; i < 64; i++) {
    	print_page_table(ps_pids[i]);	// should print non overlapping mappings.  
	}
}



