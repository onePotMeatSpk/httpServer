
#include"threadPool.h"

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

	//创建线程池
	threadPool = threadPoolCreate(3, 100, 100);
	
	//创建反应堆
	epollFd = createReactor(REACTOR_SIZE);

	//反应堆运行
	runReactor();
	
	return 0;
}

