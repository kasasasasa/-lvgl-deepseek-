// deepseek_wrapper.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DEEPSEEK_PORT 11434
#define DEEPSEEK_IP "192.168.208.97"  // 根据实际情况修改
#define BUFFER_SIZE 4096

static char* extract_json_string(const char *json, const char *key)
{
    char search_key[256];
    char *found, *start, *end;
    static char result[4096];
    
    snprintf(search_key, sizeof(search_key), "\"%s\":\"", key);
    found = strstr(json, search_key);
    if(!found) return NULL;
    
    start = found + strlen(search_key);
    end = strchr(start, '"');
    if(!end) return NULL;
    
    int len = end - start;
    if(len >= (int)sizeof(result)) len = sizeof(result) - 1;
    strncpy(result, start, len);
    result[len] = '\0';
    return result;
}

int get_deepseek_result_with_question(const char *question, char *response, int response_size)
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char request_body[2048];
    char request[4096];
    char buffer[BUFFER_SIZE];
    char allbuf[BUFFER_SIZE * 4] = {0};
    int total_received = 0;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(DEEPSEEK_PORT);
    if (inet_pton(AF_INET, DEEPSEEK_IP, &serv_addr.sin_addr) <= 0) {
        close(sock);
        return -2;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return -3;
    }
    
    // 转义JSON字符串
    char escaped[2048] = {0};
    char *src = (char*)question, *dst = escaped;
    while(*src && (dst - escaped) < 2000) {
        if(*src == '"' || *src == '\\') *dst++ = '\\';
        *dst++ = *src++;
    }
    *dst = '\0';
    
    snprintf(request_body, sizeof(request_body), 
             "{\"model\":\"deepseek-r1:1.5b\",\"prompt\":\"%s\",\"stream\":false}", escaped);
    
    snprintf(request, sizeof(request),
             "POST /api/generate HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             DEEPSEEK_IP, DEEPSEEK_PORT, strlen(request_body), request_body);
    
    if (send(sock, request, strlen(request), 0) < 0) {
        close(sock);
        return -4;
    }
    
    int valread;
    while ((valread = read(sock, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[valread] = '\0';
        if (total_received + valread < (int)sizeof(allbuf) - 1) {
            strcat(allbuf + total_received, buffer);
            total_received += valread;
        }
        if (strstr(allbuf, "}\r\n") || strstr(allbuf, "}\n")) break;
    }
    close(sock);
    
    char *body = strstr(allbuf, "\r\n\r\n");
    if (!body) body = allbuf;
    else body += 4;
    
    char *resp = extract_json_string(body, "response");
    if (resp && strlen(resp) > 0) {
        strncpy(response, resp, response_size - 1);
        response[response_size - 1] = '\0';
        return 0;
    }
    
    return -5;
}