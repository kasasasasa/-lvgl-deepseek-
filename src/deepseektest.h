#ifndef DEEPSEEKTEST_H
#define DEEPSEEKTEST_H

#ifdef __cplusplus
extern "C" {
#endif

// 获取DeepSeek回答
// 参数：
//   text: 用户输入的问题
//   out_buf: 输出缓冲区
//   buf_len: 缓冲区大小
// 返回值：0成功，-1失败
int get_deepseek_result(const char *text, char *out_buf, int buf_len);

#ifdef __cplusplus
}
#endif

#endif