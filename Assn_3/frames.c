#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define offset_mask 0x111;
#define vpn_mask 0x11111000;

char *inputfile;
int frames;
char *method;
char read_write;
unsigned int v_address;
int verb=0;




void opt_str()
{

}
void fifo_str()
{

    FILE* fptr = fopen(inputfile, "r");
    if (fptr == NULL) {
        printf("File Error 404");
        return;
    }
    while(fscanf(fptr, "%x %c", &v_address, &read_write)>0)
    {
        printf("%x %c\n",v_address,read_write);
        int vpn=v_address & vpn_mask;
        
    }

}
void clock_str()
{
    
}
void lru_str()
{
    
}
void random_str()
{
    
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

    //getting strategy
    do_strategy(method);

    return 0;
}