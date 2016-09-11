//
///////////////////////////// named pipe reader
//
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_BUF 1024

int main()
{
    int fd;
    char * myfifo = "/tmp/myfifo";
    char buf[MAX_BUF];

    /* open, read, and display the message from the FIFO */
    fd = open(myfifo, O_RDONLY);
    int nread = read(fd, buf, MAX_BUF);
    while (nread != 0)
    {
        printf("Received: %s\n", buf);
        nread = read(fd, buf, MAX_BUF);
    }
    close(fd);
    return 0;
}

#if 0
//
///////////////////////////// named pipe writer
//
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main()
{
    int fd;
    char * myfifo = "/tmp/myfifo";

    /* create the FIFO (named pipe) */
    mkfifo(myfifo, 0666);

    /* write "Hi" to the FIFO */
    fd = open(myfifo, O_WRONLY);
    char buff[500];
    char* line = gets(buff);
    while (strcmp(line, "bye") != 0)
    {
        write(fd, line, strlen(line));
        line = gets(buff);
    }
    close(fd);
    
    /* remove the FIFO */
    unlink(myfifo);

    return 0;
}

//
///////////////////////////// logger.cpp
//
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main()
{
    char buff[500];
    while (!feof(stdin))
    {
        char* line = gets(buff);
        printf("%s\n", line);
    }
    return 0;
}

//
///////////////////////////// sender.cpp
//
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_BUF 1024

int main()
{
    char buff[1024] = {0};
    FILE* cvt;
    int status;
    /* Launch converter and open a pipe through which the parent will write to it */
    cvt = popen("logger", "w");
    if (!cvt)
    {
        printf("couldn't open a pipe; quitting\n");
        return 1;
    }
    printf("enter Fahrenheit degrees: " );
    fgets(buff, sizeof (buff), stdin); /*read user's input */
    /* Send expression to converter for evaluation */
    fprintf(cvt, "%s\n", buff);
    fflush(cvt);
    /* Close pipe to converter and wait for it to exit */
    status=pclose(cvt);
    return 0;
}

#endif
