#include"server.h"
#define REACTOR_SIZE 4095
#define LISTEN_BACKLOG 1024
#define PORT 5000
#define BUF_LEN 4096

struct ReactorEvent
{
	int fd;//该监听对应的句柄
	int status;//该监听在树上，为1；否则，为0
	void* arg;//泛型指针，用来给callBack传三参
	int events;//该监听所监听事件的类型，填EPOLLIN等
	void (*callBack)(int fd, int events, void* arg);//回调函数，根据fd和events的不同，对所监听到的事件做出对应的操作
	char buf[LEN_BUF];//该监听所需要使用的缓冲区
	int len;//缓冲区内字符数量
	time_t lastActive;//最后一次活跃的时间
};

int epollFd;
ReactorEvent arrayEventsMonitoring[REACTOR_SIZE + 1];
struct epoll_event arrayEventsReady[REACTOR_SIZE + 1];




int createReactor(int size);
void setEvent(int fd, void (*callBack)(int, int, void*), ReactorEvent* ev, bool isInit);
void addEvent(int events, ReactorEvent* ev);
void delEvent(int socketFd, ReactorEvent* ev);
void initReactor();
void runReactor();
void acceptConnectionWork(int listenFd, int events, void* arg);
void recvMessageWork(int connectFd, int events, void* arg);
int getLine(int fd, char *buf, int size);





//Summary:  httpServer主函数
//Parameters:
//       无参数
//Return : 0.
int main(int argc, char* argv[])
{
	//更改工作路径
	int ret = chdir("../resource/");
	if(ret == -1)
	{
		perror("chdir() failed");
		exit(0);
	}

	//创建反应堆
	epollFd = createReactor(REACTOR_SIZE);

	//反应堆运行
	runReactor();
	
	return 0;
}


//Summary:读取一行以\r\n结尾的数据
//Parameters:
//		fd：读取的文件的句柄
//		buf：承接数据的内存的指针
//		size：buf的大小
//Return:
//		读到数据，则返回正数
//		对端关闭，则返回0
//		读取错误，则返回-1，且设置errno
int getLine(int fd, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size-1) && (c != '\n')) {  
        n = recv(cfd, &c, 1, 0);
        if (n > 0) {     
            if (c == '\r') {            
                n = recv(cfd, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n')) {              
                    recv(cfd, &c, 1, 0);
                } else {                       
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        } else {      
            c = '\n';
        }
    }
    buf[i] = '\0';
    
    if (-1 == n)
    	i = n;

	return i;
}


//Summary:创建反应堆
//Parameters:
//		size:指定反应堆大小，这个大小只是建议而已，实际上内核会自主决定，所以只需要给个大于0的值即可
//Return:
//		创建成功，则返回反应堆句柄
//		创建失败，则返回-1，且设置errno
int createReactor(int size)
{
	int epollFd = Epoll_create(size);
	if(epollFd == -1)
	{
		perror("epoll_create() failed");
		exit(0);
	}
	return epollFd;
}


//Summary:设置反应堆事件
//Parameters:
//		fd：该事件所监听的句柄
//		callBack：该事件触发的回调函数
//		ev：指向该事件结构体的指针，为传入传出参数
//		isInit：表示现在这次设置，是否是对该fd的初始化设置，若是，则为1，否则为0
//Return:void
void setEvent(int fd, void (*callBack)(int, int, void*), ReactorEvent* ev, bool isInit)
{
	ev->fd = fd;
	ev->status = 0;
	ev->arg = ev;
	ev->events = 0;
	ev->callBack = callBack;
	ev->lastActive = Time(NULL);

	if(isInit)
	{
		ev->len = 0;
		memset(buf, 0, BUF_LEN); 
	}
}


//Summary:将事件挂上epoll
//Parameters:
//		epollFd：epoll的句柄
//		events：监听模式
//		ev：指向该事件结构体的指针，为传入传出参数
//Return:void
void addEvent(int events, ReactorEvent* ev)
{
	//该事件已经在epoll上
	if(ev->status == 1)
		return;

	//创建epoll_ctl操作的真实对象
	struct epoll_event event = {0, {0}};
	
	//设置events
	event.events = events;
	ev->events = events
	events.data.ptr = ev;

	//设置status
	ev->status = 1;
	
	//epoll_ctl将事件挂上
	Epoll_ctl(epollFd, EPOLL_CTL_ADD, ev->fd, &event);
}


//Summary:将事件从epoll上摘下
//Parameters:
//		socketFd：要摘下的事件的句柄
//		ev：指向该事件结构体的指针
//Return:void
void delEvent(int socketFd, ReactorEvent* ev)
{
	//该事件本来就不在epoll上
	if(ev->status == 0)
		return;

	//设置status
	ev->status = 0;

	//epoll_ctl将事件挂上
	Epoll_ctl(epollFd, EPOLL_CTL_DEL, ev->fd, NULL);
}


//Summary:初始化反应堆
//Parameters:
//		size:指定反应堆大小，这个大小只是建议而已，实际上内核会自主决定，所以只需要给个大于0的值即可
//Return: void
void initReactor()
{
	//创建监听句柄listenFd
	int listenFd = Socket(int AF_INET, SOCK_STREAM, 0);
	
	//设置服务器ip、端口信息
	struct sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PORT);
	addrServer.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//bind
	Bind(listenFd, (struct sockaddr*)&addrServer, sizeof(addrServer));

	//listen
	Listen(listenFd, LISTEN_BACKLOG);

	//打印服务器信息
	printf("server created successfully, fd = %d, ip = %s\n", listenFd, inet_ntoa(addrServer.sin_addr));
	
	//设置listenFd的监听事件
	setEvent(listenFd, acceptConnectionWork, &arrayEventsMonitoring[REACTOR_SIZE], 1);
	
	//将listenFd挂上epoll
	addEvent(EPOLLIN, &arrayEventsMonitoring[REACTOR_SIZE]);
}


