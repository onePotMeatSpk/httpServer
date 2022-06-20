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
void sendError(int connectFd, int errorNo);
void sendDir(int connectFd, const char* dirName);
void sendRespondHead(int connectFd, int statusCode, const char* reasonPhrase, const char* type, int len);
void sendFile(int connectFd, const char* fileName);
const char* getFileType(const char* fileName);
const char* getReasonPhrase(int statusCode);
int hexit(char c);
void encodeStr(char* to, int tosize, const char* from);
void decodeStr(char *to, char *from);










//Summary:  httpServer主函数
//Parameters:
//       无参数
//Return : 0.
int main(int argc, char* argv[])
{
	//忽略SIGPIPE信号
	//因为在服务器发送文件时的几次write的间隙之间，对端有可能close连接（我水平太低，想不通为什么对端会关闭连接，但他就是会）
	//这样write就会出错，并发出SIGPIPE信号
	//而SIGPIPE信号的默认处理方式，是kill掉当前进程！！！那么就会导致程序直接崩掉（血泪教训！！！）
	//所以需要忽略SIGPIPE信号
	signal(SIGPIPE, SIG_IGN);

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
	//utf-8 -> url，保证服务器可以得到正确的请求
	decodeStr(path, path);
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
	}

	//该http请求为GET报文
	if(strncasecmp(method, "GET", 3) == 0)
	{
		//得到URL路径
		const char* fileName;
		//如果请求的是根目录
		if(strcmp(path, "/") == 0)
			fileName = ".";
		//如果请求的不是根目录
		else
			fileName = path + 1;
		

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

	//
	printf("DISCONNECT: %d\n", connectFd);
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
	
	//如果请求的资源不存在，则回复404
	if(ret == -1)
		sendError(connectFd, 404);
	//如果该资源是普通文件，则可以将该文件的内容直接回复
	else if(S_ISREG(stateFile.st_mode))
	{
		//回复HTTP响应首部
		sendRespondHead(connectFd, 200, getReasonPhrase(200), getFileType(fileName), stateFile.st_size);
		
		//回复文件内容
		sendFile(connectFd, fileName);		
	}
	//如果该资源是目录文件，则发送含有该目录中所有目录项的超链接和大小信息的HTML
	else
	{
		sendDir(connectFd, fileName);
		return;
	}
}


//Summary:发送HTTP错误响应报文
//Parameters:
//		connectFd:某连接的句柄
//		errorNo：错误号
//Return: void
void sendError(int connectFd, int errorNo)
{
	//制作报文体
	char bufErrorHTML[BUF_LEN] = {0};
	sprintf(bufErrorHTML + strlen(bufErrorHTML), "<html>\n");
	
	sprintf(bufErrorHTML + strlen(bufErrorHTML), "<head>\n");
	sprintf(bufErrorHTML + strlen(bufErrorHTML), "<title>%d %s</title>\n", errorNo, getReasonPhrase(errorNo));
	sprintf(bufErrorHTML + strlen(bufErrorHTML), "</head>\n");
	
	sprintf(bufErrorHTML + strlen(bufErrorHTML), "<body>\n");
	sprintf(bufErrorHTML + strlen(bufErrorHTML), "<h1>%d %s</h1>\n", errorNo, getReasonPhrase(errorNo));
	sprintf(bufErrorHTML + strlen(bufErrorHTML), "<hr size = 3/>\n");
	sprintf(bufErrorHTML + strlen(bufErrorHTML), "</body>\n");
	
	sprintf(bufErrorHTML + strlen(bufErrorHTML), "</html>\n");


	//发送错报文首部
	sendRespondHead(connectFd, errorNo, getReasonPhrase(errorNo), "text/html", strlen(bufErrorHTML));

	//发送报文体
	Send(connectFd, bufErrorHTML, strlen(bufErrorHTML), 0);
}



