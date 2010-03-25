/*
 *本程序用C语言作为Linux守护进程
 *实现每秒读取队列内容平且POST发送给处理端（例如PHP程序）
 *可以修改处理队列数据部分的代码实现其他的数据处理
 *本程序只是一个最初级的作品，本人的C语言水平也很低（在写这个程序前还不会C语言的socket发送HTTP请求）
 *希望张宴大哥给予指正与修改，体现我们开源的优势～哈哈
 *
 *使用方法：
 *make
 *make install
 *client_httpsqs -q your_queue_name -t 20(每秒获取队列的次数)
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

#define RemoteIp "xxx.xxx.xxx.xxx"//本程序获取队列后的处理是将数据POST给PHP程序 用户可以自行修改那一部分
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
	int black_sock;
    char data[3];
    char str[20][5000];
    char post[5000];
    char pattern[] = "Connection: close";
	int i = 0;
    int j = 0;
    int len = 0;

    char * pos;
    int p = 0;

    int dataline = 8;
    char * queuedata;

    loop --;
    while(loop)
    {
        memset(data, 0, strlen(data));

        black_sock = htconnect(HttpsqsIp, HttpsqsPort);
        if (black_sock < 0) return;

        htsend(black_sock, "GET /?charset=utf-8&name=%s&opt=get HTTP/1.1\r\n", queuename, 10);
        htsend(black_sock, "Host: %s\r\n", HttpsqsIp, 10);
        htsend(black_sock, "Connection: close\r\n", 10);
        htsend(black_sock, "\r\n", 10);

        i = 0;
        j = 0;
        while (read(black_sock, data, 1) > 0)
	    {
            if(data[0] == '\n' && i == 0)
                continue;
            if(data[0] == '\n')
            {
                if(i != 0)
                {
                    str[j][i+1] == '\0';
                    j++;
                    i = 0;
                }
            } else {
                str[j][i] = data[0];
                i++;
            }
        }

        close(black_sock);
        memset(data, 0, strlen(data));

        if(str[3][0] == 'P')
        {
            pos = malloc((strlen(str[3])-5)*sizeof(char));
            for(i=6; i<strlen(str[3]); i++)
            {
                pos[i-6] = str[3][i];
            }
            p = atoi(pos);

            queuedata = malloc(strlen(str[dataline])*sizeof(char));
            strcpy(queuedata, str[dataline]);
        
		    if((fp = fopen("httpsqs_sms.log", "a")) >= 0)
			    fprintf(fp, "GET FROM HTTPSQS:\r\nPOS:%d\nDATA:%s\r\n", p, queuedata);

            //这部分是获取队列内容后的操作部分，queuedata为队列内容，p为队列的Pos
            black_sock = htconnect(RemoteIp, RemotePort);
            if (black_sock < 0) return;
            len = strlen(queuedata) + 5;
            htsend(black_sock, "POST /index.php?m=data&a=update HTTP/1.1\r\n", 10);
            htsend(black_sock, "Content-type: application/x-www-form-urlencoded\r\n", 10);
            htsend(black_sock, "Host: www.oooffice.com\r\n", 10);
            htsend(black_sock, "Content-Length: %d\r\n", len, 10);
            htsend(black_sock, "Connection: close\r\n", 10);
            htsend(black_sock, "\r\n", 10);
            htsend(black_sock, "data=%s", queuedata, 10);
            
            free(queuedata);

            i = 0;
            while (read(black_sock, data, 1)>0)
            {
                if(data[0] == '|')
                {
                    if(post[i-1] == 'D' && post[i-2] == 'N' && post[i-3] == 'E')
                    {
                        post[i-3] = '\0';
                        break;
                    }
                }
                post[i] = data[0];
                i++;
            }
            memset(data, 0, strlen(data));
            close(black_sock);
            if(strlen(post))
            {
			    if(fp >=0)
			    {
				    strcpy(post, strstr(post, "Array"));
				    fprintf(fp, "POST TO REMOTE\r\n%s\r\n", post);
				    fclose(fp);
			    }
            }
        }
        memset(post, 0, strlen(post));
        loop --;
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
