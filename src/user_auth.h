#ifndef USER_AUTH_H
#define USER_AUTH_H

#include <stdbool.h>

// 用户数据库结构
typedef struct {
    char phone[12];
    char password[32];
} AuthUser;

// 用户管理函数
void load_users_from_file(void);
void save_users_to_file(void);
int is_valid_phone(const char *phone);
int find_user_by_phone(const char *phone);
int add_user(const char *phone, const char *password);
int verify_login(const char *phone, const char *password, char *err_msg, int err_size);
int get_user_count(void);
int update_user_password(int index, const char *new_password);
const AuthUser* get_user_by_index(int index);

#endif // USER_AUTH_H