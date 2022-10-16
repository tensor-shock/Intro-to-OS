#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int vpn_mask = 0xfffff000;

char *inputfile;
int frames;
char *method;
char read_write;
unsigned int v_address;
int verb=0;
int *page_table;
int *valid;
int *dirty;
int num_mem_access=0;
int misses=0;
int writes=0;
int drops=0;
int *record;
char *record_rw;


void print_page_table()
{
    for(int i=0;i<frames;i++)
    {
        printf("entry %d is %x\n",i,page_table[i]);
    }
}

void print_summary()
{
    printf("Number of memory accesses: %d\n",num_mem_access);
    printf("Number of misses: %d\n",misses);
    printf("Number of writes: %d\n",writes);
    printf("Number of drops: %d\n",drops);
}

void opt_str()
{
    FILE* fptr = fopen(inputfile, "r");
    if (fptr == NULL) {
        printf("File Error 404");
        return;
    }

    const int MAX=10000005;
    record=(int*)malloc(MAX*sizeof(int));
    record_rw=(char*)malloc(MAX*sizeof(char));
    int ctr=0;
    int assigned=0;
    int *oracle=malloc(frames*sizeof(int));
    

    while(fscanf(fptr, "%x %c", &v_address, &read_write)>0)
    {
        int vpn=(v_address & vpn_mask)>>12;
        record[ctr]=vpn;
        record_rw[ctr]=read_write;
        ctr++;
    }

    for(int i=0;i<ctr;i++)
    {
        // sanity
        // printf("%x %c\n",v_address,read_write);

        int present=0;
        num_mem_access++;
        for(int j=0;j<frames;j++)
        {
            if(valid[j]!=0)
            {
                if(page_table[j]==record[i])
                {
                    //hit
                    present=1;
                    if(record_rw[i]=='W')
                    dirty[j]=1;
                    break;
                }
            }
        }

        //miss->page replacement
        if(!present && assigned<frames)
        {
            misses++;
            page_table[assigned]=record[i];
            valid[assigned]=1;
            assigned++;
        }
        else if(!present)
        {
            misses++;

            int replaced_index=0;
            int next_max=0;

            for(int ind=0;ind<frames;ind++)
            {
                int vpnsearch=page_table[ind];
                oracle[ind]=MAX+3;
                for(int p=i+1;p<ctr;p++)
                {
                    if(record[p]==vpnsearch)
                    {
                        oracle[ind]=p;
                        break;
                    }
                }
            }
            next_max=oracle[0];
            for(int ind=0;ind<frames;ind++)
            {
                if(oracle[ind]>next_max)
                {
                    next_max=oracle[ind];
                    replaced_index=ind;
                }
            }

            int replaced=page_table[replaced_index];


            if(verb)
            {
                printf("Page 0x%x was read from disk, ",record[i]);
            }
            if(dirty[replaced_index]==0)
            {
                drops++;
                if(verb)
                {
                    printf("page 0x%x was dropped (it was not dirty).\n",replaced);
                }
            }
            else
            {
                writes++;
                if(verb)
                {
                    printf("page 0x%x was written to the disk.\n",replaced);
                }
            }
            page_table[replaced_index]=record[i];
            valid[replaced_index]=1;
            dirty[replaced_index]=0;
            if(record_rw[i]=='W')
            dirty[replaced_index]=1;
        }

        // sanity
        // printf("page table is \n");
        // print_page_table();
        // printf("\n");

    }
    
    print_summary();
}
void fifo_str()
{
    
    FILE* fptr = fopen(inputfile, "r");
    if (fptr == NULL) {
        printf("File Error 404");
        return;
    }

    int fifo_start_index=0;

    while(fscanf(fptr, "%x %c", &v_address, &read_write)>0)
    {
        // sanity
        // printf("%x %c\n",v_address,read_write);

        int vpn=(v_address & vpn_mask)>>12;
        int present=0;
        num_mem_access++;
        for(int i=0;i<frames;i++)
        {
            if(valid[i]!=0)
            {
                if(page_table[i]==vpn)
                {
                    //hit
                    present=1;
                    if(read_write=='W')
                    dirty[i]=1;
                    break;
                }
            }
        }

        //miss->page replacement
        if(!present)
        {
            misses++;
            int replaced=page_table[fifo_start_index];
            page_table[fifo_start_index]=vpn;


            if(verb && valid[fifo_start_index])
            {
                printf("Page 0x%x was read from disk, ",vpn);
            }
            if(dirty[fifo_start_index]==0 && valid[fifo_start_index])
            {
                drops++;
                if(verb)
                {
                    printf("page 0x%x was dropped (it was not dirty).\n",replaced);
                }
            }
            else if(valid[fifo_start_index])
            {
                writes++;
                if(verb)
                {
                    printf("page 0x%x was written to the disk.\n",replaced);
                }
            }

            valid[fifo_start_index]=1;
            dirty[fifo_start_index]=0;
            if(read_write=='W')
            dirty[fifo_start_index]=1;

            fifo_start_index+=1;
            fifo_start_index%=frames;
        }

        // sanity
        // printf("page table is \n");
        // print_page_table();
        // printf("\n");

    }
    print_summary();
    

}
void clock_str()
{
    FILE* fptr = fopen(inputfile, "r");
    if (fptr == NULL) {
        printf("File Error 404");
        return;
    }

    int assigned=0;
    int *reference = malloc(frames*sizeof(int));
    int clock_hand=0;
    for(int i=0;i<frames;i++)
    {
        reference[i]=0;
    }

    while(fscanf(fptr, "%x %c", &v_address, &read_write)>0)
    {
        // sanity
        // printf("%x %c\n",v_address,read_write);
        
        int vpn=(v_address & vpn_mask)>>12;
        int present=0;
        num_mem_access++;
        for(int i=0;i<frames;i++)
        {
            if(valid[i]!=0)
            {
                if(page_table[i]==vpn)
                {
                    //hit
                    present=1;
                    if(read_write=='W')
                    dirty[i]=1;
                    reference[i]=1;
                    break;
                }
            }
        }

        //miss->page replacement
        if(!present && assigned<frames)
        {
            misses++;
            page_table[assigned]=vpn;
            valid[assigned]=1;
            reference[assigned]=1;
            assigned++;
        }
        else if(!present)
        {
            misses++;

            int replaced_index;
            int got_index=0;
            
            for(int i=0;i<=frames;i++)
            {
                if(reference[clock_hand]==0)
                {
                    replaced_index=clock_hand;
                    got_index=1;
                }
                else{
                    reference[clock_hand]=0;
                }
                clock_hand+=1;
                clock_hand%=frames;
                if(got_index==1)
                break;
            }
            int replaced=page_table[replaced_index];


            if(verb)
            {
                printf("Page 0x%x was read from disk, ",vpn);
            }
            if(dirty[replaced_index]==0)
            {
                drops++;
                if(verb)
                {
                    printf("page 0x%x was dropped (it was not dirty).\n",replaced);
                }
            }
            else
            {
                writes++;
                if(verb)
                {
                    printf("page 0x%x was written to the disk.\n",replaced);
                }
            }
            page_table[replaced_index]=vpn;
            reference[replaced_index]=1;
            valid[replaced_index]=1;
            dirty[replaced_index]=0;
            if(read_write=='W')
            dirty[replaced_index]=1;
        }

        // sanity
        // printf("page table is \n");
        // print_page_table();
        // printf("\n");

    }
    print_summary();
}
void lru_str()
{
    FILE* fptr = fopen(inputfile, "r");
    if (fptr == NULL) {
        printf("File Error 404");
        return;
    }

    int assigned=0;
    int counts=0;
    int *recent_array = malloc(frames*sizeof(int));
    for(int i=0;i<frames;i++)
    {
        recent_array[i]=-1;
    }

    while(fscanf(fptr, "%x %c", &v_address, &read_write)>0)
    {
        // sanity
        // printf("%x %c\n",v_address,read_write);
        
        int vpn=(v_address & vpn_mask)>>12;
        int present=0;
        num_mem_access++;
        for(int i=0;i<frames;i++)
        {
            if(valid[i]!=0)
            {
                if(page_table[i]==vpn)
                {
                    //hit
                    present=1;
                    if(read_write=='W')
                    dirty[i]=1;
                    recent_array[i]=counts;
                    counts++;
                    break;
                }
            }
        }

        //miss->page replacement
        if(!present && assigned<frames)
        {
            misses++;
            page_table[assigned]=vpn;
            valid[assigned]=1;
            recent_array[assigned]=counts;
            assigned++;
            counts++;
        }
        else if(!present)
        {
            misses++;

            int replaced_index = 0;
            
            for(int i=1;i<frames;i++)
            {
                if(recent_array[i]<recent_array[replaced_index])
                {
                    replaced_index=i;
                }
            }
            int replaced=page_table[replaced_index];


            if(verb)
            {
                printf("Page 0x%x was read from disk, ",vpn);
            }
            if(dirty[replaced_index]==0)
            {
                drops++;
                if(verb)
                {
                    printf("page 0x%x was dropped (it was not dirty).\n",replaced);
                }
            }
            else
            {
                writes++;
                if(verb)
                {
                    printf("page 0x%x was written to the disk.\n",replaced);
                }
            }
            page_table[replaced_index]=vpn;
            recent_array[replaced_index]=counts;
            valid[replaced_index]=1;
            dirty[replaced_index]=0;
            if(read_write=='W')
            dirty[replaced_index]=1;

            counts++;
        }

        // sanity
        // printf("page table is \n");
        // print_page_table();
        // printf("\n");

    }
    print_summary();
}
void random_str()
{
    FILE* fptr = fopen(inputfile, "r");
    if (fptr == NULL) {
        printf("File Error 404");
        return;
    }

    srand(5635);
    int assigned=0;
    while(fscanf(fptr, "%x %c", &v_address, &read_write)>0)
    {
        // sanity
        // printf("%x %c\n",v_address,read_write);
        
        int vpn=(v_address & vpn_mask)>>12;
        int present=0;
        num_mem_access++;
        for(int i=0;i<frames;i++)
        {
            if(valid[i]!=0)
            {
                if(page_table[i]==vpn)
                {
                    //hit
                    present=1;
                    if(read_write=='W')
                    dirty[i]=1;
                    break;
                }
            }
        }

        //miss->page replacement
        if(!present && assigned<frames)
        {
            misses++;
            page_table[assigned]=vpn;
            valid[assigned]=1;
            assigned++;
        }
        else if(!present)
        {
            misses++;

            int replaced_index = rand() % frames;
            int replaced=page_table[replaced_index];


            if(verb)
            {
                printf("Page 0x%x was read from disk, ",vpn);
            }
            if(dirty[replaced_index]==0)
            {
                drops++;
                if(verb)
                {
                    printf("page 0x%x was dropped (it was not dirty).\n",replaced);
                }
            }
            else
            {
                writes++;
                if(verb)
                {
                    printf("page 0x%x was written to the disk.\n",replaced);
                }
            }
            page_table[replaced_index]=vpn;
            valid[replaced_index]=1;
            dirty[replaced_index]=0;
            if(read_write=='W')
            dirty[replaced_index]=1;
        }

        // sanity
        // printf("page table is \n");
        // print_page_table();
        // printf("\n");

    }
    print_summary();

}


void do_strategy(char *method)
{
    if(!strcmp(method,"OPT"))
    {
        opt_str();
    }
    else if(!strcmp(method,"FIFO"))
    {
        fifo_str();
    }
    else if(!strcmp(method,"CLOCK"))
    {
        clock_str();
    }
    else if(!strcmp(method,"LRU"))
    {
        lru_str();
    }
    else
    {
        random_str();
    }
}



int main(int argc,char **argv)
{
    //taking input
    inputfile=argv[1];
    frames=atoi(argv[2]);
    method=argv[3];
    if(argc==5 && strcmp(argv[4],"-verbose")==0)
    {
        verb=1;
    }

    page_table=malloc(frames*sizeof(int));
    valid=malloc(frames*sizeof(int));
    dirty=malloc(frames*sizeof(int));
    for(int i=0;i<frames;i++)
    {
        valid[i]=0;
        dirty[i]=0;
    }

    //doing strategy
    do_strategy(method);

    return 0;
}