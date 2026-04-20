#include "../ui.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

// ===== 外部声明 =====
extern lv_obj_t * ui_Screen2;
extern void ui_Screen2_screen_init(void);

// ===== 背景图片声明 =====
LV_IMG_DECLARE(ui_img_klm_bmp);  // 如果图片格式是 png

// ===== 时间显示相关 =====
static lv_obj_t * time_label = NULL;
static lv_timer_t * time_timer = NULL;

// ===== UI对象 =====
lv_obj_t * ui_Screen7 = NULL;
static lv_obj_t * grid_left;
static lv_obj_t * grid_right;
static lv_obj_t * label_timer;
static lv_obj_t * label_score_left;
static lv_obj_t * label_score_right;

// ===== 游戏数据 =====
static int score_left = 0;
static int score_right = 0;
static int time_left = 30;

static lv_timer_t * game_timer = NULL;
static lv_timer_t * ready_timer = NULL;

static lv_obj_t * ready_popup = NULL;
static lv_obj_t * ready_label = NULL;

static int ready_count = 3;
static lv_obj_t * result_popup = NULL;
static int game_running = 0;

static lv_obj_t * current_mole_left = NULL;
static lv_obj_t * current_mole_right = NULL;

// ===== 按钮数据 =====
typedef struct {
    int is_left;
} BtnUserData;

// ========== 函数前置声明 ==========
static void show_result_popup(void);
static void show_ready_popup(void);
static void reset_game(void);

static void update_time_cb(lv_timer_t * t)
{
    if(time_label) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[16];
        strftime(time_str, sizeof(time_str), "%H:%M", tm_info);
        lv_label_set_text(time_label, time_str);
    }
}

// ===== 点击事件 =====
static void game_click_event(lv_event_t * e)
{
    if(!game_running) return;
    
    lv_obj_t * btn = lv_event_get_target(e);
    BtnUserData * data = lv_event_get_user_data(e);

    if(btn == current_mole_left || btn == current_mole_right) {
        if(data->is_left)
            score_left++;
        else
            score_right++;

        char buf[32];
        sprintf(buf, "A:%d", score_left);
        lv_label_set_text(label_score_left, buf);

        sprintf(buf, "B:%d", score_right);
        lv_label_set_text(label_score_right, buf);

        lv_obj_set_style_bg_color(btn, lv_color_hex(0xEFE9F2), LV_PART_MAIN);

        if(btn == current_mole_left) current_mole_left = NULL;
        if(btn == current_mole_right) current_mole_right = NULL;
    }
}

static void btn_delete_event(lv_event_t * e)
{
    BtnUserData * data = lv_event_get_user_data(e);
    if(data) lv_free(data);
}

// ===== 创建九宫格 =====
static void create_grid_buttons(lv_obj_t * parent, int is_left)
{
    for(int i = 0; i < 9; i++) {
        lv_obj_t * btn = lv_btn_create(parent);
        lv_obj_set_grid_cell(btn,
            LV_GRID_ALIGN_STRETCH, i % 3, 1,
            LV_GRID_ALIGN_STRETCH, i / 3, 1);

        lv_obj_set_style_bg_color(btn, lv_color_hex(0xEFE9F2), LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xD1B0D6), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_hex(0xB19AB3), LV_PART_MAIN);

        BtnUserData * data = lv_malloc(sizeof(BtnUserData));
        data->is_left = is_left;

        lv_obj_add_event_cb(btn, game_click_event, LV_EVENT_CLICKED, data);
        lv_obj_add_event_cb(btn, btn_delete_event, LV_EVENT_DELETE, data);
    }
}

// ===== 随机生成地鼠 =====
static void spawn_mole(lv_obj_t * grid, lv_obj_t ** current_mole)
{
    if(*current_mole && lv_obj_is_valid(*current_mole)) {
        lv_obj_set_style_bg_color(*current_mole, lv_color_hex(0xEFE9F2), LV_PART_MAIN);
    }

    int index = rand() % 9;
    lv_obj_t * btn = lv_obj_get_child(grid, index);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xB19AB3), LV_PART_MAIN);
    *current_mole = btn;
}

