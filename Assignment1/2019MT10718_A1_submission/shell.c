#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>


const int his_size=6;
char cmd_his[his_size][1024];
int commands=0;
const int bsize=1024;
int process_log[bsize];
char *process_stat[bsize];
int running_process;

int printpath()
{
    char *buf = getcwd(NULL, 0);
    if (buf == NULL) {
        printf("getcwd error");
        return 1;
    }
    printf("%s~$ ", buf);
    free(buf);
    return 0;
}
char* get_command()
{
    char *buffer;
    size_t b_size = bsize;
    buffer = (char *)malloc(b_size * sizeof(char));
    if( buffer == NULL)
    {
        printf("Unable to allocate buffer");
        return NULL;
    }
    getline(&buffer,&b_size,stdin);
    strcpy(cmd_his[commands%his_size], buffer);
    commands++;
    return buffer;
}
void ps_update(int pid,int stat)
{
    for(int i=0;i<bsize;i++)
    {
        if(process_log[i]==-1)
        {
            process_log[i]=pid;
            if(stat==0)
            {
                process_stat[i]="STOPPED";
            }
            else{
                process_stat[i]="RUNNING";
            }
            process_log[i+1]=-1;
            process_stat[i+1]=NULL;
            return;
        }
        if(process_log[i]==pid)
        {
            process_log[i]=pid;
            if(stat==0)
            {
                process_stat[i]="STOPPED";
            }
            else{
                process_stat[i]="RUNNING";
            }
            return;
        }
        
    }
}
int sep_pipes(char *line, char **pipeparts)
{
    int i;
	for (i = 0; i < 2; i++) {
    pipeparts[i] = strsep(&line, "|");
    if (pipeparts[i] == NULL)
        break;
	}

	if (pipeparts[1] == NULL)
	return 0; 
    return 1;
}
void sep_args(char *line, char**args)
{
    int i;

	for (i = 0; i < bsize; i++) {
		args[i] = strsep(&line, " \n\t\r\a");

		if (args[i] == NULL)
			break;
		if (strlen(args[i]) == 0)
			i--;
	}
}
int assignop(char *line,char **args)
{
    int i;
	for (i = 0; i < 2; i++) {
    args[i] = strsep(&line, "=");
    if (args[i] == NULL)
        break;
	}

	if (args[1] == NULL)
	return 0; 
    return 1;
}
int checkbag(char **args)
{
    if(args[0][0]=='&')
    {
        int j;
        char *temp[bsize];
        char *sentence=args[0];
        for (j = 0; j <= 2; j++) {
            temp[j] = strsep(&sentence, "&");
            if (temp[j] == NULL)
                break;
        }
        args[0]=temp[1];
        return 1;
    }
    return 0;
}
int checkenv(char **args)
{
    for(int i=0;i<bsize;i++)
    {
        if(args[i]==NULL)
        break;
        if(args[i][0]=='$')
        {
            int j;
            char *temp[bsize];
            char *sentence=args[i];
            for (j = 0; j <= 2; j++) {
                temp[j] = strsep(&sentence, "$");
                if (temp[j] == NULL)
                    break;
            }
            
            char *val=getenv(temp[1]);
            args[i]=val;

        }
    }
    return 0;
}
int getargs(char *line,char **args,char **pipeargs)
{
    char *pipeparts[2],*equalparts[2];
    int equalnum = assignop(line,equalparts);
    int pipenum=0;
    if(equalnum==0)
    pipenum=sep_pipes(line,pipeparts);
    
    
    if(equalnum==1)
    {
        sep_args(equalparts[0],args);
        sep_args(equalparts[1],pipeargs);
    }
    else if(pipenum==0)
    {
        sep_args(pipeparts[0],args);
    }
    else{
        sep_args(pipeparts[0],args);
        sep_args(pipeparts[1],pipeargs);
    }
    int chk,bgchk=0;
    if(pipenum==0)
    {
        chk=checkbag(args);
        if(chk)
        bgchk=1;
    }
    chk=checkenv(args);
    if(chk==-1)
    {
        return -1;
    }
    chk=checkenv(pipeargs);
    if(chk==-1)
    {
        return -1;
    }
    
    if(equalnum)
    {
        return 3;
    }
    if(bgchk)
    return 4;
    return 1+pipenum;
}
int run_args(char **args)
{
    int rc=fork();
    if(rc<0)
    {
        printf("Fork failed");
        return 1;
    }
    else if(rc==0)
    {
        
        if (execvp(args[0], args) < 0) {
            printf("Could not execute command -> %s\n",args[0]);
            exit(EXIT_FAILURE);
        }
    }
    else 
    {
        ps_update(rc,1);
        // printf("parent of child %d\n ",rc);
        int wc=wait(NULL);
        ps_update(rc,0);
    }
    return 0;
}
void process_handler()
{
    // printf("Background process over\n");
    // ps_update(running_process,0);
    int kidpid,status;
    while ((kidpid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("Child %d terminated\n", kidpid);
    }

}
int runbg(char **args)
{

    // signal(SIGCHLD,process_handler);
    int status;
    int rc=fork();
    if(rc<0)
    {
        printf("Fork failed");
        return 1;
    }
    else if(rc==0)
    {
        
        if (execvp(args[0], args) < 0) {
            printf("Could not execute command -> %s\n",args[0]);
            exit(EXIT_FAILURE);
        }
    }
    else 
    {
        ps_update(rc,1);
        waitpid(rc, &status, WNOHANG);
    }
    return 0;
}
void cmd_history()
{
    for(int i=0;i<5;i++)
    {
        if(commands-2-i>=0)
        printf("%s",cmd_his[(commands-2-i)%his_size]);
    }
}
void ps_history()
{
    for(int i=0;i<bsize;i++)
    {
        if(process_log[i]==-1)
        return;
        int stat=kill(process_log[i],0);
        if(stat==0)
        {
            printf("%d %s\n",process_log[i], "RUNNING");
        }
        else{
            printf("%d %s\n",process_log[i], "STOPPED");
        }
    }
}
void run_pipe(char **args, char **pipeargs)
{
	int pipeline[2];
	pid_t p1, p2;

	if (pipe(pipeline) < 0) {
		printf("Error in piping\n");
		return;
	}
	p1 = fork();
	if (p1 < 0) {
		printf("Fork 1 failed/n");
		return;
	}

	if (p1 == 0) {


		close(pipeline[0]);
		dup2(pipeline[1], STDOUT_FILENO);
		close(pipeline[1]);


        if(strcmp(args[0],"cmd_history")==0)
        {
            cmd_history();
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(args[0],"ps_history")==0)
        {
            ps_history();
            exit(EXIT_SUCCESS);
        }
        else if (execvp(args[0], args) < 0) {
            perror("Could not execute command 1\n");
			exit(EXIT_FAILURE);
		}


		
	} else {
		p2 = fork();

		if (p2 < 0) {
			printf("Fork 2 failed\n");
			return;
		}
		if (p2 == 0) {
			close(pipeline[1]);
			dup2(pipeline[0], STDIN_FILENO);
			close(pipeline[0]);
            if(strcmp(pipeargs[0],"cmd_history")==0)
            {
                cmd_history();
                exit(EXIT_SUCCESS);
            }
            else if(strcmp(pipeargs[0],"ps_history")==0)
            {
                ps_history();
                exit(EXIT_SUCCESS);
            }
            else if (execvp(pipeargs[0], pipeargs) < 0) {
                perror("Could not execute command 2\n");
                exit(EXIT_FAILURE);
                // return;
            }
			// if (execvp(pipeargs[0], pipeargs) < 0) {
			// 	printf("Could not execute command 2\n");
			// 	exit(EXIT_FAILURE);
		} else {
            close(pipeline[1]);
            close(pipeline[0]);
			wait(NULL);
			wait(NULL);
		}
	}
}
void ctrl_c_handler(int sig)
{
    signal(sig, SIG_IGN);
    // printf("OUCH BYE\n");
    exit(0);
}

int main(int argc,char **argv)
{
    // printf("\nAssignment terminal begins\n\n");
    signal(SIGINT, ctrl_c_handler);
    process_log[0]=-1;
    process_stat[0]=NULL;
    int id;
    while(1)
    {
        id=printpath();
        if(id)
        continue;
        int iscom=0;
        char *line=get_command();
        
        
        int kpid,stat;
        while ((kpid = waitpid(-1, &stat, WNOHANG)) > 0)
        {
        }

        if(line==NULL)
        continue;

        for(int i=0;i<bsize;i++)
        {
            if(line[i]=='\n')
            break;
            if(line[i]!=' ' && line[i]!='\t' && line[i]!='\r')
            {
                iscom=1;
                break;
            }
        }
        if(!iscom)
        {
            continue;
        }
        char *args[1024], *pipeargs[1024];
        int processtype=getargs(line,args,pipeargs);
        if(processtype==-1)
        {
            printf("Entered command is incorrect");
        }
        else if(processtype == 1)
        {
            if(strcmp(args[0],"cmd_history")==0)
            {
                cmd_history();
            }
            else if(strcmp(args[0],"ps_history")==0)
            {
                ps_history();
            }
            else{
                id=run_args(args);
                if(id)
                continue;
            }
        }
        else if(processtype==2)
        {
            run_pipe(args,pipeargs);
        }
        else if(processtype==3)
        {
            const char *name=args[0];
            const char *value=pipeargs[0];
            int envnum=setenv(name, value, 1);
        }
        else if(processtype==4)
        {
            if(strcmp(args[0],"cmd_history")==0)
            {
                cmd_history();
            }
            else if(strcmp(args[0],"ps_history")==0)
            {
                ps_history();
            }
            else{
                id=runbg(args);
                if(id)
                continue;
            }
        }
    }
    exit(EXIT_SUCCESS);
    
}