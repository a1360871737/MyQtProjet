#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <jpeglib.h>
#include <math.h>
#include <sys/ioctl.h>

typedef struct Filesend
{
        int recive;
        int client_fd;
        int id;
}SENDFILESOCK;

typedef struct FileData
{
        char szFilename[1024];
        int client_fd;
        char *pcbuffer;
        struct epoll_event *events;
}FILEDATA;

void log_info(const char* message) {
    time_t current_time;
    char* c_time_string;
    FILE* log_file;

    current_time = time(NULL);

    c_time_string = ctime(&current_time);

    log_file = fopen("log.txt", "a");
    if (log_file == NULL) {
        perror("Error opening file");
        return;
    }

    fprintf(log_file, "[%s] %s\n", c_time_string, message);

    fclose(log_file);
}

#define PORT 8888       
#define MAX_BUFFER 1025 
#define MAX_EVENTS 1024

int dbdata(char *szUsr, char *szPass)
{
        sqlite3 *db;
    sqlite3_open("test.db", &db);

    const char *sql = "SELECT sql FROM sqlite_master WHERE type='table' AND sql LIKE '%BEGIN IMMEDIATE%'";
        char *err_msg = 0;
    sqlite3_stmt *stmt;
        char szCommond[200] = {0};
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to prepare SQL statement: %d\n", rc);
        return 1;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("Database is locked\n");
    }
        sqlite3_finalize(stmt);

    rc = sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to lock database: %d\n", rc);
        return 1;
    }

    sprintf(szCommond,"INSERT INTO user (username, password, phone, addr)"
            "VALUES ('%s', '%s', '18720524450', '0');", szUsr,szPass);
    rc = sqlite3_exec(db, szCommond, 0, 0, &err_msg);                
    if (rc != SQLITE_OK ) {
    printf("SQL err: %s\n", err_msg);
    sqlite3_free(err_msg);
            rc = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
            return 1;
    } else {
        printf("dbAddok\n");
    }

    rc = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to unlock database: %d\n", rc);
        return 1;
    }

    sqlite3_close(db);
    return 0;
}

