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

ReactorEvent arrayEvents[REACTOR_SIZE + 1];


int createReactor(int size = REACTOR_SIZE);
void setEvent(int fd, void (*callBack)(int, int, void*), ReactorEvent* ev, bool isInit);
void addEvent(int epollFd, int events, ReactorEvent* ev);
void initReactor();
void runReactor(int epollFd);
void acceptConnectionWork(int fd, int events, void* arg);
void sendMessageWork(int fd, int events, void* arg);
void recvMessageWork(int fd, int events, void* arg);





//Summary:  httpServer主函数
//Parameters:
//       无参数
//Return : 0.
int main(int argc, char* argv[])
{
	//更改工作路径
	int ret = chdir("../dir/");
	if(ret == -1)
	{
		perror("chdir() failed");
		exit(0);
	}

	//创建反应堆
	int epollFd = createReactor();

	//反应堆运行
	
	
	return 0;
}


//Summary:创建反应堆
//Parameters:
//		size:指定反应堆大小，这个大小只是建议而已，实际上内核会自主决定，所以只需要给个大于0的值即可
//Return:
//		创建成功，则返回反应堆句柄
//		创建失败，则返回-1，且设置errno
int createReactor(int size = REACTOR_SIZE)
{
	int epollFd = Epoll_create(REACTOR_SIZE);
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


//Summary:将监听挂上epoll
//Parameters:
//		epollFd：epoll的句柄
//		events：监听模式
//		ev：指向该事件结构体的指针，为传入传出参数
//Return:void
void addEvent(int epollFd, int events, ReactorEvent* ev)
{
	//创建epoll_ctl操作的真实对象
	struct epoll_event event = {0, {0}};
	
	//设置events
	event.events = events;
	ev->events = events
	events.data.ptr = ev;

	//设置status
	ev->status = 1;
	
	//epoll_ctl将监听挂上
	Epoll_ctl(epollFd, EPOLL_CTL_ADD, ev->fd, &event);
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
	socklen_t addrlen = sizeof(addrServer);
	
	//bind
	Bind(listenFd, (struct sockaddr*)&addrServer, addrlen);

	//listen
	Listen(listenFd, LISTEN_BACKLOG);

	//打印服务器信息
	printf("server created successfully, fd = %d, ip = %s\n", listenFd, inet_ntoa(addrServer.sin_addr));
	
	//设置listenFd的监听事件
	setEvent(listenFd, acceptConnectionWork, &arrayEvents[REACTOR_SIZE], 1);
	
	//将listenFd挂上epoll
	addEvent(epollFd, EPOLLIN, &arrayEvents[REACTOR_SIZE])
}


//Summary:运行反应堆
//Parameters:
//		epollFd:指定反应堆的句柄
//Return: void
void runReactor(int epollFd)
{
	//初始化反应堆以及服务器
	initReactor();
	while(1)
	{
		//断开很久不活跃的连接
		//反应堆监听
		//处理监听到的事件
	}
}



void acceptConnectionWork(int fd, int events, void* arg)
{
	//创建connectFd
	//accept
	//在arrayEvent中，为新的连接connectFd寻找空位
	//如果没有找到空位，就关闭该连接，并且打印提示信息
	//设置connectFd为非阻塞
	//设置connectFd的监听事件	
	//将connectFd挂上epoll，ET模式
}