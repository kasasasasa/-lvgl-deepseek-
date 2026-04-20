#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include "cjson/cJSON.h"
#include <stdbool.h>

#define PORT 11434                  // 开放的端口
#define SERVER_IP "192.168.208.97"     // 电脑端的 IP 地址

#define BUFFER_SIZE 4096
#define MAX_READ_COUNT 50 // 防止无限循环

int sock = 0;

//1. 初始化 socket 连接
static int init_socket()
{
    struct sockaddr_in serv_addr;
    //1.1 创建 socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    //1.2 将 IP 地址从字符串转换为二进制格式
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -2;
    }
    //1.3 连接到服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -3;
    }
    
    return sock;
}

// 3. unicode转utf8
char* unicode_to_utf8(const char* unicode_str) 
{
    unsigned int code;
    //3.1 提取 4 位十六进制 Unicode 码点
    sscanf(unicode_str, "%4x", &code);

    //3.2 转换为 UTF-8 编码
    char* utf8 = NULL;
    if (code <= 0x7F) {
        utf8 = malloc(2);
        utf8[0] = code;
        utf8[1] = '\0';
    } else if (code <= 0x7FF) {
        utf8 = malloc(3);
        utf8[0] = 0xC0 | (code >> 6);
        utf8[1] = 0x80 | (code & 0x3F);
        utf8[2] = '\0';
    } else if (code <= 0xFFFF) {
        utf8 = malloc(4);
        utf8[0] = 0xE0 | (code >> 12);
        utf8[1] = 0x80 | ((code >> 6) & 0x3F);
        utf8[2] = 0x80 | (code & 0x3F);
        utf8[3] = '\0';
    }
    return utf8;
}

//4. 解码unicode转义字符
char* decode_unicode_escapes(const char* input) 
{
    char* output = malloc(strlen(input) * 4 + 1); // 预留足够空间
    char* out_ptr = output;
    const char* in_ptr = input;

    while (*in_ptr) {
        if (strncmp(in_ptr, "\\u", 2) == 0) {
            // 处理 \uXXXX
            char* utf8 = unicode_to_utf8(in_ptr + 2);
            strcpy(out_ptr, utf8);
            out_ptr += strlen(utf8);
            in_ptr += 6; // 跳过 \uXXXX
            free(utf8);
        } else {
            *out_ptr++ = *in_ptr++;
        }
    }
    *out_ptr = '\0';
    return output;
}


//5. 解析JSON语句(非流式)
int parse_json(const char *json, char *text)
{
    // 将解析后的文本存储在 text 中
    //5.1 查找 HTTP 响应正文
    char *body = strstr(json, "\r\n\r\n");
    if (!body) return 1;
    
    //5.2 跳过 HTTP 头部
    while(*body != '{')
        if(*body++ == '\0') 
            return 1;
    
    //printf("HTTP响应正文: %s\n", body);

    //5.3 解析JSON
    cJSON *root = cJSON_Parse(body);
    if (!root) {
        //printf("JSON解析错误: %s\n", cJSON_GetErrorPtr());
        cJSON_Delete(root);
        return 2;
    }
    
    //5.4 提取字段
    cJSON *response = cJSON_GetObjectItem(root, "response");
    if(response == NULL) 
    {
        //printf("JSON字段不存在\n");
        return 3;
    }

    strcpy(text, response->valuestring); 
    //5.6 清理资源
    cJSON_Delete(root);

    return 0;
}

// 6. 发送请求
int send_request(const char *text)
{
    //6.1 构造 HTTP 请求体（关闭流式响应）
    char request_body[1024] = {0};
    snprintf(request_body, 1024, "{\"model\": \"deepseek-r1:1.5b\", \"prompt\": \"%s\", \"stream\":  \
    false,\"include_context\": false}", text);
    //6.2 编辑请求头
    char request_headers[256] = {0};
    snprintf(request_headers, sizeof(request_headers),
    "POST /api/generate HTTP/1.1\r\n"
    "Host: %s:11434\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %zu\r\n"
    "\r\n",
    SERVER_IP,
    strlen(request_body));
    //6.3 编辑请求协议
    char request[4096] = {0};
    snprintf(request, sizeof(request), "%s%s", request_headers, request_body);
    //6.4 发送请求
    int ret = send(sock, request, strlen(request), 0);
    if (ret < 0)
    {
        printf("Failed to send request.\n");
        return -6;
    }
    //printf("Request sent:\n%s\n", request);

    return 0;
}