int datasearch(char *szUsr, char *szPass)
{
        sqlite3 *db;
    sqlite3_open("test.db", &db);
        int flag = 1;
   
    const char *sql = "SELECT sql FROM sqlite_master WHERE type='table' AND sql LIKE '%BEGIN IMMEDIATE%'";
        char *err_msg = 0;
    sqlite3_stmt *stmt;
        sqlite3_stmt *stmtPass;
        char *password =  NULL;
        char szCommond[200] = {0};
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to prepare SQL statement: %d\n", rc);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("Database is locked\n");
    }
        sqlite3_finalize(stmt);

    rc = sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to lock database: %d\n", rc);
    }

        sprintf(szCommond,"SELECT password FROM user WHERE username = '%s';", szUsr);
        rc = sqlite3_prepare_v2(db, szCommond, -1, &stmtPass, &err_msg);                 
        if (rc != SQLITE_OK ) {
        printf("SQL err: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        printf("SearchDBok\n");
    }
        rc = sqlite3_step(stmtPass);
        if (rc == SQLITE_ROW) 
        {
                password = sqlite3_column_text(stmtPass, 0);
                if (0 == strcmp(password, szPass))
                {
                        flag = 0;
                }
                else
                {
                        flag = 2;
                }
                printf("Alice's password is: %s\n", password);
    } 
        else 
        {
        fprintf(stderr, "Error getting result: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmtPass);
    rc = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to unlock database: %d\n", rc);
    }

    sqlite3_close(db);
    return flag;
}

void* handlesendfile(void* pstFiledata) {
    FILEDATA *pstdata = (FILEDATA *)pstFiledata;
    int client_socket = pstdata->client_fd;
    struct epoll_event *events = pstdata->events;
    // 发送文件内容
   FILE* fp = fopen(pstdata->szFilename, "rb");
    if (fp == NULL) {
        printf("Can't open file %s \n", pstdata->szFilename);
        return NULL;
    }

    // 使用 libjpeg 库进行 JPEG 格式压缩处理
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *raw_image = NULL;
    JSAMPROW row_pointer[1];
    int row_stride = 0;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int pixel_size = cinfo.output_components;

    raw_image = (unsigned char*)malloc(width * height * pixel_size);
    row_stride = width * pixel_size;

    while (cinfo.output_scanline < cinfo.output_height) {
        row_pointer[0] = &raw_image[(cinfo.output_scanline) * row_stride];
        jpeg_read_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);

    // 使用 libjpeg 库进行 JPEG 格式压缩，将压缩后的数据发送给客户端
    struct jpeg_compress_struct cinfo_compress;
    struct jpeg_error_mgr jerr_compress;
    unsigned char *compressed_image = NULL;
    unsigned long compressed_size = 0;

    memset(&cinfo_compress, 0, sizeof(cinfo_compress));
    cinfo_compress.err = jpeg_std_error(&jerr_compress);
    jpeg_create_compress(&cinfo_compress);

    FILE *outfile = tmpfile();
    if (outfile == NULL) {
        perror("create outfile failed");
        exit(EXIT_FAILURE);
    }

    jpeg_stdio_dest(&cinfo_compress, outfile);
    // 计算缩放后的目标宽度和高度
    float aspect_ratio = (float) width / (float) height;
    int target_height = (width/4 + width/2) / aspect_ratio;

    cinfo_compress.image_width =  (width/4 + width/2);
    cinfo_compress.image_height = target_height;
    cinfo_compress.input_components = pixel_size;
    cinfo_compress.in_color_space = JCS_RGB;
    jpeg_set_quality(&cinfo_compress, 1, TRUE);

    jpeg_set_defaults(&cinfo_compress);
    jpeg_start_compress(&cinfo_compress, TRUE);

    row_stride = width * pixel_size;
    while (cinfo_compress.next_scanline < cinfo_compress.image_height) {
        row_pointer[0] = &raw_image[cinfo_compress.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo_compress, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo_compress);
    compressed_size = ftell(outfile); // 获取压缩后的文件大小
    compressed_image = (unsigned char*)malloc(compressed_size);
    rewind(outfile);
    fread(compressed_image, 1, compressed_size, outfile);
    fclose(outfile);

    char header[1024] = {0};
    sprintf(header, "HTTP/1.1 200 OK\r\n"
                    "Content-Type: image/jpeg\r\n"
                    "Content-Length: %lu\r\n"
                    "Connection: close\r\n\r\n", compressed_size);
    ssize_t header_len = strlen(header);
    send(client_socket, header, header_len, 0);

    //int ret = send(client_socket, out_buffer, len,0);
    printf("%lld\n", compressed_size);
    int sent_bytes = 0;
    while (sent_bytes < compressed_size) {
        if (events->events & EPOLLOUT)
        {
            // 每次最多发送 MAX_SEGMENT_SIZE 字节的数据
            int bytes_to_send = fmin(compressed_size - sent_bytes, 2048);
            int bytes_sent = send(client_socket, compressed_image + sent_bytes, bytes_to_send, 0);
            if (bytes_sent == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    /* 发送缓冲区已满，等待下一次 EPOLLOUT 事件 */
                    continue;
                } else {
                    perror("send");
                    exit(EXIT_FAILURE);
                }
                perror("Error: Failed to send data to client.\n");
                break;
            }
            sent_bytes += bytes_sent;
        }
    }
    // 释放内存
    free(raw_image);
    free(compressed_image);
    printf("OK");
    #if 0
    int buf_size = 1024*10;
    char buf[1024*10];
    while (1) {
        size_t n = fread(buf, 1, buf_size, fp);
        if (n == 0) break;
        if (send(pstdata->client_fd, buf, n, 0) < 0) {
            log_info("send failde");
            perror("send");
            break;
        }
        usleep(50000);
    }

    fclose(fp);
    #endif
    pthread_exit(NULL);
}

void send_file(int client_socket, const char* filename, struct epoll_event *events) {
 
    FILEDATA *pstFiledata = (FILEDATA *)calloc(sizeof(FILEDATA),1);
    strncpy(pstFiledata->szFilename, filename, strlen(filename));
    pstFiledata->events = events;
    pstFiledata->client_fd = client_socket;
    pthread_t tid;
    if (pthread_create(&tid, NULL, handlesendfile, pstFiledata) != 0 ) {
        printf("Failed to create thread\n");
    }
}

void RevieOK(char *buffer, int *pstfd, int fd, struct epoll_event *events)
{
        int len = strlen(buffer);
        printf("%s   len : %d\n", buffer, len);
        int i = 0;
        {
            char szGet[13] = "GET /File:";
            int cnt = 0;
            for (i = 0; i < 10; i++)
            {
                if (szGet[i] != buffer[i])
                {
                    break;
                }
            }
            if (i == 10)
            {
                char filename[1024] = {0};
                sscanf(buffer, "GET /File:%s HTTP/1.1\r\n", filename);
                printf("Request for file : %s\n",filename);
                send_file(fd, filename, events);
                memset(buffer, 0, MAX_BUFFER);
                return;
            }
        }
        {
                char szSendFile[10] = "sendF:";
                char recieve[1024] = {0};
                int cnt = 0;
                for (i = 0; i < 6; i++)
                {
                    if (szSendFile[i] != buffer[i])
                    {
                        break;
                    }
                }
                if (i == 6)
                {
                        for(i = 6; i  < len; i++)
                        {
                                recieve[cnt++] = buffer[i];
                        }
                        *pstfd = atoi(recieve);
                        printf("%d  recive\n",*pstfd);
                        memset(buffer, 0, MAX_BUFFER);
                        buffer[0] = '2';
                        buffer[1] = '0';
                        buffer[2] = '0';
                        return;
                }

        }
        {
                char szLogin[10] = "login:";
                char szUsr[21] = {0};
                char szPass[21] = {0};
                int cnt = 0;
                for (i = 0; i < 6; i++)
                {
                        if (szLogin[i] != buffer[i])
                        {
                                break;
                        }
                }
                if (i == 6)
                {
                        for(i = 6; i < len; i++)
                        {
                                if (buffer[i] == ';')
                                {
                                        break;
                                }
                                else
                                {
                                        szUsr[cnt++] = buffer[i];
                                }
                        }
                        i++;
                        cnt = 0;
                        while(i < len)
                        {
                                szPass[cnt++] = buffer[i];
                                i++;
                        }
                        printf(" OK  %s %s\n", szUsr, szPass);
                        if (0 == datasearch(szUsr, szPass))
                        {
                                memset(buffer, 0, MAX_BUFFER);
                                buffer[0] = '2';
                                buffer[1] = '0';
                                buffer[2] = '0';
                        }
                        else
                        {
                                memset(buffer, 0, MAX_BUFFER);
                                buffer[0] = '9';
                        }
                        return;
                }
        }
        {
                char szReg[10] = "regin:";
                char szUsr[21] = {0};
                char szPass[21] = {0};
                int cnt = 0;
                for (i = 0; i < 6; i++)
                {
                        if (szReg[i] != buffer[i])
                        {
                                break;
                        }
                }
                if (i == 6)
                {
                        for(i = 6; i < len; i++)
                        {
                                if (buffer[i] == ';')
                                {
                                        break;
                                }
                                else
                                {
                                        szUsr[cnt++] = buffer[i];
                                }
                        }
                        i++;
                        cnt = 0;
                        while(i < len)
                        {
                                szPass[cnt++] = buffer[i];
                                i++;
                        }
                        printf(" OK  %s %s\n", szUsr, szPass);
                        if (2 == datasearch(szUsr, szPass) || 0 == datasearch(szUsr, szPass))
                        {
                                memset(buffer, 0, MAX_BUFFER);
                                buffer[0] = '9';
                                buffer[1] = '9';
                                return;
                        }
                        if (0 == dbdata(szUsr, szPass))
                        {
                                memset(buffer, 0, MAX_BUFFER);
                                buffer[0] = '2';
                                buffer[1] = '0';
                                buffer[2] = '0';
                        }
                        else
                        {
                                memset(buffer, 0, MAX_BUFFER);
                                buffer[0] = '9';
                        }
                        return;
                }
        }
        return;
}

void* handle_client(void* socket_fd) {
        int client_fd = *(int *)socket_fd;
        FILE *fp;
        int recive = 0;
        int total = 0;
        int len = 0;
        char szName[20] = {0};
        sprintf(szName, "recv%d.dat" ,client_fd);
        char buffer[MAX_BUFFER] = {0};
        int valread;
        while(1)
        {
            if (recive != 0)
            {
                    fp = fopen(szName, "wb");  // 打开接收的文件
                    while (total < recive && (len = recv(client_fd, buffer, MAX_BUFFER, 0)) > 0) {
                            total += len;
                            fwrite(buffer, sizeof(char), len, fp);  // 将接收到的数据写入文件中
                    }
                    fclose(fp);
                    if (total >= recive)
                    {
                        send(client_fd, "200", 3, 0);
                        recive = 0;
                        total = 0;
                    }
                    else
                    {
                            send(client_fd, "888", 3, 0);
                    }
            }
            else
            {
                    valread = read(client_fd, buffer, sizeof(buffer));
                    if (valread == 0 || strcmp(buffer, "exit") == 0) {
                            printf("Client disconnected. IP address, Port :\n");
                            break;
                    }
                    printf("Received message: %s\n", buffer);

                    //RevieOK(buffer, &recive, client_fd);
                    send(client_fd, buffer, strlen(buffer), 0);
                    printf("Response sent.\n");

                    memset(buffer, 0, MAX_BUFFER);
            }
        }

        close(client_fd);
        pthread_exit(NULL);
}
int handle_client2(int fd, int *re,int *total, struct epoll_event *events)
{
    char buf[MAX_BUFFER] = {0} ;
    FILE *fp;
    int len = 0;
    char szName[20] = {0};
    sprintf(szName, "recv%d.dat" ,fd);
    char szPercent[32] = {0};
    int recieve = *re;
    if (recieve != 0)
    {
        fp = fopen(szName, "wb");  // 打开接收的文件
        while (*total < recieve && (len = recv(fd, buf, MAX_BUFFER, 0)) > 0) {
                (*total) += len;
                fwrite(buf, sizeof(char), len, fp);  // 将接收到的数据写入文件中
        }
        fclose(fp);
        if (*total >= recieve)
        {
            send(fd, "200", 3, 0);
            *re = 0;
            *total = 0;
        }
        else
        {
            sprintf(szPercent, "percent:%d/%d",(*total),recieve);
            send(fd, szPercent, strlen(szPercent), MSG_DONTWAIT);
        }
    }
    else
    {
        int n = read(fd, buf, MAX_BUFFER);
        if (n < 0) {
            if (errno == ECONNRESET) {
                close(fd);
                events->data.fd = -1;
            } else {
                perror("read error");
            }
        } else if (n == 0) {
            close(fd);
            events->data.fd = -1;
        } else {
            RevieOK(buf, re, fd, events);
            send(fd, buf, strlen(buf), 0);
            printf("recv data: %s\n", buf);
        }
    }
    return ;
}
int main() {
    int listenfd, connfd, epollfd;
    struct sockaddr_in serv_addr, cliaddr;
    socklen_t cliaddr_len = sizeof(cliaddr);
    char buf[MAX_BUFFER];
    struct epoll_event ev, events[MAX_EVENTS];
    int recieve[MAX_EVENTS] = {0};
    int total[MAX_EVENTS] = {0};

    // 创建监听 socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        log_info("socket failde");
        exit(-1);
    }

    // 设置 socket 地址信息
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    // 绑定 socket 和地址
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind error");
        log_info("bind failde");
        exit(-1);
    }

    // 监听 socket
    if (listen(listenfd, SOMAXCONN) < 0) {
        perror("listen error");
        log_info("listen failde");
        exit(-1);
    }

    // 创建 epoll 对象
    epollfd = epoll_create(MAX_EVENTS);
    if (epollfd < 0) {
        perror("epoll_create error");
        log_info("epoll_create failde");
        exit(-1);
    }

    // 添加监听 socket 到 epoll 对象中
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) < 0) {
        perror("epoll_ctl error");
        log_info("epoll_ctl failde");
        exit(-1);
    }

    while (1) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            perror("epoll_wait error");
            log_info("epoll_wait failde");
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listenfd) {
                // 有新连接请求，建立连接，并添加到 epoll 对象中
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
                if (connfd < 0) {
                    perror("accept error");
                    log_info("accept failde");
                    continue;
                }

                // 设置连接 socket 为非阻塞模式，并添加到 epoll 对象中
                if ((fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK)) < 0) {
                    perror("fcntl error");
                    continue;
                }
                ev.events = EPOLLOUT | EPOLLIN | EPOLLET;
                ev.data.fd = connfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
                    perror("epoll_ctl error");
                    continue;
                }

                printf("new connection accepted: fd=%d, ip=%s, port=%d\n", connfd,
                    inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
            } else if (events[i].events & EPOLLIN) {
                int fd = events[i].data.fd;
                handle_client2(fd, &recieve[i], &total[i], &events[i]);
                #if 0
                // 有可读事件，读取数据并处理
                int fd = events[i].data.fd;
                int n = read(fd, buf, BUF_SIZE);
                if (n < 0) {
                    if (errno == ECONNRESET) {
                        close(fd);
                        events[i].data.fd = -1;
                    } else {
                        perror("read error");
                    }
                } else if (n == 0) {
                    close(fd);
                    events[i].data.fd = -1;
                } else {
                    printf("recv data: %s\n", buf);
                }
                #endif
            }
        }

        // 删除已关闭的连接 socket
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == -1) {
                epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
            }
        }
    }

    // 关闭监听 socket 和 epoll 对象
    close(listenfd);
    close(epollfd);

    return 0;
}