// ===== 游戏循环 =====
static void game_timer_cb(lv_timer_t * t)
{
    if(!game_running) return;
    
    time_left--;

    char buf[32];
    sprintf(buf, "time:%d", time_left);
    lv_label_set_text(label_timer, buf);

    spawn_mole(grid_left, &current_mole_left);
    spawn_mole(grid_right, &current_mole_right);

    if(time_left <= 0) {
        game_running = 0;
        if(game_timer) {
            lv_timer_del(game_timer);
            game_timer = NULL;
        }
        show_result_popup();
    }
}

// ===== 准备计时 =====
static void ready_timer_cb(lv_timer_t * t)
{
    ready_count--;

    char buf[32];
    sprintf(buf, "ready\n%d", ready_count);

    if(ready_label) {
        lv_label_set_text(ready_label, buf);
    }

    if(ready_count <= 0) {
        if(ready_timer) {
            lv_timer_del(ready_timer);
            ready_timer = NULL;
        }

        if(ready_popup) {
            lv_obj_del(ready_popup);
            ready_popup = NULL;
            ready_label = NULL;
        }

        game_running = 1;
        game_timer = lv_timer_create(game_timer_cb, 1000, NULL);
    }
}

// ===== 准备弹窗 =====
static void show_ready_popup(void)
{
    if(ready_timer) {
        lv_timer_del(ready_timer);
        ready_timer = NULL;
    }
    
    ready_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(ready_popup, 300, 200);
    lv_obj_center(ready_popup);
    lv_obj_set_style_bg_color(ready_popup, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_radius(ready_popup, 20, LV_PART_MAIN);
    lv_obj_set_style_border_color(ready_popup, lv_color_hex(0xB19AB3), LV_PART_MAIN);
    lv_obj_set_style_border_width(ready_popup, 2, LV_PART_MAIN);

    ready_label = lv_label_create(ready_popup);
    lv_label_set_text(ready_label, "ready\n3");
    lv_obj_center(ready_label);
    lv_obj_set_style_text_color(ready_label, lv_color_hex(0xB19AB3), LV_PART_MAIN);
    lv_obj_set_style_text_font(ready_label, &lv_font_montserrat_32, LV_PART_MAIN);

    ready_count = 3;
    ready_timer = lv_timer_create(ready_timer_cb, 1000, NULL);
}

// ===== 重置游戏 =====
static void reset_game(void)
{
    // 停止所有计时器
    if(game_timer) {
        lv_timer_del(game_timer);
        game_timer = NULL;
    }
    if(ready_timer) {
        lv_timer_del(ready_timer);
        ready_timer = NULL;
    }
    
    // 关闭所有弹窗
    if(ready_popup && lv_obj_is_valid(ready_popup)) {
        lv_obj_del(ready_popup);
        ready_popup = NULL;
        ready_label = NULL;
    }
    if(result_popup && lv_obj_is_valid(result_popup)) {
        lv_obj_del(result_popup);
        result_popup = NULL;
    }
    
    // 恢复地鼠颜色
    if(current_mole_left && lv_obj_is_valid(current_mole_left)) {
        lv_obj_set_style_bg_color(current_mole_left, lv_color_hex(0xEFE9F2), LV_PART_MAIN);
        current_mole_left = NULL;
    }
    if(current_mole_right && lv_obj_is_valid(current_mole_right)) {
        lv_obj_set_style_bg_color(current_mole_right, lv_color_hex(0xEFE9F2), LV_PART_MAIN);
        current_mole_right = NULL;
    }
    
    // 重置分数和时间
    score_left = 0;
    score_right = 0;
    time_left = 30;
    game_running = 0;
    
    // 更新显示
    lv_label_set_text(label_score_left, "A:0");
    lv_label_set_text(label_score_right, "B:0");
    lv_label_set_text(label_timer, "time:30");
    
    // 显示准备弹窗
    show_ready_popup();
}

// ===== 再来一局 =====
static void restart_event(lv_event_t * e)
{
    LV_UNUSED(e);
    
    // 关闭结果弹窗
    if(result_popup && lv_obj_is_valid(result_popup)) {
        lv_obj_del(result_popup);
        result_popup = NULL;
    }
    
    reset_game();
}

// ===== 返回按钮事件（与 Screen4 的 back_event 一致）=====
static void back_btn_event(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // 关闭可能存在的弹窗
        if(ready_popup && lv_obj_is_valid(ready_popup)) {
            lv_obj_del(ready_popup);
            ready_popup = NULL;
            ready_label = NULL;
        }
        if(result_popup && lv_obj_is_valid(result_popup)) {
            lv_obj_del(result_popup);
            result_popup = NULL;
        }
        
        // 停止所有计时器
        if(game_timer) {
            lv_timer_del(game_timer);
            game_timer = NULL;
        }
        if(ready_timer) {
            lv_timer_del(ready_timer);
            ready_timer = NULL;
        }
        if(time_timer) {
            lv_timer_del(time_timer);
            time_timer = NULL;
        }
        
        game_running = 0;
        
        // 返回登录界面
        _ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, NULL);
    }
}

