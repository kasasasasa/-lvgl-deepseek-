#include "../ui.h"

#include <time.h>

static lv_obj_t * time_label = NULL;
static lv_timer_t * time_timer = NULL;

lv_obj_t * ui_Screen1 = NULL;
static lv_obj_t * gif = NULL;
static lv_timer_t * anim_t = NULL;
static lv_obj_t * progress_bar;
static lv_timer_t * timer;
static int progress = 0;

/* 声明来自 main.c 的函数 */
extern void load_screen2(void);

static void update_time_cb(lv_timer_t * t)
{
    if(time_label) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[16];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
        lv_label_set_text(time_label, time_str);
    }
}

//跳转函数
static void go_next_screen()
{
    // ⭐ 初始化下一个界面（比如Screen2）
    ui_Screen2_screen_init();

    // ⭐ 切换界面
    lv_scr_load(ui_Screen2);
}

//进度条
static void progress_timer_cb(lv_timer_t * t)
{
    progress += 2;   // ⭐ 控制速度（越大越快）

    if(progress > 100) progress = 100;

    lv_bar_set_value(progress_bar, progress, LV_ANIM_OFF);

    if(progress >= 100) {
        lv_timer_del(timer);
        go_next_screen();  // ⭐ 自动跳转
    }
}

//点击跳过
static void screen_click_event(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {

        if(timer) {
            lv_timer_del(timer);
        }

        go_next_screen();
    }
}

// 点击事件（关键）
static void screen1_click_event(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED){

        // 初始化 Screen2
        ui_Screen2_screen_init();

        // 切换屏幕
        lv_scr_load(ui_Screen2);
    }
}

/* 初始化界面 */
void ui_Screen1_screen_init(void)
{
    if (ui_Screen1) {
        lv_obj_del(ui_Screen1);
    }

    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);

    gif = lv_gif_create(ui_Screen1);

    if(gif == NULL){
        printf("gif create failed\n");
        return;
    }

    lv_gif_set_src(gif, "A:/root/bulb.gif");  // 根据你实际路径改
    lv_obj_center(gif);

    /* ===== 进度条 ===== */
    progress_bar = lv_bar_create(ui_Screen1);
    lv_obj_set_size(progress_bar, 800, 20);
    lv_obj_align(progress_bar, LV_ALIGN_BOTTOM_MID, 0, -30);

    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);

    // 设置进度条背景样式
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0xE8E0F0), LV_PART_MAIN);      // 背景色：浅紫色
    lv_obj_set_style_bg_opa(progress_bar, 255, LV_PART_MAIN);

    // 设置进度条指示器样式（已填充部分）
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0xB19AB3), LV_PART_INDICATOR); // 指示器色：紫色
    lv_obj_set_style_bg_opa(progress_bar, 255, LV_PART_INDICATOR);

    // 可选：设置圆角
    lv_obj_set_style_radius(progress_bar, 10, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar, 10, LV_PART_INDICATOR);

    /* ===== 点击跳过 ===== */
    lv_obj_add_event_cb(ui_Screen1, screen_click_event, LV_EVENT_CLICKED, NULL);

    /* ===== 启动定时器（模拟GIF进度） ===== */
    progress = 0;
    timer = lv_timer_create(progress_timer_cb, 50, NULL);

    // ===== 右上角时间显示 =====
    time_label = lv_label_create(ui_Screen1);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xB19AB3), LV_PART_MAIN);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -20, 20);

    // 初始化时间显示
    update_time_cb(NULL);

    // 每秒更新一次时间
    time_timer = lv_timer_create(update_time_cb, 1000, NULL);
}

/* 销毁 */
void ui_Screen1_screen_destroy(void)
{
    if(anim_t){
        lv_timer_del(anim_t);
        anim_t = NULL;
    }

    if (ui_Screen1) {
        lv_obj_del(ui_Screen1);
        ui_Screen1 = NULL;
        gif = NULL;
    }

    if(time_timer) {
    lv_timer_del(time_timer);
    time_timer = NULL;
    }
    time_label = NULL;
}