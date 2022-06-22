#ifndef _SOCKETPACKAGE_
#define _SOCKETPACKAGE_

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/un.h>
#include <stddef.h>
#include <dirent.h>

using namespace std;






int Socket(int domain, int type, int protocol);
int Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int Bind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen);
int Listen(int s, int backlog);
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int Close(int fd);
int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int Inet_pton(int af, const char *src, void *dst);
const char *Inet_ntop(int af, const void *src, char *dst, socklen_t size);
ssize_t Read(int fd, void *buf, size_t n);
ssize_t Readn(int fd, void *buf, size_t n);
ssize_t Recv(int sockfd, void *buf, size_t len, int flags);
ssize_t Write(int fd, const void *buf, size_t count);
int Send(int s, const void *msg, size_t len, int flags);
int Pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
int Pthread_detach(pthread_t thread);
void Pthread_exit(void *retval);
int Pthread_cancel(pthread_t thread);
int Pthread_join(pthread_t thread, void **retval);
int Epoll_create(int size);
int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
time_t Time(time_t *t);
int Stat(const char *pathname, struct stat *buf);
int Pthread_mutex_destroy(pthread_mutex_t *mutex);
int Pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int Pthread_mutex_lock(pthread_mutex_t *mutex);
int Pthread_mutex_trylock(pthread_mutex_t *mutex);
int Pthread_mutex_unlock(pthread_mutex_t *mutex);
int Pthread_cond_destroy(pthread_cond_t *cond);
int Pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr);
int Pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec* abstime);
int Pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int Pthread_cond_broadcast(pthread_cond_t *cond);
int Pthread_cond_signal(pthread_cond_t *cond);










#endif
