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
	char buf[BUF_LEN];//该监听所需要使用的缓冲区
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
void disconnect(int connectFd, ReactorEvent* ev);
void dealWithGETRequest(int connectFd, const char* fileName);
void sendRespondHead(int connectFd, int no, const char* description, const char* type, int len);
void sendFile(int connectFd, const char* fileName);
const char* getFileType(const char* fileName);








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
int getLine(int connectFd, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size-1) && (c != '\n'))
	{  
        n = Recv(connectFd, &c, 1, 0);
		
        if (n > 0) 
		{     
            if (c == '\r')
			{            
                n = Recv(connectFd, &c, 1, MSG_PEEK);
				
                if ((n > 0) && (c == '\n'))
					Recv(connectFd, &c, 1, 0);
				else
					c = '\n';
            }
			
            buf[i] = c;
            i++;
        }
		else     
            c = '\n';
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
		memset(ev->buf, 0, BUF_LEN); 
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
	ev->events = events;
	event.data.ptr = ev;

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

	//epoll_ctl将事件删除
	Epoll_ctl(epollFd, EPOLL_CTL_DEL, ev->fd, NULL);
}


//Summary:初始化反应堆
//Parameters:
//		size:指定反应堆大小，这个大小只是建议而已，实际上内核会自主决定，所以只需要给个大于0的值即可
//Return: void
void initReactor()
{
	//创建监听句柄listenFd
	int listenFd = Socket(AF_INET, SOCK_STREAM, 0);
	
	//设置服务器ip、端口信息
	struct sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PORT);
	addrServer.sin_addr.s_addr = htonl(INADDR_ANY);

	//设置端口复用
	int opt = 1;
	Setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));
	
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
	static int loopNum = 1;
	//初始化反应堆以及服务器
	initReactor();
	while(1)
	{
		printf("第%d轮监听\n", loopNum++);
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
			if((event.events & EPOLLIN) && (ev->events & EPOLLIN))
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
	
	//读http报文请求行
	char line[BUF_LEN] = {0};
	int len = getLine(connectFd, line, sizeof(line));
	
	//未读到数据，因为对端已关闭
	if(len == 0)
	{
		//服务器也关闭该连接，并将该连接事件从epoll上取下
		disconnect(connectFd, ev);
		return;
	}
	
	//读取出错
	if(len == -1)
	{
		//EAGAIN和EINTR已经在Recv()内部处理，所以该处错误一定是其他错误，
		//对该错误，做不处理该次HTTP请求的处理
		perror("recv() failed");
		return;
	}
	
	//正确读到http消息的第一行，则从其中，解析得到method、path、protocol
	char method[16], path[256], protocol[16];
	sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
	printf("method=%s, path=%s, protocol=%s\n", method, path, protocol);

	//将该条http报文读完，以防止拥塞
	while(1)
	{
		char bufRubbish[BUF_LEN];
		len = getLine(connectFd, bufRubbish, sizeof(bufRubbish));
		if(bufRubbish[0] == '\n')
			break;
		if(len == -1 || len == 0)
			break;

		//打印出其他每行的信息
		//printf("%s", bufRubbish);
	}

	//该http请求为GET报文
	if(strncasecmp(method, "GET", 3) == 0)
	{
		//得到URL路径
		char* fileName = path + 1;

		//处理http请求
		dealWithGETRequest(connectFd, fileName);
		
		//断开连接
		disconnect(connectFd, ev);
	}
}


//Summary:断开某连接
//Parameters:
//		connectFd:某连接的句柄
//		ev：该连接在epoll上对应的事件
//Return: void
void disconnect(int connectFd, ReactorEvent* ev)
{
	//将该连接事件从epoll上删除
	delEvent(connectFd, ev);

	//并关闭该连接
	Close(connectFd);
}


//Summary:处理GET请求
//Parameters:
//		connectFd:某连接的句柄
//		fileName：请求访问的文件路径
//Return: void
void dealWithGETRequest(int connectFd, const char* fileName)
{
	//取得请求资源的信息
	struct stat stateFile;
	int ret = Stat(fileName, &stateFile);
	
	//如果请求的资源不存在
	if(ret == -1)
	{
		//回复404界面
		//
		printf("%s is not existed\n", fileName - 1);
		return;
	}


	//如果该资源是普通文件，则可以将该文件的内容直接回复
	if(S_ISREG(stateFile.st_mode))
	{
		//回复HTTP响应首部
		sendRespondHead(connectFd, 200, "OK", getFileType(fileName), stateFile.st_size);
		
		//回复文件内容
		sendFile(connectFd, fileName);		
	}
	//如果该资源是目录文件，则………………
	else
	{}
}



//Summary:发送响应报文的首部
//Parameters:
//		connectFd:某连接的句柄
//		no：状态码
//		description：状态码描述
//		type：回发文件的类型
//		len：回发文件的长度
//Return: void
void sendRespondHead(int connectFd, int no, const char* description, const char* type, int len)
{
	char bufRespondHead[BUF_LEN];

	//拼接状态行
	sprintf(bufRespondHead, "HTTP/1.1 %d %s\r\n", no, description);
	
	//拼接Content-Type字段
	sprintf(bufRespondHead + strlen(bufRespondHead), "Content-Type: %s\r\n", type);

	//拼接Content-Length字段
	sprintf(bufRespondHead + strlen(bufRespondHead), "Content-Length: %d\r\n", len);

	//拼接头部结束标志
	sprintf(bufRespondHead + strlen(bufRespondHead), "\r\n");

	//发送首部
	Send(connectFd, (const void*)bufRespondHead, strlen(bufRespondHead), 0);
	
	printf("%s", bufRespondHead);
}


//Summary:发送响应报文的文件内容部分
//Parameters:
//		connectFd:某连接的句柄
//		fileName：请求访问的文件路径
//Return: void
void sendFile(int connectFd, const char* fileName)
{
	//打开文件
	int fileFd = open(fileName, O_RDONLY);
	if(fileFd == -1)
	{
		//发送404
		//
		
		return;
	}

	char bufFile[BUF_LEN] = {0};
	int numRead = 0;
	
	//循环读取文件内容，直到读完
	while((numRead = Read(fileFd, bufFile, sizeof(bufFile))) > 0)
	{
		//将文件内容发送
		int numSend = Send(connectFd, bufFile, numRead, 0);
		
		//如果numSend为-1，因为EAGAIN和EINTR都已经在Send()内部处理，所以传出的一定是其他错误
		//所以此处需要对错误进行处理，如果此处不做处理，会导致程序崩溃
		//但是为什么，会崩溃呢？？？？
		if(numSend == -1)
			break;
	}
		
	Close(fileFd);
}


//Summary:通过文件名确定文件类型
//Parameters:
//		fileName：请求访问的文件路径
//Return: 文件类型
const char* getFileType(const char* fileName)
{
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char* dot = strchr(fileName, '.');   
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav" ) == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}


