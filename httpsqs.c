/*
HTTP Simple Queue Service - httpsqs v1.7
Author: Zhang Yan (http://blog.s135.com), E-mail: net@s135.com
This is free software, and you are welcome to modify and redistribute it under the New BSD License
*/

#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>

#include <err.h>
#include <event.h>
#include <evhttp.h>

#include <tcbdb.h>

#include "prename.h"

#define VERSION "1.7"

/* 每个队列的默认最大长度为100万条 */
#define HTTPSQS_DEFAULT_MAXQUEUE 1000000

/* 全局设置 */
TCBDB *httpsqs_db_tcbdb; /* 数据表 */
int httpsqs_settings_syncinterval; /* 同步更新内容到磁盘的间隔时间 */
char *httpsqs_settings_pidfile; /* PID文件 */
char *httpsqs_settings_auth; /* 验证密码 */

/* 创建多层目录的函数 */
void create_multilayer_dir( char *muldir ) 
{
    int    i,len;
    char    str[512];
    
    strncpy( str, muldir, 512 );
    len=strlen(str);
    for( i=0; i<len; i++ )
    {
        if( str[i]=='/' )
        {
            str[i] = '\0';
            //判断此目录是否存在,不存在则创建
            if( access(str, F_OK)!=0 )
            {
                mkdir( str, 0777 );
            }
            str[i]='/';
        }
    }
    if( len>0 && access(str, F_OK)!=0 )
    {
        mkdir( str, 0777 );
    }

    return;
}

char *urldecode(char *input_str) 
{
		int len = strlen(input_str);
		char *str = strdup(input_str);
		
        char *dest = str; 
        char *data = str; 

        int value; 
        int c; 

        while (len--) { 
                if (*data == '+') { 
                        *dest = ' '; 
                } 
                else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) 
  && isxdigit((int) *(data + 2))) 
                { 

                        c = ((unsigned char *)(data+1))[0]; 
                        if (isupper(c)) 
                                c = tolower(c); 
                        value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16; 
                        c = ((unsigned char *)(data+1))[1]; 
                        if (isupper(c)) 
                                c = tolower(c); 
                                value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10; 

                        *dest = (char)value ; 
                        data += 2; 
                        len -= 2; 
                } else { 
                        *dest = *data; 
                } 
                data++; 
                dest++; 
        } 
        *dest = '\0'; 
        return str; 
}

/* 读取队列写入点的值 */
static int httpsqs_read_putpos(const char* httpsqs_input_name)
{
	int queue_value = 0;
	char *queue_value_tmp;
	char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
	
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "putpos");
	
	queue_value_tmp = tcbdbget2(httpsqs_db_tcbdb, queue_name);
	if(queue_value_tmp){
		queue_value = atoi(queue_value_tmp);
		free(queue_value_tmp);
	}
	
	return queue_value;
}

/* 读取队列读取点的值 */
static int httpsqs_read_getpos(const char* httpsqs_input_name)
{
	int queue_value = 0;
	char *queue_value_tmp;
	char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
	
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "getpos");
	
	queue_value_tmp = tcbdbget2(httpsqs_db_tcbdb, queue_name);
	if(queue_value_tmp){
		queue_value = atoi(queue_value_tmp);
		free(queue_value_tmp);
	}
	
	return queue_value;
}

/* 读取用于设置的最大队列数 */
static int httpsqs_read_maxqueue(const char* httpsqs_input_name)
{
	int queue_value = 0;
	char *queue_value_tmp;
	char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
	
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "maxqueue");
	
	queue_value_tmp = tcbdbget2(httpsqs_db_tcbdb, queue_name);
	if(queue_value_tmp){
		queue_value = atoi(queue_value_tmp);
		free(queue_value_tmp);
	} else {
		queue_value = HTTPSQS_DEFAULT_MAXQUEUE; /* 默认队列长度 */
	}
	
	return queue_value;
}

