#include"socketPackage.h"


//Socket
int Socket(int domain, int type, int protocol)
{
  int fd = socket(domain, type, protocol);
  if(fd == -1)
  {
    perror("socket() failed");
    exit(-1);
  }
  
  return fd;
}


//Setsockopt
int Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
	int ret = setsockopt(sockfd, level, optname, optval, optlen);
	if(ret == -1)
	{
    	perror("setsockopt() failed");
    	exit(-1);
  	}
	return ret;
}


//Bind
int Bind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen)
{
  int ret = bind(sockfd, my_addr, addrlen);
  if(ret == -1)
  {
    perror("bind() failed");
    exit(-1);
  }

  return ret;
}

//Listen
int Listen(int s, int backlog)
{
  int ret = listen(s, backlog);
  if(ret == -1)
  {
    perror("listen() failed");
    exit(-1);
  }

  return ret;
}

//Accept
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
  int fd = accept(s, addr, addrlen);
  if(fd == -1)
  {
    perror("accept() failed");
    exit(-1);
  }

  return fd;
}

//Close
int Close(int fd)
{
  int ret = close(fd);
  if(ret == -1)
  {
    perror("close() failed");
    exit(-1);
  }

  return ret;
}

//Connect
int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  int ret = connect(sockfd, addr, addrlen);
  if(ret == -1)
  {
    perror("connect() failed");
    exit(-1);
  }

  return ret;
}

//Inet_pton
int Inet_pton(int af, const char *src, void *dst)
{
  int ret = inet_pton(af, src, dst);
  if(ret == 0)
  {
    printf("inet_pton() failed: A valid network address in the specified address family is not given.\n");
    exit(-1);
  }
  if(ret == -1)
  {
    perror("inet_pton() failed");
    exit(-1);
  }
  
  return ret;
}

//Inet_ntop
const char *Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
  const char* ret = inet_ntop(af, src, dst, size);
  if(ret == NULL)
  {
    perror("inet_ntop() failed");
    exit(-1);
  }
  
  return ret;
}

//Read
ssize_t Read(int fd, void *buf, size_t n)
{
  ssize_t numRead;

locationReadAgain:
  numRead = read(fd, buf, n);
  printf("nnnnumRead = %d\n", numRead);
  if(numRead == -1)
  {
  
    //出错原因是read()被信号中止，则可以继续读
    if(errno == EINTR)
      goto locationReadAgain;
	//出错原因是非阻塞，则可以继续
	else if(errno == EAGAIN)
      	goto locationReadAgain;
    //否则，报错退出
    else
      perror("read() failed");
  }

  return numRead;
}

//Readn
ssize_t Readn(int fd, void *buf, size_t n)
{
  ssize_t numRead = 0;//numRead：本次调用read()读到多少字节
  size_t numLeft = n;//numLeft：当前还有多少字节需要读
  char* ptr = (char*)buf;//ptr：在buf中的偏移

  while(numLeft > 0)
  {
    numRead = read(fd, ptr, numLeft);
    //读到了缓冲区末尾，再也读不到更多字节了
    if(numRead == 0)
    {
      break;
    }
    //read()出错
    if(numRead == -1)
    {
      //出错原因是read()被信号中止，则可以继续读
      if(errno == EINTR)
      {
        numRead = 0;
        continue;
      }
      //否则，报错退出
      else
      {
        perror("read() failed");
        exit(-1);
      }
    }
    
    //根据本轮read()读到的字节数，更新各变量
    numLeft -= numRead;
    ptr += numRead;
  }
  
  //返回实际读到的字节数：要求字节数 - 剩余字节数
  return n - numLeft;
}


ssize_t Recv(int sockfd, void *buf, size_t len, int flags)
{
	ssize_t numRecv;

locationRecvAgain:
	numRecv = recv(sockfd, buf, len, flags);
 	if(numRecv == -1)
  	{
    	//出错原因是read()被信号中止，则可以继续读
    	if(errno == EINTR)
      		goto locationRecvAgain;
		//出错原因是非阻塞，则可以继续
		else if(errno == EAGAIN)
      		goto locationRecvAgain;
    	//否则，报错退出
    	else
      		perror("recv() failed");
  	}

  return numRecv;
}



//Write
ssize_t Write(int fd, const void *buf, size_t count)
{
	ssize_t numWrite;
		
locationWriteAgain:
	numWrite = write(fd, buf, count);
  	if(numWrite == -1)
  	{	
    	//出错原因是write()被信号中止，则可以继续读
    	if(errno == EINTR)
      		goto locationWriteAgain;
		//出错原因是非阻塞，则可以继续
		else if(errno == EAGAIN)
      		goto locationWriteAgain;
    	//否则，报错退出
    	else
      		perror("write() failed");
  	}

	return numWrite;
}


//Send
int Send(int s, const void *msg, size_t len, int flags)
{
	ssize_t numSend;
	
locationSendAgain:
  	numSend = send(s, msg, len, flags);
  	if(numSend == -1)
  	{	
    	//出错原因是read()被信号中止，则可以继续读
    	if(errno == EINTR)
      		goto locationSendAgain;
		//出错原因是非阻塞，则可以继续
		else if(errno == EAGAIN)
      		goto locationSendAgain;
    	//否则，报错退出
    	else
      		perror("send() failed");
  	}

  	return numSend;
}



//Pthread_create
int Pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg)
{
  int ret = pthread_create(thread, attr, start_routine, arg);
  if(ret != 0)
  {
    perror("pthread_create() failed");
    exit(-1);
  }
  
  return ret;
}

//Pthread_detach
int Pthread_detach(pthread_t thread)
{
  int ret = pthread_detach(thread);
  if(ret != 0)
  {
    perror("pthread_detach() failed");
    exit(-1);
  }
  
  return ret;
}

//Pthread_exit
void Pthread_exit(void *retval)
{
  pthread_exit(retval);
}

//Pthread_cancel
int Pthread_cancel(pthread_t thread)
{
  int ret = pthread_cancel(thread);
  if(ret != 0)
  {
    perror("pthread_cancel() failed");
    exit(-1);
  }
  
  return ret;
}

//Pthread_join
int Pthread_join(pthread_t thread, void **retval)
{
  int ret = pthread_join(thread, retval);
  if(ret != 0)
  {
    perror("pthread_join() failed");
    exit(-1);
  }
  
  return ret;
}

//Epoll_create
int Epoll_create(int size)
{
  int ret = epoll_create(size);
  if(ret == -1)
  {
    perror("epoll_create() failed");
    exit(-1);
  }
  
  return ret;
}

//Epoll_ctl
int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
  int ret = epoll_ctl(epfd, op, fd, event);
  if(ret == -1)
  {
    perror("epoll_ctl() failed");
  }
  
  return ret;
}

//Epoll_wait
int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
  int ret = epoll_wait(epfd, events, maxevents, timeout);;
  if(ret == -1)
  {
    perror("epoll_wait() failed");
    exit(-1);
  }
  
  return ret;
}

time_t Time(time_t *t)
{
	time_t ret = time(t);
	if(ret == (time_t)-1)
  	{
   		perror("time() failed");
 		exit(-1);
	}
  
  	return ret;
}


//Stat
int Stat(const char *pathname, struct stat *buf)
{
	int ret = stat(pathname, buf);
  	if(ret == -1)
  	{
    	perror("stat() failed");
  	}
  
 	return ret;
}