//Summary:发送HTTP目录文件响应报文
//Parameters:
//		connectFd:某连接的句柄
//		errorNo：错误号
//Return: void
void sendDir(int connectFd, const char* dirName)
{
	//提取本页目录中，的所有目录项信息，每个目录项都是一个子文件（或子文件夹）
	struct dirent** direntList;
	int numDirent = scandir(dirName, &direntList, NULL, alphasort);
	
	//制作报文体
	char bufDirHTML[BUF_LEN] = {0};
	sprintf(bufDirHTML + strlen(bufDirHTML), "<html>\n");
	sprintf(bufDirHTML + strlen(bufDirHTML), "<head>\n");
	sprintf(bufDirHTML + strlen(bufDirHTML), "<title>%d %s</title>\n", 200, getReasonPhrase(200));
	sprintf(bufDirHTML + strlen(bufDirHTML), "</head>\n");
	
	sprintf(bufDirHTML + strlen(bufDirHTML), "<body>\n");
	sprintf(bufDirHTML + strlen(bufDirHTML), "<table width=\"500\">\n");
	
	//将该页的路径，进行utf-8 -> url转码，保证web端可以正确解析
	char dirNameEncoded[1024] = {0};
	encodeStr(dirNameEncoded, sizeof(dirNameEncoded), dirName);
	
	//附上首页的绝对路径，实现返回首页的功能
	sprintf(bufDirHTML + strlen(bufDirHTML), "<tr><td><a href=\"/\">%s</a></td></tr>\n", "返回首页");
	
	//附上本页的绝对路径，实现刷新本页的功能
	sprintf(bufDirHTML + strlen(bufDirHTML), "<tr><td><a href=\"/%s\">%s</a></td></tr>\n", dirNameEncoded, "刷新本页");
	
	//附上上页的绝对路径，实现返回上页的功能
	//如果本页是首页，那自然没有上页，也就不需要上页链接了，否则需要上层链接
	if(strcmp(dirNameEncoded, "."))
	{
		
		//在本页路径中，从右往左找寻第一个'/'，'/'之前的部分，即为上页的路径
		const char* partition = strrchr(dirNameEncoded, '/');

		//如果本页路径中没有'/'，说明上页是首页，则不需要提取上页路径，直接附上首页路径即可
		if(partition == NULL)
			sprintf(bufDirHTML + strlen(bufDirHTML), "<tr><td><a href=\"/\">%s</a></td></tr>\n", "返回上页");
		//否则，需要将上页路径提取出来
		else
		{
			char upperLevelPath[1024] = {0};
			strncpy(upperLevelPath, dirNameEncoded, partition - dirNameEncoded);
			sprintf(bufDirHTML + strlen(bufDirHTML), "<tr><td><a href=\"/%s\">%s</a></td></tr>\n", upperLevelPath, "返回上页");
		}
	}

	//附上子文件（或子文件夹）的绝对路径，实现浏览目录以及进入子文件（或子文件夹）的功能
	for(int i = 0; i < numDirent; i++)
	{
		//将该页子文件（或子文件夹）的路径，进行utf-8 -> url转码，保证web端可以正确解析
		char newPartEncoded[1024] = {0};
		encodeStr(newPartEncoded, sizeof(newPartEncoded), direntList[i]->d_name);
		
		//制作报文
		if(strcmp(newPartEncoded, ".") && strcmp(newPartEncoded, ".."))
			sprintf(bufDirHTML + strlen(bufDirHTML), "<tr><td><a href=\"/%s/%s\">%s</a></td></tr>\n", dirNameEncoded, newPartEncoded, direntList[i]->d_name);
	}
	sprintf(bufDirHTML + strlen(bufDirHTML), "</table>\n");
	sprintf(bufDirHTML + strlen(bufDirHTML), "</body>\n");
	sprintf(bufDirHTML + strlen(bufDirHTML), "</html>\n");

	//发送报文首部
	sendRespondHead(connectFd, 200, getReasonPhrase(200), "text/html; charset=utf-8", strlen(bufDirHTML));

	//发送报文体
	Send(connectFd, bufDirHTML, strlen(bufDirHTML), 0);
}


//Summary:发送HTTP响应报文的首部
//Parameters:
//		connectFd:某连接的句柄
//		no：状态码
//		description：状态码描述
//		type：回发文件的类型
//		len：回发文件的长度
//Return: void
void sendRespondHead(int connectFd, int statusCode, const char* reasonPhrase, const char* type, int len)
{
	char bufRespondHead[BUF_LEN] = {0};

	//拼接状态行
	sprintf(bufRespondHead, "HTTP/1.1 %d %s\r\n", statusCode, reasonPhrase);
	
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

	//如果打开文件失败，则回复404
	if(fileFd == -1)
	{
		printf("open failed\n");
		sendError(connectFd, 404);
		return; 
	}

	char bufFile[BUF_LEN] = {0};
	int numRead = 0;
	
	//循环读取文件内容，直到读完
	while((numRead = Read(fileFd, bufFile, sizeof(bufFile))) > 0)
	{
		//发送读到的文件内容（采取分包发送的策略）
		int numAlreadySend = 0;//已发送的数据量
		int numSend = 0;//某次调用write所发数据量

		//如果已发数据量<欲发数据量，说明数据仍未发完，需要继续发送
		while(numAlreadySend < numRead)
		{
			//调用write，尝试发送一波数据，以期将剩余数据发送出去
			numSend = Write(connectFd, bufFile + numAlreadySend, numRead - numAlreadySend);

			//调用write出错
			if(numSend == -1)
			{
				//记录错误号，以防在此期间，其他的进程更改了errno
				perror("write failed");
				int errorNo = errno;

				//write被信号中断，则重新调用write
				if(errorNo == EINTR)
					continue;
				//写缓冲区满，则先睡眠10ms，以期写缓冲区腾出空间
				else if(errorNo == EAGAIN || errorNo == EWOULDBLOCK)
					usleep(10000);
				//如果是致命错误，就停止操作，退出函数
				else
					goto locationExit;
			}
			//缓冲区有剩余空间，且成功写入了一些数据
			else
			{
				
				//更新已发数据量
				numAlreadySend += numSend;

				//如果已发数据量<欲发数据量，说明该次调用write，数据仍未发完，也就是说，写缓冲区暂时没有空位
				//则先睡眠10ms，以期写缓冲区腾出空间
				if(numAlreadySend < numRead)
					usleep(10000);				
			}
		}
	
	}
	
locationExit:	
	Close(fileFd);
}