/* 设置最大的队列数量，返回值为设置的队列数量。如果返回值为0，则表示设置取消（取消原因为：设置的最大的队列数量小于”当前队列写入位置点“和”当前队列读取位置点“，或者”当前队列写入位置点“小于”当前队列的读取位置点） */
static int httpsqs_maxqueue(const char* httpsqs_input_name, int httpsqs_input_num)
{
	int queue_put_value = 0;
	int queue_get_value = 0;
	int queue_maxnum_int = 0;
	
	/* 读取当前队列写入位置点 */
	queue_put_value = httpsqs_read_putpos(httpsqs_input_name);
	
	/* 读取当前队列读取位置点 */
	queue_get_value = httpsqs_read_getpos(httpsqs_input_name);

	/* 设置最大的队列数量，最小值为10条，最大值为10亿条 */
	queue_maxnum_int = httpsqs_input_num;
	
	/* 设置的最大的队列数量必须大于等于”当前队列写入位置点“和”当前队列读取位置点“，并且”当前队列写入位置点“必须大于等于”当前队列读取位置点“ */
	if (queue_maxnum_int >= queue_put_value && queue_maxnum_int >= queue_get_value && queue_put_value >= queue_get_value) {
		char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
		char queue_maxnum[16] = {0};
		sprintf(queue_name, "%s:%s", httpsqs_input_name, "maxqueue");
		sprintf(queue_maxnum, "%d", queue_maxnum_int);
		tcbdbput2(httpsqs_db_tcbdb, queue_name, queue_maxnum);
		
		tcbdbsync(httpsqs_db_tcbdb); /* 实时刷新到磁盘 */
		
		return queue_maxnum_int;
	}
	
	return 0;
}

/* 重置队列，0表示重置成功 */
static int httpsqs_reset(const char* httpsqs_input_name)
{
	char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
	
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "putpos");
	tcbdbout2(httpsqs_db_tcbdb, queue_name);

	memset(queue_name, '\0', 300);
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "getpos");
	tcbdbout2(httpsqs_db_tcbdb, queue_name);
	
	memset(queue_name, '\0', 300);
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "maxqueue");
	tcbdbout2(httpsqs_db_tcbdb, queue_name);
	
	tcbdbsync(httpsqs_db_tcbdb); /* 实时刷新到磁盘 */
	
	return 0;
}

/* 查看单条队列内容 */
char *httpsqs_view(const char* httpsqs_input_name, int pos)
{
	char *queue_value;
	char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
	
	sprintf(queue_name, "%s:%d", httpsqs_input_name, pos);
	
	queue_value = tcbdbget2(httpsqs_db_tcbdb, queue_name);
	
	return queue_value;
}

/* 修改定时更新内存内容到磁盘的间隔时间，返回间隔时间（秒） */
static int httpsqs_synctime(int httpsqs_input_num)
{
	if (httpsqs_input_num >= 1) {
		httpsqs_settings_syncinterval = httpsqs_input_num;
	}
	return httpsqs_settings_syncinterval;
}

/* 获取本次“入队列”操作的队列写入点 */
static int httpsqs_now_putpos(const char* httpsqs_input_name)
{
	int maxqueue_num = 0;
	int queue_put_value = 0;
	int queue_get_value = 0;
	char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
	char queue_input[32] = {0};
	
	/* 获取最大队列数量 */
	maxqueue_num = httpsqs_read_maxqueue(httpsqs_input_name);
	
	/* 读取当前队列写入位置点 */
	queue_put_value = httpsqs_read_putpos(httpsqs_input_name);
	
	/* 读取当前队列读取位置点 */
	queue_get_value = httpsqs_read_getpos(httpsqs_input_name);	
	
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "putpos");	
	
	/* 队列写入位置点加1 */
	queue_put_value = queue_put_value + 1;
	if (queue_put_value == queue_get_value) { /* 如果队列写入ID+1之后追上队列读取ID，则说明队列已满，返回0，拒绝继续写入 */
		queue_put_value = 0;
	}
	else if (queue_get_value <= 1 && queue_put_value > maxqueue_num) { /* 如果队列写入ID大于最大队列数量，并且从未进行过出队列操作（=0）或进行过1次出队列操作（=1），返回0，拒绝继续写入 */
		queue_put_value = 0;
	}	
	else if (queue_put_value > maxqueue_num) { /* 如果队列写入ID大于最大队列数量，则重置队列写入位置点的值为1 */
		if(tcbdbput2(httpsqs_db_tcbdb, queue_name, "1")) {
			queue_put_value = 1;
		}
	} else { /* 队列写入位置点加1后的值，回写入数据库 */
		sprintf(queue_input, "%d", queue_put_value);
		tcbdbput2(httpsqs_db_tcbdb, queue_name, (char *)queue_input);
	}
	
	return queue_put_value;
}

