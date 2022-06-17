#include <sys/types.h>
#include <errno.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
using namespace std;



ssize_t Read(int fd, void *buf, size_t n)
{
  ssize_t numRead;

locationReadAgain:
  numRead = read(fd, buf, n);
  if(numRead == -1)
  {
  printf("nnnnumRead = %d\n", numRead);
    //出错原因是read()被信号中止，则可以继续读
       if(errno == EINTR)
              goto locationReadAgain;
    //          	//出错原因是非阻塞，则可以继续
              		else if(errno == EAGAIN)
              		      	goto locationReadAgain;
    //          		      	    //否则，报错退出
              		      	        else
              		      	              perror("read() failed");
              		      	                }
    
              		      	                  return numRead;
}





int main()
{

int fd = open("../resource/woc.txt", O_RDWR);
int n;

char buf[1024];

//for(int i = 0; i<10; i++)
//{
//	sprintf(buf, "%d", i);
//	write(fd, buf, 1);
//}

//lseek(fd, 0, SEEK_SET);

while((n = Read(fd, buf, 1024)) > 0)
{
	printf("n = %d\n", n);
        printf("strlen(buf) = %d\n", strlen(buf));
	printf(" %s\n", buf);
	write(STDOUT_FILENO, buf, strlen(buf));
	printf("\n");
}

printf("asdas\n");
return 1;
}
