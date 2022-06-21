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
	int ret;
locationCloseAgain:
  	ret = close(fd);
  
  	if(ret == -1)
  	{
    	perror("close() failed");
		//文件已经被关闭
		if(errno == EBADF)
			return 0;
		//被信号打断
 		else if(errno == EINTR)
			goto locationCloseAgain;
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
	//将缓冲区清空，然后再读
	memset(buf, 0, n);
  	ssize_t numRead;

locationReadAgain:
  	numRead = read(fd, buf, n);
  	if(numRead == -1)
  	{
  		perror("read() failed");
    	//被信号中止，则可以重新读
    	if(errno == EINTR)
      		goto locationReadAgain;
		//读缓冲区无内容，则可以重新读
		else if(errno == EAGAIN || errno == EWOULDBLOCK)
      		goto locationReadAgain;
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
	
	numWrite = write(fd, buf, count);
  	if(numWrite == -1)
  		perror("write() failed");

	return numWrite;
}


//Send
int Send(int s, const void *msg, size_t len, int flags)
{
	ssize_t numSend;
	
locationSendAgain:
  	numSend = send(s, msg, len, flags);
	//if(numSend != len)
		//goto locationSendAgain;
  	if(numSend == -1)
  	{
  		perror("send() failed");
    	//出错原因是read()被信号中止，则可以继续读
    	if(errno == EINTR )
      		goto locationSendAgain;
		//出错原因是非阻塞，则可以继续
		else if(errno == EAGAIN || errno == EWOULDBLOCK)
		{
			sleep(1);
			goto locationSendAgain;
			
		}
      		
		//出错原因是非阻塞，则可以继续
		//else if(errno == ECONNRESET)
      	//	goto locationSendAgain;
    	//否则，报错退出
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



int Pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	int ret = pthread_mutex_destroy(mutex);
  	if(ret != 0)
  	{
    	perror("pthread_mutex_destroy() failed");
  	}
  
 	return ret;
}


int Pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr)
{
	int ret = pthread_mutex_init(mutex, attr);
  	if(ret != 0)
  	{
    	perror("pthread_mutex_init() failed");
  	}
  
 	return ret;
}



int Pthread_mutex_lock(pthread_mutex_t *mutex)
{
	int ret = pthread_mutex_lock(mutex);
  	if(ret != 0)
  	{
    	perror("pthread_mutex_lock() failed");
  	}
  
 	return ret;
}

int Pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	int ret = pthread_mutex_trylock(mutex);
  	if(ret != 0)
  	{
    	perror("pthread_mutex_trylock() failed");
  	}
  
 	return ret;
}
int Pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	int ret = pthread_mutex_unlock(mutex);
  	if(ret != 0)
  	{
    	perror("pthread_mutex_unlock() failed");
  	}
  
 	return ret;
}




int Pthread_cond_destroy(pthread_cond_t *cond)
{
	int ret = pthread_cond_destroy(cond);
  	if(ret != 0)
  	{
    	perror("pthread_cond_destroy() failed");
  	}
  
 	return ret;
}


int Pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr)
{
	int ret = pthread_cond_init(cond, attr);
  	if(ret != 0)
  	{
    	perror("pthread_cond_init() failed");
  	}
  
 	return ret;
}



int Pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict abstime)
{
	int ret = pthread_cond_timedwait(cond, mutex, abstime);
  	if(ret != 0)
  	{
    	perror("pthread_cond_timedwait() failed");
  	}
  
 	return ret;
}


int Pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex)
{
	int ret = pthread_cond_wait(cond, mutex);
  	if(ret != 0)
  	{
    	perror("pthread_cond_wait() failed");
  	}
  
 	return ret;
}


int Pthread_cond_broadcast(pthread_cond_t *cond)
{	
	int ret = pthread_cond_broadcast(cond);
  	if(ret != 0)
  	{
    	perror("pthread_cond_broadcast() failed");
  	}
  
 	return ret;
}


int Pthread_cond_signal(pthread_cond_t *cond)
{
	int ret = pthread_cond_signal(cond);
  	if(ret != 0)
  	{
    	perror("pthread_cond_signal() failed");
  	}
  
 	return ret;
}