/* 获取本次“出队列”操作的队列读取点，返回值为0时队列全部读取完成 */
static int httpsqs_now_getpos(const char* httpsqs_input_name)
{
	int maxqueue_num = 0;
	int queue_put_value = 0;
	int queue_get_value = 0;
	char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
	
	/* 获取最大队列数量 */
	maxqueue_num = httpsqs_read_maxqueue(httpsqs_input_name);
	
	/* 读取当前队列写入位置点 */
	queue_put_value = httpsqs_read_putpos(httpsqs_input_name);
	
	/* 读取当前队列读取位置点 */
	queue_get_value = httpsqs_read_getpos(httpsqs_input_name);
	
	/* 如果queue_get_value的值不存在，重置队列读取位置点为1 */
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "getpos");
	/* 如果queue_get_value的值不存在，重置为1 */
	if (queue_get_value == 0 && queue_put_value > 0) {
		queue_get_value = 1;
		tcbdbput2(httpsqs_db_tcbdb, queue_name, "1");
	/* 如果队列的读取值（出队列）小于队列的写入值（入队列） */
	} else if (queue_get_value < queue_put_value) {
		queue_get_value = queue_get_value + 1;
		char queue_input[32] = {0};
		sprintf(queue_input, "%d", queue_get_value);
		tcbdbput2(httpsqs_db_tcbdb, queue_name, queue_input);
	/* 如果队列的读取值（出队列）大于队列的写入值（入队列），并且队列的读取值（出队列）小于最大队列数量 */
	} else if (queue_get_value > queue_put_value && queue_get_value < maxqueue_num) {
		queue_get_value = queue_get_value + 1;
		char queue_input[32] = {0};
		sprintf(queue_input, "%d", queue_get_value);
		tcbdbput2(httpsqs_db_tcbdb, queue_name, queue_input);
	/* 如果队列的读取值（出队列）大于队列的写入值（入队列），并且队列的读取值（出队列）等于最大队列数量 */
	} else if (queue_get_value > queue_put_value && queue_get_value == maxqueue_num) {
		queue_get_value = 1;
		tcbdbput2(httpsqs_db_tcbdb, queue_name, "1");
	/* 队列的读取值（出队列）等于队列的写入值（入队列），即队列中的数据已全部读出 */
	} else {
		queue_get_value = 0;
	}
	
	return queue_get_value;
}