//Summary:通过文件名确定文件类型
//Parameters:
//		fileName：请求访问的文件路径
//Return: 文件类型
const char* getFileType(const char* fileName)
{
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char* dot = strrchr(fileName, '.');   
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
	if (strcmp(dot, ".pdf") == 0)
        return "application/pdf";
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


//Summary:通过状态码获得状态描述
//Parameters:
//		statusCode：状态码
//Return: 文件类型
const char* getReasonPhrase(int statusCode)
{
	switch (statusCode)
	{
		case 100:
			return "Continue";
			break;
		case 101:
			return "Switching Protocols";
			break;
		case 200:
			return "OK";
			break;
		case 201:
			return "Created";
			break;
		case 202:
			return "Accepted";
			break;
		case 203:
			return "Non-Authoritative information";
			break;
		case 204:
			return "No Content";
			break;
		case 205:
			return "Reset Content";
			break;
		case 206:
			return "Partial Content";
			break;
		case 300:
			return "Multiple Choices";
			break;
		case 301:
			return "Moved Permanently";
			break;
		case 302:
			return "Found";
			break;
		case 303:
			return "See Other";
			break;
		case 304:
			return "Not Modified";
			break;
		case 305:
			return "Use Proxy";
			break;
		case 306:
			return "unused";
			break;
		case 307:
			return "Temporary Redirect）";
			break;
		case 400:
			return "Bad Request";
			break;
		case 401:
			return "Unauthorized";
			break;
		case 402:
			return "Payment Required";
			break;
		case 403:
			return "Forbidden";
			break;
		case 404:
			return "Not Found";
			break;
		case 405:
			return "Method Not Allowe";
			break;
		case 406:
			return "Not Acceptable";
			break;
		case 407:
			return "Proxy Authentication Required";
			break;
		case 408:
			return "Request Timeout";
			break;
		case 409:
			return "Confilict";
			break;
		case 410:
			return "gone";
			break;
		case 411:
			return "Length Required";
			break;
		case 412:
			return "Precondition Failed";
			break;
		case 413:
			return "Request Entity TooLarge";
			break;
		case 414:
			return "Request-URI Too Long";
			break;
		case 415:
			return "Unsupported Media Type";
			break;
		case 416:
			return "Requested Range Not Satisfiable";
			break;
		case 417:
			return "Expectation Failed";
			break;
		case 500:
			return "Internal Server Error";
			break;
		case 501:
			return "Not Implemented";
			break;
		case 502:
			return "Bad Gateway";
			break;
		case 503:
			return "Service Unavailable";
			break;
		case 504:
			return "Gateway Timeout";
			break;
		case 505:
			return "HTTP version Not Supported";
			break;
		default:
			return "NMSL";
	}
}



//Summary:将16进制数转化为10进制
//Parameters:
//		c：16进制数
//Return: 10进制数
int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}


//Summary:编码，utf-8 -> url
//Parameters:
//		to：编码后的字符串
//		tosize：编码后的字符串的长度
//		from：编码前的字符串
//Return: void
void encodeStr(char* to, int tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {    
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0) {      
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}

//Summary:解码，  url -> utf-8
//Parameters:
//		to：解码后的字符串
//		from：解码前的字符串
//Return: void
void decodeStr(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from  ) {     
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {       
            *to = hexit(from[1])*16 + hexit(from[2]);
            from += 2;                      
        } else {
            *to = *from;
        }
    }
    *to = '\0';
}