// ===== 返回按钮事件（弹窗内） =====
static void popup_back_event(lv_event_t * e)
{
    LV_UNUSED(e);
    
    // 关闭弹窗
    if(ready_popup && lv_obj_is_valid(ready_popup)) {
        lv_obj_del(ready_popup);
        ready_popup = NULL;
        ready_label = NULL;
    }
    if(result_popup && lv_obj_is_valid(result_popup)) {
        lv_obj_del(result_popup);
        result_popup = NULL;
    }
    
    // 停止所有计时器
    if(game_timer) {
        lv_timer_del(game_timer);
        game_timer = NULL;
    }
    if(ready_timer) {
        lv_timer_del(ready_timer);
        ready_timer = NULL;
    }
    if(time_timer) {
        lv_timer_del(time_timer);
        time_timer = NULL;
    }
    
    game_running = 0;
    
    // 返回登录界面
    _ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, NULL);
}

// ===== 游戏结束弹窗 =====
static void show_result_popup(void)
{
    char result[128];

    if(score_left > score_right)
        sprintf(result, "A Win!\n%d : %d", score_left, score_right);
    else if(score_right > score_left)
        sprintf(result, "B Win!\n%d : %d", score_left, score_right);
    else
        sprintf(result, "Tie!\n%d : %d", score_left, score_right);

    lv_obj_t * box = lv_obj_create(lv_layer_top());
    result_popup = box;

    lv_obj_set_size(box, 400, 250);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_radius(box, 20, LV_PART_MAIN);
    lv_obj_set_style_border_color(box, lv_color_hex(0xB19AB3), LV_PART_MAIN);
    lv_obj_set_style_border_width(box, 2, LV_PART_MAIN);

    lv_obj_t * txt = lv_label_create(box);
    lv_label_set_text(txt, result);
    lv_obj_align(txt, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(txt, lv_color_hex(0xB19AB3), LV_PART_MAIN);
    lv_obj_set_style_text_font(txt, &lv_font_montserrat_28, LV_PART_MAIN);

    // restart 按钮
    lv_obj_t * btn1 = lv_btn_create(box);
    lv_obj_set_size(btn1, 120, 40);
    lv_obj_align(btn1, LV_ALIGN_BOTTOM_LEFT, 30, -20);
    lv_obj_add_event_cb(btn1, restart_event, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn1, lv_color_hex(0x8E6AA8), LV_PART_MAIN);
    lv_obj_set_style_radius(btn1, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn1, lv_color_hex(0xA07DB8), LV_STATE_PRESSED);

    lv_obj_t * l1 = lv_label_create(btn1);
    lv_label_set_text(l1, "restart");
    lv_obj_center(l1);
    lv_obj_set_style_text_color(l1, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    // back 按钮（返回 Screen2）
    lv_obj_t * btn2 = lv_btn_create(box);
    lv_obj_set_size(btn2, 120, 40);
    lv_obj_align(btn2, LV_ALIGN_BOTTOM_RIGHT, -30, -20);
    lv_obj_add_event_cb(btn2, popup_back_event, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn2, lv_color_hex(0xB19AB3), LV_PART_MAIN);
    lv_obj_set_style_radius(btn2, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn2, lv_color_hex(0xC8A8CB), LV_STATE_PRESSED);

    lv_obj_t * l2 = lv_label_create(btn2);
    lv_label_set_text(l2, "back");
    lv_obj_center(l2);
    lv_obj_set_style_text_color(l2, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
}

// ===== 初始化 =====
void ui_Screen7_screen_init(void)
{
    srand(time(NULL));

    ui_Screen7 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen7, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen7, lv_color_hex(0xFAF8EF), LV_PART_MAIN);

    // 重置游戏状态
    game_running = 0;
    score_left = 0;
    score_right = 0;
    time_left = 30;
    current_mole_left = NULL;
    current_mole_right = NULL;

    static int32_t col[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t row[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    // 左侧九宫格
    grid_left = lv_obj_create(ui_Screen7);
    lv_obj_set_size(grid_left, 320, 320);
    lv_obj_align(grid_left, LV_ALIGN_LEFT_MID, 40, 20);
    lv_obj_set_layout(grid_left, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid_left, col, row);
    lv_obj_remove_flag(grid_left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(grid_left, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(grid_left, 0, LV_PART_MAIN);

    // 右侧九宫格
    grid_right = lv_obj_create(ui_Screen7);
    lv_obj_set_size(grid_right, 320, 320);
    lv_obj_align(grid_right, LV_ALIGN_RIGHT_MID, -40, 20);
    lv_obj_set_layout(grid_right, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid_right, col, row);
    lv_obj_remove_flag(grid_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(grid_right, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(grid_right, 0, LV_PART_MAIN);

    create_grid_buttons(grid_left, 1);
    create_grid_buttons(grid_right, 0);

    // 分数显示
    label_score_left = lv_label_create(ui_Screen7);
    lv_obj_align(label_score_left, LV_ALIGN_TOP_LEFT, 40, 10);
    lv_label_set_text(label_score_left, "A:0");
    lv_obj_set_style_text_font(label_score_left, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_score_left, lv_color_hex(0xB19AB3), LV_PART_MAIN);

    label_score_right = lv_label_create(ui_Screen7);
    lv_obj_align(label_score_right, LV_ALIGN_TOP_RIGHT, -40, 10);
    lv_label_set_text(label_score_right, "B:0");
    lv_obj_set_style_text_font(label_score_right, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_score_right, lv_color_hex(0xB19AB3), LV_PART_MAIN);

    label_timer = lv_label_create(ui_Screen7);
    lv_obj_align(label_timer, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_text(label_timer, "time:30");
    lv_obj_set_style_text_font(label_timer, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_timer, lv_color_hex(0xB19AB3), LV_PART_MAIN);

    // 返回按钮（左下角）
    lv_obj_t * back_btn = lv_btn_create(ui_Screen7);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xB19AB3), LV_PART_MAIN);
    lv_obj_set_style_radius(back_btn, 20, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, back_btn_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "返回");
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(back_label, &ui_font_myy2, LV_PART_MAIN);
    lv_obj_center(back_label);

    // 右下角时间显示
    time_label = lv_label_create(ui_Screen7);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xB19AB3), LV_PART_MAIN);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(time_label, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    update_time_cb(NULL);
    time_timer = lv_timer_create(update_time_cb, 1000, NULL);

    // 显示准备弹窗
    show_ready_popup();
    
    printf("[DEBUG] Screen7 initialized\n");
}

// ===== 屏幕销毁 =====
void ui_Screen7_screen_destroy(void)
{
    // 停止所有计时器
    if(game_timer) {
        lv_timer_del(game_timer);
        game_timer = NULL;
    }
    if(ready_timer) {
        lv_timer_del(ready_timer);
        ready_timer = NULL;
    }
    if(time_timer) {
        lv_timer_del(time_timer);
        time_timer = NULL;
    }
    
    // 关闭弹窗
    if(ready_popup && lv_obj_is_valid(ready_popup)) {
        lv_obj_del(ready_popup);
        ready_popup = NULL;
        ready_label = NULL;
    }
    if(result_popup && lv_obj_is_valid(result_popup)) {
        lv_obj_del(result_popup);
        result_popup = NULL;
    }
    
    // 删除屏幕对象
    if(ui_Screen7) {
        lv_obj_del(ui_Screen7);
        ui_Screen7 = NULL;
    }
    
    // 清空所有指针
    grid_left = NULL;
    grid_right = NULL;
    label_timer = NULL;
    label_score_left = NULL;
    label_score_right = NULL;
    time_label = NULL;
    current_mole_left = NULL;
    current_mole_right = NULL;
    game_running = 0;
    
    printf("[DEBUG] Screen7 destroyed\n");
}