/* 处理模块 */
void httpsqs_handler(struct evhttp_request *req, void *arg)
{
        struct evbuffer *buf;
        buf = evbuffer_new();
		
		/* 分析URL参数 */	
		const char *httpsqs_query_part;
		struct evkeyvalq httpsqs_http_query;
		httpsqs_query_part = evhttp_uri_get_query(req->uri_elems);
		evhttp_parse_query_str(httpsqs_query_part, &httpsqs_http_query);
		
		/* 接收GET表单参数 */
		const char *httpsqs_input_auth = evhttp_find_header (&httpsqs_http_query, "auth"); /* 队列名称 */
		const char *httpsqs_input_name = evhttp_find_header (&httpsqs_http_query, "name"); /* 队列名称 */
		const char *httpsqs_input_charset = evhttp_find_header (&httpsqs_http_query, "charset"); /* 操作类别 */
		const char *httpsqs_input_opt = evhttp_find_header (&httpsqs_http_query, "opt"); /* 操作类别 */
		const char *httpsqs_input_data = evhttp_find_header (&httpsqs_http_query, "data"); /* 操作类别 */
		const char *httpsqs_input_pos_tmp = evhttp_find_header (&httpsqs_http_query, "pos"); /* 队列位置点 字符型 */
		const char *httpsqs_input_num_tmp = evhttp_find_header (&httpsqs_http_query, "num"); /* 队列总长度 字符型 */
		int httpsqs_input_pos = 0;
		int httpsqs_input_num = 0;
		
		/* 返回给用户的Header头信息 */
		if (httpsqs_input_charset != NULL && strlen(httpsqs_input_charset) <= 40) {
			char content_type[64] = {0};
			sprintf(content_type, "text/plain; charset=%s", httpsqs_input_charset);
			evhttp_add_header(req->output_headers, "Content-Type", content_type);
		} else {
			evhttp_add_header(req->output_headers, "Content-Type", "text/plain");
		}
		evhttp_add_header(req->output_headers, "Connection", "keep-alive");
		evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");
		//evhttp_add_header(req->output_headers, "Connection", "close");		
		
		/* 权限校验 */
		bool is_auth_pass = false; /* 是否验证通过 */
		if (httpsqs_settings_auth != NULL) {
			/* 如果命令行启动参数设置了验证密码 */
			if (httpsqs_input_auth != NULL && strcmp(httpsqs_settings_auth, httpsqs_input_auth) == 0) {
				is_auth_pass = true;
			} else {
				is_auth_pass = false;
			}
		} else {
			/* 如果命令行启动参数没有设置验证密码 */
			is_auth_pass = true;
		}

		if (is_auth_pass == false) {
			/* 校验失败 */
			evbuffer_add_printf(buf, "%s", "HTTPSQS_AUTH_FAILED");
		}
		else 
		{
			/* 校验成功，或者命令行启动参数没有设置校验密码 */
			if (httpsqs_input_pos_tmp != NULL) {
				httpsqs_input_pos = atoi(httpsqs_input_pos_tmp); /* 队列位置点 数值型 */
			}
			if (httpsqs_input_num_tmp != NULL) {
				httpsqs_input_num = atoi(httpsqs_input_num_tmp); /* 队列总长度 数值型 */
			}
	
			/*参数是否存在判断 */
			if (httpsqs_input_name != NULL && httpsqs_input_opt != NULL && strlen(httpsqs_input_name) <= 256) {
				/* 入队列 */
				if (strcmp(httpsqs_input_opt, "put") == 0) {
					/* 优先接收POST正文信息 */
					int buffer_data_len;
					buffer_data_len = EVBUFFER_LENGTH(req->input_buffer);
					if (buffer_data_len > 0) {
						int queue_put_value = httpsqs_now_putpos((char *)httpsqs_input_name);
						if (queue_put_value > 0) {
							char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
							sprintf(queue_name, "%s:%d", httpsqs_input_name, queue_put_value);
							char *httpsqs_input_postbuffer;					
							char *buffer_data = (char *)tccalloc(1, buffer_data_len + 1);
							memcpy (buffer_data, EVBUFFER_DATA(req->input_buffer), buffer_data_len);
							httpsqs_input_postbuffer = urldecode(buffer_data);
							tcbdbput2(httpsqs_db_tcbdb, queue_name, httpsqs_input_postbuffer);
							memset(queue_name, '\0', 300);
							sprintf(queue_name, "%d", queue_put_value);					
							evhttp_add_header(req->output_headers, "Pos", queue_name);
							evbuffer_add_printf(buf, "%s", "HTTPSQS_PUT_OK");
							free(httpsqs_input_postbuffer);
							free(buffer_data);
						} else {
							evbuffer_add_printf(buf, "%s", "HTTPSQS_PUT_END");
						}
					/* 如果POST正文无内容，则取URL中data参数的值 */
					} else if (httpsqs_input_data != NULL) {
						int queue_put_value = httpsqs_now_putpos((char *)httpsqs_input_name);
						if (queue_put_value > 0) {
							char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
							sprintf(queue_name, "%s:%d", httpsqs_input_name, queue_put_value);				
							buffer_data_len = strlen(httpsqs_input_data);
							char *httpsqs_input_postbuffer;
							char *buffer_data = (char *)tccalloc(1, buffer_data_len + 1);					
							memcpy (buffer_data, httpsqs_input_data, buffer_data_len);
							httpsqs_input_postbuffer = urldecode(buffer_data);
							tcbdbput2(httpsqs_db_tcbdb, queue_name, httpsqs_input_postbuffer);
							memset(queue_name, '\0', 300);
							sprintf(queue_name, "%d", queue_put_value);					
							evhttp_add_header(req->output_headers, "Pos", queue_name);
							evbuffer_add_printf(buf, "%s", "HTTPSQS_PUT_OK");
							free(httpsqs_input_postbuffer);
							free(buffer_data);
						} else {
							evbuffer_add_printf(buf, "%s", "HTTPSQS_PUT_END");
						}
					} else {
						evbuffer_add_printf(buf, "%s", "HTTPSQS_PUT_ERROR");
					}
				}
				/* 出队列 */
				else if (strcmp(httpsqs_input_opt, "get") == 0) {
					int queue_get_value = 0;
					queue_get_value = httpsqs_now_getpos((char *)httpsqs_input_name);
					if (queue_get_value == 0) {
						evbuffer_add_printf(buf, "%s", "HTTPSQS_GET_END");
					} else {
						char queue_name[300] = {0}; /* 队列名称的总长度，用户输入的队列长度少于256字节 */
						sprintf(queue_name, "%s:%d", httpsqs_input_name, queue_get_value);
						char *httpsqs_output_value;
						httpsqs_output_value = tcbdbget2(httpsqs_db_tcbdb, queue_name);
						if (httpsqs_output_value) {
							memset(queue_name, '\0', 300);
							sprintf(queue_name, "%d", queue_get_value);	
							evhttp_add_header(req->output_headers, "Pos", queue_name);
							evbuffer_add_printf(buf, "%s", httpsqs_output_value);
							free(httpsqs_output_value);
						} else {
							evbuffer_add_printf(buf, "%s", "HTTPSQS_GET_END");
						}
					}
				}
				/* 查看队列状态（普通浏览方式） */
				else if (strcmp(httpsqs_input_opt, "status") == 0) {
					int maxqueue = httpsqs_read_maxqueue((char *)httpsqs_input_name); /* 最大队列数量 */
					int putpos = httpsqs_read_putpos((char *)httpsqs_input_name); /* 入队列写入位置 */
					int getpos = httpsqs_read_getpos((char *)httpsqs_input_name); /* 出队列读取位置 */
					int ungetnum;
					const char *put_times;
					const char *get_times;
					if (putpos >= getpos) {
						ungetnum = abs(putpos - getpos); /* 尚未出队列条数 */
						put_times = "1st lap";
						get_times = "1st lap";
					} else if (putpos < getpos) {
						ungetnum = abs(maxqueue - getpos + putpos); /* 尚未出队列条数 */
						put_times = "2nd lap";
						get_times = "1st lap";
					}
					evbuffer_add_printf(buf, "HTTP Simple Queue Service v%s\n", VERSION);
					evbuffer_add_printf(buf, "------------------------------\n");
					evbuffer_add_printf(buf, "Queue Name: %s\n", httpsqs_input_name);
					evbuffer_add_printf(buf, "Maximum number of queues: %d\n", maxqueue);
					evbuffer_add_printf(buf, "Put position of queue (%s): %d\n", put_times, putpos);
					evbuffer_add_printf(buf, "Get position of queue (%s): %d\n", get_times, getpos);
					evbuffer_add_printf(buf, "Number of unread queue: %d\n", ungetnum);
				}
				/* 查看队列状态（JSON方式，方便客服端程序处理） */
				else if (strcmp(httpsqs_input_opt, "status_json") == 0) {
					int maxqueue = httpsqs_read_maxqueue((char *)httpsqs_input_name); /* 最大队列数量 */
					int putpos = httpsqs_read_putpos((char *)httpsqs_input_name); /* 入队列写入位置 */
					int getpos = httpsqs_read_getpos((char *)httpsqs_input_name); /* 出队列读取位置 */
					int ungetnum;
					const char *put_times;
					const char *get_times;
					if (putpos >= getpos) {
						ungetnum = abs(putpos - getpos); /* 尚未出队列条数 */
						put_times = "1";
						get_times = "1";
					} else if (putpos < getpos) {
						ungetnum = abs(maxqueue - getpos + putpos); /* 尚未出队列条数 */
						put_times = "2";
						get_times = "1";
					}
					evbuffer_add_printf(buf, "{\"name\":\"%s\",\"maxqueue\":%d,\"putpos\":%d,\"putlap\":%s,\"getpos\":%d,\"getlap\":%s,\"unread\":%d}\n", httpsqs_input_name, maxqueue, putpos, put_times, getpos, get_times, ungetnum);
				}			
				/* 查看单条队列内容 */
				else if (strcmp(httpsqs_input_opt, "view") == 0 && httpsqs_input_pos >= 1 && httpsqs_input_pos <= 1000000000) {
					char *httpsqs_output_value;
					httpsqs_output_value = httpsqs_view ((char *)httpsqs_input_name, httpsqs_input_pos);
					if (httpsqs_output_value) {
						evbuffer_add_printf(buf, "%s", httpsqs_output_value);
						free(httpsqs_output_value);
					}
				}
				/* 重置队列 */
				else if (strcmp(httpsqs_input_opt, "reset") == 0) {
					int reset = httpsqs_reset((char *)httpsqs_input_name);
					if (reset == 0) {
						evbuffer_add_printf(buf, "%s", "HTTPSQS_RESET_OK");
					} else {
						evbuffer_add_printf(buf, "%s", "HTTPSQS_RESET_ERROR");
					}
				}
				/* 设置最大的队列数量，最小值为10条，最大值为10亿条 */
				else if (strcmp(httpsqs_input_opt, "maxqueue") == 0 && httpsqs_input_num >= 10 && httpsqs_input_num <= 1000000000) {
					if (httpsqs_maxqueue((char *)httpsqs_input_name, httpsqs_input_num) != 0) {
						/* 设置成功 */
						evbuffer_add_printf(buf, "%s", "HTTPSQS_MAXQUEUE_OK");
					} else {
						/* 设置取消 */
						evbuffer_add_printf(buf, "%s", "HTTPSQS_MAXQUEUE_CANCEL");
					}
				}
				/* 设置定时更新内存内容到磁盘的间隔时间，最小值为1秒，最大值为10亿秒 */
				else if (strcmp(httpsqs_input_opt, "synctime") == 0 && httpsqs_input_num >= 1 && httpsqs_input_num <= 1000000000) {
					if (httpsqs_synctime(httpsqs_input_num) >= 1) {
						/* 设置成功 */
						evbuffer_add_printf(buf, "%s", "HTTPSQS_SYNCTIME_OK");
					} else {
						/* 设置取消 */
						evbuffer_add_printf(buf, "%s", "HTTPSQS_SYNCTIME_CANCEL");
					}				
				} else {
					/* 命令错误 */
					evbuffer_add_printf(buf, "%s", "HTTPSQS_ERROR");				
				}
			} else {
				/* 命令错误 */
				evbuffer_add_printf(buf, "%s", "HTTPSQS_ERROR");
			}
		}
	
		/* 输出内容给客户端 */
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
		
		/* 内存释放 */
		evhttp_clear_headers(&httpsqs_http_query);
		evbuffer_free(buf);
}

