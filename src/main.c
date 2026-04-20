/*******************************************************************
 *
 * main.c - LVGL simulator for GNU/Linux
 *
 * Based on the original file from the repository
 *
 * @note eventually this file won't contain a main function and will
 * become a library supporting all major operating systems
 *
 * To see how each driver is initialized check the
 * 'src/lib/display_backends' directory
 *
 * - Clean up
 * - Support for multiple backends at once
 *   2025 EDGEMTech Ltd.
 *
 * Author: EDGEMTech Ltd, Erik Tagirov (erik.tagirov@edgemtech.ch)
 *
 ******************************************************************/
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lvgl/src/libs/gif/lv_gif.h"      // ← 添加这行：GIF 支持头文件

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"

/* Internal functions */
static void configure_simulator(int argc, char **argv);
static void print_lvgl_version(void);
static void print_usage(void);

/* contains the name of the selected backend if user
 * has specified one on the command line */
static char *selected_backend;

/* Global simulator settings, defined in lv_linux_backend.c */
extern simulator_settings_t settings;


/**
 * @brief Print LVGL version
 */
static void print_lvgl_version(void)
{
    fprintf(stdout, "%d.%d.%d-%s\n",
            LVGL_VERSION_MAJOR,
            LVGL_VERSION_MINOR,
            LVGL_VERSION_PATCH,
            LVGL_VERSION_INFO);
}

/**
 * @brief Print usage information
 */
static void print_usage(void)
{
    fprintf(stdout, "\nlvglsim [-V] [-B] [-b backend_name] [-W window_width] [-H window_height]\n\n");
    fprintf(stdout, "-V print LVGL version\n");
    fprintf(stdout, "-B list supported backends\n");
}

/**
 * @brief Configure simulator
 * @description process arguments recieved by the program to select
 * appropriate options
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
static void configure_simulator(int argc, char **argv)
{
    int opt = 0;

    selected_backend = NULL;
    driver_backends_register();

    const char *env_w = getenv("LV_SIM_WINDOW_WIDTH");
    const char *env_h = getenv("LV_SIM_WINDOW_HEIGHT");
    /* Default values */
    settings.window_width = atoi(env_w ? env_w : "800");
    settings.window_height = atoi(env_h ? env_h : "480");

    /* Parse the command-line options. */
    while ((opt = getopt (argc, argv, "b:fmW:H:BVh")) != -1) {
        switch (opt) {
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
            break;
        case 'V':
            print_lvgl_version();
            exit(EXIT_SUCCESS);
            break;
        case 'B':
            driver_backends_print_supported();
            exit(EXIT_SUCCESS);
            break;
        case 'b':
            if (driver_backends_is_supported(optarg) == 0) {
                die("error no such backend: %s\n", optarg);
            }
            selected_backend = strdup(optarg);
            break;
        case 'W':
            settings.window_width = atoi(optarg);
            break;
        case 'H':
            settings.window_height = atoi(optarg);
            break;
        case ':':
            print_usage();
            die("Option -%c requires an argument.\n", optopt);
            break;
        case '?':
            print_usage();
            die("Unknown option -%c.\n", optopt);
        }
    }
}

/**
 * @brief entry point
 * @description start a demo
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
int main(int argc, char **argv)
{

    configure_simulator(argc, argv);

    /* Initialize LVGL. */
    lv_init();

    /* Initialize the configured backend */
    if (driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }

    /* Enable for EVDEV support */
#if LV_USE_EVDEV
    if (driver_backends_init_backend("EVDEV") == -1) {
        die("Failed to initialize evdev");
    }
#endif

    // LV_FONT_DECLARE(my_language);  //声明字库
    // lv_obj_t * label = lv_label_create(lv_scr_act());
    // lv_label_set_text_fmt(label, "Hello, 湛江科技学院！");
    // lv_obj_set_style_text_font(label, &my_language, 0);

    /* Create a demo */
    //lv_demo_widgets();
    //lv_demo_music();
    //lv_demo_benchmark();
    //lv_demo_keypad_encoder();
    //lv_demo_stress();
    //lv_demo_widgets_start_slideshow();

    ui_init();

// /* 从文件加载 GIF（需要文件系统支持） */
// lv_obj_t * gif_img = lv_gif_create(lv_scr_act());
// lv_gif_set_src(gif_img, "A:/gf1.gif");  // 路径前缀根据文件系统配置调整
// lv_obj_center(gif_img);  // 居中显示，也可以自定义位置

// get_deepseek_result();  //测试时使用

//  LV_IMAGE_DECLARE(gf1);
//  lv_obj_t * gif_img1 = lv_gif_create(lv_scr_act());
//  lv_gif_set_src(gif_img1, &gf1);
//  lv_obj_align(gif_img1, LV_ALIGN_CENTER,0, 0);

// 声明图片资源
// LV_IMAGE_DECLARE(im2);
// lv_obj_t * img2 = lv_gif_create(lv_screen_active());
// lv_gif_set_src(img2, &im2);
// lv_obj_align(img2, LV_ALIGN_CENTER,0, 0);
 
// // 声明图片资源
// LV_IMAGE_DECLARE(im3);
// lv_obj_t * img3 = lv_gif_create(lv_screen_active());
// lv_gif_set_src(img3, &im3);
// lv_obj_align(img3, LV_ALIGN_CENTER,0, 0);


    /*Create a Demo*/
    //lv_demo_widgets();
    //lv_demo_widgets_start_slideshow();

    
    // lv_example_ime_pinyin_1();
    /* Enter the run loop of the selected backend */
    driver_backends_run_loop();

    return 0;
}