//7. 判断括号是否匹配（判断数据是否完整，括号不匹配说明数据没接收完整）
bool isValid(const char* s) 
{
    char stack[BUFFER_SIZE];  // 简易栈
    int top = -1;      // 栈顶指针
    
    for (int i = 0; s[i]; i++) 
    {
        if (s[i] == '(' || s[i] == '[' || s[i] == '{') 
        {
            stack[++top] = s[i];  // 左括号入栈
        } 
        else if (s[i] == ')' || s[i] == ']' || s[i] == '}') 
        {
            if (top == -1) 
                return false;  // 栈空说明不匹配
            char left = stack[top--];     // 弹出栈顶元素
            if (!(left == '(' && s[i] == ')' ||  // 直接判断匹配
                  left == '[' && s[i] == ']' || 
                  left == '{' && s[i] == '}')) 
            {
                return false;
            }
        }
    }
    return top == -1;  // 栈必须为空
}

// 8. 删除字符串中的子串
void removeSubstr(char *str, const char *sub) 
{
    size_t sub_len = strlen(sub);
    if (sub_len == 0) return; // 防止空子串

    char *pos;
    while ((pos = strstr(str, sub)) != NULL) 
    {
        // 计算需要移动的剩余长度（包括结尾的 \0）
        size_t tail_len = strlen(pos + sub_len) + 1;
        memmove(pos, pos + sub_len, tail_len);
    }
}

//获取deepseek的结果
//返回值：0成功，-1失败
//获取deepseek的结果
//返回值：0成功，-1失败
int get_deepseek_result(const char *text, char *out_buf, int buf_len)
{
    int err_val = 0;
    
    // 1. 初始化socket
    err_val = init_socket();
    if (err_val == -1) 
    {
        printf("error value:%d\n", err_val);
        return -1;
    }

    //char text[1024] = {0};
    char allbuf[BUFFER_SIZE * 4] = {0}; // 扩大缓冲区
    char buffer[BUFFER_SIZE] = {0};
    int total_received = 0;
    int read_count = 0;


    printf("请输入问题: %s\n",text);
    // if (fgets(text, sizeof(text), stdin) == NULL) 
    // {
    //     perror("fgets failed");
    //     continue;
    // }
    
    // // 移除换行符
    //text[strcspn(text, "\n")] = 0; 

    // 3. 发送请求
    err_val = send_request(text);
    if (err_val == -1) 
    {
        printf("发送失败, 错误码:%d\n", err_val);
        return -1;
    }

    total_received = 0;
    read_count = 0;
    memset(allbuf, 0, sizeof(allbuf));
    memset(buffer, 0, sizeof(buffer));

    // 4. 循环接收响应（核心修改）
    int valread;
    while ((valread = read(sock, buffer, BUFFER_SIZE - 1)) > 0) 
    {
        // 添加字符串终止符
        buffer[valread] = '\0';
        
        // 检查缓冲区空间
        if (total_received + valread >= (int)sizeof(allbuf) - 1) 
        {
            printf("警告：缓冲区溢出风险，停止接收\n");
            break;
        }
        
        // 拼接数据
        strcat(allbuf + total_received, buffer);
        total_received += valread;
        
        // 检查数据完整性（JSON格式或特定结束标志）
        if (isValid(allbuf) || ++read_count >= MAX_READ_COUNT) 
        {
            break;
        }
        
        memset(buffer, 0, BUFFER_SIZE);
    }

    // 处理接收错误
    if (valread < 0) 
    {
        perror("读取错误");
        printf("服务器繁忙，请稍候再试\n");
        return -2;
    }
    
    //测试用，正常使用时清注释掉
    //printf("%s\n",allbuf);
    //printf("========================================\n");

    // 5. 解析JSON
    char output[BUFFER_SIZE * 2] = {0};
    err_val = parse_json(allbuf, output);
    if (err_val == 0) 
    {
        // 6. 清理特殊标签
        removeSubstr(output, "</think>");
        removeSubstr(output, "<think>");
        removeSubstr(output, "\n\n");
        
        printf("Response:\n%s\n", output);

        /* 把结果拷给用户缓冲区 */
        strncpy(out_buf, output, buf_len - 1);
        out_buf[buf_len - 1] = '\0';
    } 
    else 
    {
        printf("解析失败，错误码:%d\n", err_val);
        printf("原始响应：\n%.200s...\n", allbuf);
    }

    close(sock);
    return 0;
}
// int main()
// {
    
//     get_deepseek_result();


    
//     return 0;

// }