/* 子进程信号处理 */
static void kill_signal_worker(const int sig) {
	/* 同步内存数据到磁盘，并关闭数据库 */
	tcbdbsync(httpsqs_db_tcbdb);
	tcbdbclose(httpsqs_db_tcbdb);
	tcbdbdel(httpsqs_db_tcbdb);
	
    exit(0);
}

/* 父进程信号处理 */
static void kill_signal_master(const int sig) {
	/* 删除PID文件 */
	remove(httpsqs_settings_pidfile);

    /* 给进程组发送SIGTERM信号，结束子进程 */
    kill(0, SIGTERM);
	
    exit(0);
}

/* 定时同步线程，定时将内存中的内容写入磁盘 */
static void sync_worker(const int sig) {
	pthread_detach(pthread_self());

	while(1)
	{
		/* 间隔httpsqs_settings_syncinterval秒同步一次数据到磁盘 */
		sleep(httpsqs_settings_syncinterval);	
	
		/* 同步内存数据到磁盘 */
		tcbdbsync(httpsqs_db_tcbdb);
	}
}

static void show_help(void)
{
	char *b = "--------------------------------------------------------------------------------------------------\n"
		  "HTTP Simple Queue Service - httpsqs v" VERSION " (April 14, 2011)\n\n"
		  "Author: Zhang Yan (http://blog.s135.com), E-mail: net@s135.com\n"
		  "This is free software, and you are welcome to modify and redistribute it under the New BSD License\n"
		  "\n"
		   "-l <ip_addr>  interface to listen on, default is 0.0.0.0\n"
		   "-p <num>      TCP port number to listen on (default: 1218)\n"
		   "-x <path>     database directory (example: /opt/httpsqs/data)\n"
		   "-t <second>   keep-alive timeout for an http request (default: 60)\n"
		   "-s <second>   the interval to sync updated contents to the disk (default: 5)\n"
		   "-c <num>      the maximum number of non-leaf nodes to be cached (default: 1024)\n"
		   "-m <size>     database memory cache size in MB (default: 100)\n"
		   "-i <file>     save PID in <file> (default: /tmp/httpsqs.pid)\n"
		   "-a <auth>     the auth password to access httpsqs (example: mypass123)\n"
		   "-d            run as a daemon\n"
		   "-h            print this help and exit\n\n"
		   "Use command \"killall httpsqs\", \"pkill httpsqs\" and \"kill `cat /tmp/httpsqs.pid`\" to stop httpsqs.\n"
		   "Please note that don't use the command \"pkill -9 httpsqs\" and \"kill -9 PID of httpsqs\"!\n"
		   "\n"
		   "Please visit \"http://code.google.com/p/httpsqs\" for more help information.\n\n"
		   "--------------------------------------------------------------------------------------------------\n"
		   "\n";
	fprintf(stderr, b, strlen(b));
}