#if 0
int main() {
    int server_fd, client_fd, valread;
    struct sockaddr_in address; 

    int addrlen = sizeof(address);
    char buffer[MAX_BUFFER] = {0}; 
	
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
                log_info("socket  failed");
        exit(EXIT_FAILURE);
    }
       
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
       
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
                log_info("bind  failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
                log_info("listen  failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d...\n", PORT);
    
    /* 多线程处理客户端请求 */
    pthread_t tid[1000];
    int i = 0;
    while(1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
                        log_info("accept  failed");
            exit(EXIT_FAILURE);
        }

        if (pthread_create(&tid[i], NULL, handle_client, &client_fd) != 0 ) {
            printf("Failed to create thread\n");
        }

        //if (pthread_join(tid[i], NULL) != 0) {
        //    printf("Failed to join thread\n");
        //}

        i++;
        if (i == 1000)
        {
                log_info("max  LIMIT 3");
                exit(EXIT_FAILURE);
        }
    }
    while (1) {
        
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
                        printf("out\n");
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        printf("New client connected. IP address: %s , Port : %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        
        
        while (1) {
            valread = read( client_fd , buffer, MAX_BUFFER);
            if (valread == 0 || strcmp(buffer, "exit") == 0) {
                printf("Client disconnected. IP address: %s , Port : %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                break;
            }
            printf("Received from client: %s\n", buffer);
                        RevieOK(buffer);

            send(client_fd , buffer , strlen(buffer) , 0 );
            memset(buffer, 0, MAX_BUFFER);
        }
    }
    return 0;
}
#endif