/*
 *本程序用C语言作为Linux守护进程
 *实现每秒读取队列内容平且POST发送给处理端（例如PHP程序）
 *可以修改处理队列数据部分的代码实现其他的数据处理
 *本程序只是一个最初级的作品，本人的C语言水平也很低（在写这个程序前还不会C语言的socket发送HTTP请求）
 *希望张宴大哥给予指正与修改，体现我们开源的优势～哈哈
 *
 *使用方法：
 *gcc -o httpsqs_client http_client.c
 *./httpsqs_client -q your_queue_name -t 20(每秒获取队列的次数)
 *还不会写MAKEFILE呵呵
 *本程序只能实现读取队列内容，对于1.2版本的读取pos还不支持（搞字符串指针，总弄得内存报错……）
 */
#include <unistd.h>
#include <signal.h> 
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdarg.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <time.h>

#define HttpsqsIp "192.168.1.102"
#define HttpsqsPort 1218

#define RemoteIp "121.52.218.137"
#define RemotePort 80

void init_daemon(void);


void init_daemon(void)
{
	int pid;
	int i;
	if(pid = fork())
		exit(0);
	else if(pid < 0)
		exit(1);
	setsid();

	if(pid = fork())
		exit(0);
	else if(pid < 0)
		exit(1);

	for(i = 0; i < NOFILE; ++i)
		close(i);
	chdir("/var");
	umask(0);
	return;
}

int htconnect(char *domain,int port) 
{
	int white_sock;
	struct hostent * site;
	struct sockaddr_in me;
	site = gethostbyname(domain);
	if(site == NULL) return -2;
	white_sock = socket(AF_INET,SOCK_STREAM, 0);
	if(white_sock < 0) return -1;
	memset(&me, 0, sizeof(struct sockaddr_in));
	memcpy(&me.sin_addr, site->h_addr_list[0], site->h_length);
	me.sin_family = AF_INET;
	me.sin_port = htons(port);
	return (connect(white_sock, (struct sockaddr *)&me, sizeof(struct sockaddr)) < 0) ? -1 : white_sock;
}

int htsend(int sock,char *fmt,...)
{
    char BUF[1024];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(BUF,fmt, argptr);
    va_end(argptr);
    return send(sock, BUF, strlen(BUF), 0);
}

char *getresult(char *data)
{
    char * res;
    int i;
    res = (char *)malloc(strlen(data));
    for(i=0; i<strlen(data); i++)
    {
        if(i >= 21)
        {
            res[i-21] = data[i];
        }
    }
    return res;
}

void process(char *queuename, int loop)
{
	FILE * fp;
	int m;
	int black_sock;
    char data[3];
    char * str;
    char pattern[] = "Connection: close";
	char pattern_2[] = "Array";
	int len = 0;
	int i = 0;

	for(m = 0; m < loop; m++)
    {
	    memset(str, 0, strlen(str));
        len = 0;
        i = 0;
        memset(data, 0, strlen(data));

        black_sock = htconnect(HttpsqsIp, HttpsqsPort);
	    if (black_sock < 0) return;

	    htsend(black_sock, "GET /?charset=utf-8&name=%s&opt=get HTTP/1.1\r\n", queuename, 10);
	    htsend(black_sock, "Host: %s\r\n", HttpsqsIp, 10);
	    htsend(black_sock, "Connection: close\r\n", 10);
	    htsend(black_sock, "\r\n", 10);

	    i = 0;
	    while (read(black_sock, data, 1) > 0)
    	{
		    str[i] = data[0];
		    i++;
	    }
	    close(black_sock);
        memset(data, 0, strlen(data));

        strcpy(str, strstr(str, pattern));
        strcpy(str, getresult(str));

	    if(strchr(str, 'H') == NULL)
	    {
		    if((fp = fopen("test.log", "a")) >= 0)
			    fprintf(fp, "GET FROM HTTPSQS %s\r\n", str);

		    black_sock = htconnect(RemoteIp, RemotePort);
		    if (black_sock < 0) return;

		    len = strlen(str) + 5;
		    htsend(black_sock, "POST /index.php?m=data&a=update HTTP/1.1\r\n", 10);
		    htsend(black_sock, "Content-type: application/x-www-form-urlencoded\r\n", 10);
		    htsend(black_sock, "Host: www.oooffice.com\r\n", 10);
		    htsend(black_sock, "Content-Length: %d\r\n", len, 10);
		    htsend(black_sock, "Connection: close\r\n", 10);
		    htsend(black_sock, "\r\n", 10);
		    htsend(black_sock, "data=%s", str, 10);

		    memset(str, 0, strlen(str));
		    i = 0;
		    while (read(black_sock, data, 1)>0)
		    {
                if(data[0] == '|')
                {
                    if(str[i-1] == 'D' && str[i-2] == 'N' && str[i-3] == 'E')
                    {
                        str[i-3] = '\0';
                        break;
                    }
                }
			    str[i] = data[0];
			    i++;
		    }
            memset(data, 0, strlen(data));
            close(black_sock);
		    if(fp >= 0)
		    {
			    if(strlen(str))
			    {
				    strcpy(str, strstr(str, pattern_2));
				    fprintf(fp, "POST TO REMOTE\r\n%s\r\n", str);
			    }
			    fclose(fp);
		    }
	    }
    }
	return;
}

int main(int argc,char **argv) 
{ 
    char queuename[20];
    int loop;

    if(argc <= 4 || strcmp(argv[1], "-q") != 0 || strcmp(argv[3], "-t") != 0)
    {
        printf("You should run program like:\n");
        printf("%s -q queuename -t timespersec\n", argv[0]);
        return;
    }

    strcpy(queuename, argv[2]);
    loop = atoi(argv[4]);

    if(loop < 1)
    {
        printf("The -t(times per second) must be an int number & > 0\n");
        return;
    }

    init_daemon();

    while(1)
    {
        sleep(1);
        process(queuename, loop);
    }
}