int main(int argc, char *argv[], char *envp[])
{
	int c;
	/* 默认参数设置 */
	char *httpsqs_settings_listen = "0.0.0.0";
	int httpsqs_settings_port = 1218;
	char *httpsqs_settings_datapath = NULL;
	bool httpsqs_settings_daemon = false;
	int httpsqs_settings_timeout = 60; /* 单位：秒 */
	httpsqs_settings_syncinterval = 5; /* 单位：秒 */
	int httpsqs_settings_cachenonleaf = 1024; /* 缓存非叶子节点数。单位：条 */
	int httpsqs_settings_cacheleaf = 2048; /* 缓存叶子节点数。叶子节点缓存数为非叶子节点数的两倍。单位：条 */
	int httpsqs_settings_mappedmemory = 104857600; /* 单位：字节 */
	httpsqs_settings_pidfile = "/tmp/httpsqs.pid";
	httpsqs_settings_auth = NULL; /* 验证密码 */
	
	/* 命令行参数，暂时存储下面，便于进程重命名 */
	int httpsqs_prename_num = 1;
	char httpsqs_path_file[1024] = { 0 }; // httpsqs_path_file 为 httpsqs 程序的绝对路径
	struct evbuffer *httpsqs_prename_buf; /* 原命令行参数 */
    httpsqs_prename_buf = evbuffer_new();
	readlink("/proc/self/exe", httpsqs_path_file, sizeof(httpsqs_path_file));
	evbuffer_add_printf(httpsqs_prename_buf, "%s", httpsqs_path_file);
	for (httpsqs_prename_num = 1; httpsqs_prename_num < argc; httpsqs_prename_num++) {
		evbuffer_add_printf(httpsqs_prename_buf, " %s", argv[httpsqs_prename_num]);
	}

    /* process arguments */
    while ((c = getopt(argc, argv, "l:p:x:t:s:c:m:i:a:dh")) != -1) {
        switch (c) {
        case 'l':
            httpsqs_settings_listen = strdup(optarg);
            break;
        case 'p':
            httpsqs_settings_port = atoi(optarg);
            break;
        case 'x':
            httpsqs_settings_datapath = strdup(optarg); /* httpsqs数据库文件存放路径 */
			if (access(httpsqs_settings_datapath, W_OK) != 0) { /* 如果目录不可写 */
				if (access(httpsqs_settings_datapath, R_OK) == 0) { /* 如果目录可读 */
					chmod(httpsqs_settings_datapath, S_IWOTH); /* 设置其他用户具可写入权限 */
				} else { /* 如果不存在该目录，则创建 */
					create_multilayer_dir(httpsqs_settings_datapath);
				}
				
				if (access(httpsqs_settings_datapath, W_OK) != 0) { /* 如果目录不可写 */
					fprintf(stderr, "httpsqs database directory not writable\n");
				}
			}
            break;
        case 't':
            httpsqs_settings_timeout = atoi(optarg);
            break;
        case 's':
            httpsqs_settings_syncinterval = atoi(optarg);
            break;
        case 'c':
            httpsqs_settings_cachenonleaf = atoi(optarg);
			httpsqs_settings_cacheleaf = httpsqs_settings_cachenonleaf * 2;
            break;
        case 'm':
            httpsqs_settings_mappedmemory = atoi(optarg) * 1024 * 1024; /* 单位：M */
            break;
        case 'i':
            httpsqs_settings_pidfile = strdup(optarg);
            break;
        case 'a':
            httpsqs_settings_auth = strdup(optarg);
            break;			
        case 'd':
            httpsqs_settings_daemon = true;
            break;
		case 'h':
        default:
            show_help();
            return 1;
        }
    }
	
	/* 判断是否加了必填参数 -x */
	if (httpsqs_settings_datapath == NULL) {
		show_help();
		fprintf(stderr, "Attention: Please use the indispensable argument: -x <path>\n\n");		
		exit(1);
	}
	
	/* 数据表路径 */
	int httpsqs_settings_dataname_len = 1024;
	char *httpsqs_settings_dataname = (char *)tccalloc(1, httpsqs_settings_dataname_len);
	sprintf(httpsqs_settings_dataname, "%s/httpsqs.db", httpsqs_settings_datapath);

	/* 打开数据表 */
	httpsqs_db_tcbdb = tcbdbnew();
	tcbdbsetmutex(httpsqs_db_tcbdb); /* 开启线程互斥锁 */
	tcbdbtune(httpsqs_db_tcbdb, 1024, 2048, 50000000, 8, 10, BDBTLARGE);
	tcbdbsetcache(httpsqs_db_tcbdb, httpsqs_settings_cacheleaf, httpsqs_settings_cachenonleaf);
	tcbdbsetxmsiz(httpsqs_db_tcbdb, httpsqs_settings_mappedmemory); /* 内存缓存大小 */
					
	/* 判断表是否能打开 */
	if(!tcbdbopen(httpsqs_db_tcbdb, httpsqs_settings_dataname, BDBOWRITER|BDBOCREAT)){
		show_help();
		fprintf(stderr, "Attention: Unable to open the database.\n\n");		
		exit(1);
	}
	
	/* 释放变量所占内存 */
	free(httpsqs_settings_dataname);
	
	/* 如果加了-d参数，以守护进程运行 */
	if (httpsqs_settings_daemon == true) {
        pid_t pid;

        /* Fork off the parent process */       
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }
	}
	
	/* 将进程号写入PID文件 */
	FILE *fp_pidfile;
	fp_pidfile = fopen(httpsqs_settings_pidfile, "w");
	fprintf(fp_pidfile, "%d\n", getpid());
	fclose(fp_pidfile);
	
    /* 重命名httpsqs主进程，便于ps -ef命令查看 */
    prename_setproctitle_init(argc, argv, envp);
    prename_setproctitle("[httpsqs: master process] %s", (char *)EVBUFFER_DATA(httpsqs_prename_buf));

    /* 派生httpsqs子进程（工作进程） */
    pid_t httpsqs_worker_pid_wait;
    pid_t httpsqs_worker_pid = fork();
    /* 如果派生进程失败，则退出程序 */
    if (httpsqs_worker_pid < 0)
    {
        fprintf(stderr, "Error: %s:%d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
    }
    /* httpsqs父进程内容 */
	if (httpsqs_worker_pid > 0)
	{
		/* 处理父进程接收到的kill信号 */
		
		/* 忽略Broken Pipe信号 */
		signal(SIGPIPE, SIG_IGN);
	
		/* 处理kill信号 */
		signal (SIGINT, kill_signal_master);
		signal (SIGKILL, kill_signal_master);
		signal (SIGQUIT, kill_signal_master);
		signal (SIGTERM, kill_signal_master);
		signal (SIGHUP, kill_signal_master);
	
		/* 处理段错误信号 */
		signal(SIGSEGV, kill_signal_master);

        /* 如果子进程终止，则重新派生新的子进程 */
        while (1)
        {
            httpsqs_worker_pid_wait = wait(NULL);
            if (httpsqs_worker_pid_wait < 0)
            {
                continue;
            }
			usleep(100000);
            httpsqs_worker_pid = fork();
            if (httpsqs_worker_pid == 0)
            {
                break;
            }
		}
	}
	
	/* ---------------以下为httpsqs子进程内容------------------- */
	
	/* 忽略Broken Pipe信号 */
	signal(SIGPIPE, SIG_IGN);
	
	/* 处理kill信号 */
	signal (SIGINT, kill_signal_worker);
	signal (SIGKILL, kill_signal_worker);
	signal (SIGQUIT, kill_signal_worker);
	signal (SIGTERM, kill_signal_worker);
	signal (SIGHUP, kill_signal_worker);
	
    /* 处理段错误信号 */
    signal(SIGSEGV, kill_signal_worker);	
	
	/* 创建定时同步线程，定时将内存中的内容写入磁盘 */
	pthread_t sync_worker_tid;
    pthread_create(&sync_worker_tid, NULL, (void *) sync_worker, NULL);
	
    /* 重命名httpsqs子进程，便于ps -ef命令查看 */
    prename_setproctitle_init(argc, argv, envp);
    prename_setproctitle("[httpsqs: worker process] %s", (char *)EVBUFFER_DATA(httpsqs_prename_buf));	
	evbuffer_free(httpsqs_prename_buf);
	
	/* 请求处理部分 */
    struct evhttp *httpd;

    event_init();
    httpd = evhttp_start(httpsqs_settings_listen, httpsqs_settings_port);
	if (httpd == NULL) {
		fprintf(stderr, "Error: Unable to listen on %s:%d\n\n", httpsqs_settings_listen, httpsqs_settings_port);
		kill(0, SIGTERM);
		exit(1);		
	}
	evhttp_set_timeout(httpd, httpsqs_settings_timeout);

    /* Set a callback for requests to "/specific". */
    /* evhttp_set_cb(httpd, "/select", select_handler, NULL); */

    /* Set a callback for all other requests. */
    evhttp_set_gencb(httpd, httpsqs_handler, NULL);

    event_dispatch();

    /* Not reached in this code as it is now. */
    evhttp_free(httpd);

    return 0;
}