//Summary:运行反应堆
//Parameters:
//		epollFd:指定反应堆的句柄
//Return: void
void runReactor()
{
	//初始化反应堆以及服务器
	initReactor();
	while(1)
	{
		//断开很久不活跃的连接
		//
		//
		//
		//
		


		
		//监听epoll, 200ms没有事件满足, 则返回0
		int numReady = Epoll_wait(epollFd, arrayEventsReady, REACTOR_SIZE + 1, 200);

		//如果没有满足时间，则继续监听
		if(numReady == 0)
			continue;
		
		//处理监听到的事件
		for(int i = 0; i < numReady; i++)
		{
			struct epoll_event event = arrayEventsReady[i];
			ReactorEvent* ev = (ReactorEvent*)arrayEventsReady[i].data.ptr;

			
			//如果该事件是读就绪事件
			if((event.events & POLLIN) && (ev->events & EPOLLIN))
				ev->callBack(ev->fd, EPOLLIN, ev);
			//如果该事件是写就绪事件
			//实际上，由于是B/S模型，所以本服务器上没有写就绪事件，只有读就绪事件，所以下面这两行代码，永远不会被执行
			else if((event.events & EPOLLOUT) && (ev->events & EPOLLOUT))
				ev->callBack(ev->fd, EPOLLOUT, ev);
		}
		
	}
}


//Summary:服务器监听句柄listenFd的回调函数
//Parameters:
//		listenFd:服务器监听句柄listenFd
//		events：监听事件类型
//		arg：格式规定的参数
//Return: void
void acceptConnectionWork(int listenFd, int events, void* arg)
{
	//accept
	struct sockaddr_in addrClient;
	socklen_t addrlen = sizeof(addrClient);
	int connectFd = Accept(listenFd, (struct sockaddr*)&addrClient, &addrlen);
	
	//在arrayEvent中，为新的连接connectFd寻找空位
	int i = 0;
	for(int i = 0; i < REACTOR_SIZE; i++)
		if(arrayEventsMonitoring[i].status == 0)
			break;

	//如果没有找到空位，就关闭该连接，并且打印提示信息
	if(i == REACTOR_SIZE)
	{
		printf("connected failed, becuase epoll has been FULL\n");
		close(connectFd);
	}
	else
	{
		//设置connectFd为非阻塞
		int flg = fcntl(connectFd, F_GETFL);	
		flg |= O_NONBLOCK;
		fcntl(connectFd, F_SETFL, flg);
		
		//设置connectFd的监听事件
		setEvent(connectFd, recvMessageWork, &arrayEventsMonitoring[i], 1);
		
		//将connectFd挂上epoll，ET模式
		addEvent(EPOLLIN | EPOLLET, &arrayEventsMonitoring[i]);

		printf("connect succeed, fd = %d, ip = %s\n", connectFd, inet_ntoa(addrClient.sin_addr));
	}
}


//Summary:通信句柄connectFd的回调函数
//Parameters:
//		connectFd:服务器监听句柄listenFd
//		events：监听事件类型
//		arg：格式规定的参数
//Return: void
void recvMessageWork(int connectFd, int events, void* arg)
{
	ReactorEvent* ev = (ReactorEvent*)arg;
	
	//处理事务
	char line[4096] = {0};

maskRead:
	//读http消息的第一行
	int len = getLine(connectFd, line, sizeof(line));
	
	//对端已经关闭，则关闭连接，并将该连接事件从epoll上取下
	if(len == 0)
	{
		printf("client has been closed, disconnect now\n");
		delEvent(connectFd, ev);
		Close(connectFd);
		return;
	}
	
	//读取出错
	if(len == -1)
	{
		//可以原谅的错误。重读
		if(errno == EAGAIN)
		{
			printf("EAGAIN00000\n");
			goto maskRead;
		}
		if(errno == EINTR)
		{
			printf("EINTR00000\n");
			goto maskRead;
		}
		//不可原谅的错误
		perror("recv() failed");
		exit(-1);
	}q
	
	//正确读到http消息的第一行
	//从第一行数据中，解析得到method、path、protocol
	char method[16], path[256], protocol[16];
	sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
	printf("method=%s, path=%s, protocol=%s\n", method, path, protocol);

	//将监听事件从树上摘下
	//重新设置监听事件
	//将监听事件挂上树
}
