#include "user_auth.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_USERS 100

static AuthUser users[MAX_USERS] = {0};
static int user_count = 0;

void load_users_from_file(void)
{
    FILE *fp = fopen("/tmp/user_data.txt", "r");
    if(fp) {
        int count;
        if(fscanf(fp, "%d\n", &count) == 1) {
            user_count = count;
            for(int i = 0; i < user_count && i < MAX_USERS; i++) {
                fscanf(fp, "%s %s", users[i].phone, users[i].password);
            }
            printf("[DEBUG] Loaded %d users from file\n", user_count);
        }
        fclose(fp);
    } else {
        printf("[DEBUG] No user file found\n");
    }
}

void save_users_to_file(void)
{
    FILE *fp = fopen("/tmp/user_data.txt", "w");
    if(fp) {
        fprintf(fp, "%d\n", user_count);
        for(int i = 0; i < user_count; i++) {
            fprintf(fp, "%s %s\n", users[i].phone, users[i].password);
        }
        fclose(fp);
        printf("[DEBUG] Saved %d users to file\n", user_count);
    } else {
        printf("[ERROR] Failed to save users\n");
    }
}

int is_valid_phone(const char *phone)
{
    if(!phone) return 0;
    int len = strlen(phone);
    if(len != 11) return 0;
    for(int i = 0; i < len; i++) {
        if(phone[i] < '0' || phone[i] > '9') return 0;
    }
    return 1;
}

int find_user_by_phone(const char *phone)
{
    for(int i = 0; i < user_count; i++) {
        if(strcmp(users[i].phone, phone) == 0) return i;
    }
    return -1;
}

int add_user(const char *phone, const char *password)
{
    if(user_count >= MAX_USERS) return -1;
    if(!is_valid_phone(phone)) return -2;
    if(find_user_by_phone(phone) >= 0) return -3;
    
    strcpy(users[user_count].phone, phone);
    strcpy(users[user_count].password, password);
    user_count++;
    save_users_to_file();
    return 0;
}

int verify_login(const char *phone, const char *password, char *err_msg, int err_size)
{
    if(!is_valid_phone(phone)) {
        snprintf(err_msg, err_size, "请输入11位电话号码");
        return 0;
    }
    int idx = find_user_by_phone(phone);
    if(idx < 0) {
        snprintf(err_msg, err_size, "账号不存在，请先注册");
        return 0;
    }
    if(strcmp(users[idx].password, password) != 0) {
        snprintf(err_msg, err_size, "密码错误");
        return 0;
    }
    return 1;
}

int get_user_count(void)
{
    return user_count;
}

int update_user_password(int index, const char *new_password)
{
    if(index < 0 || index >= user_count) return -1;
    if(!new_password || strlen(new_password) < 6) return -2;
    
    strcpy(users[index].password, new_password);
    save_users_to_file();
    return 0;
}

const AuthUser* get_user_by_index(int index)
{
    if(index < 0 || index >= user_count) return NULL;
    return &users[index];
}