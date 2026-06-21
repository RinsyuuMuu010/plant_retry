//
// 像素花园模拟器 - Pixel Garden Simulator
// 头文件
//
#ifndef PGSM_PLANT_GARDEN_SIMULATION_MANAGEMENT__PGSM_H
#define PGSM_PLANT_GARDEN_SIMULATION_MANAGEMENT__PGSM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

// 经济数据（跨文件引用）
extern const int g_price[];
extern const char *g_cname[];
#define WP  2   // 商店每单位水价格（金币）
#define NP  5   // 商店每单位肥料价格（金币）
#define WU  5   // 每次浇水消耗水资源
#define NU  2   // 每次施肥消耗肥料
#define WA  40  // 每次浇水增加土壤水分
#define NA  40  // 每次施肥增加土壤养分
#define MPC 4   // 地块数量上限
#define SL  "garden_save.dat"
#define SAVE_VERSION 1  // 存档版本号
#define LOG_MAX    5  // 操作日志条数

// 天气类型
typedef enum { WEATHER_SUNNY, WEATHER_CLOUDY, WEATHER_RAINY } Weather;

// 植物类型
typedef enum { Carrot, Potato, Wheat, Daisy, None } PlantType;

// 植物数据
typedef struct {
    char name[12];
    PlantType type;
    int growth_stage;
    int growth_value;       // 0-100
    int health;             // 0-100
    int water_need;
    int nutrient_need;
    int total_growth_days;
    int sell_price;
} Plant;

// 地块
typedef struct {
    int is_empty;
    Plant plant;
    int planted_days;
    int soil_water;
    int soil_nutrient;
} Plot;

// 玩家
typedef struct {
    int coins;
    int waters;
    int nutrients;
    int day;
    int plot_count;
    Weather weather;
    int seed_inventory[4];
} Player;

// 初始化
void init_plant(Plant *p, PlantType t);
void init_player(Player *p);

// 核心操作
int plant_seed (Player *pl, Plot *ps, int idx, PlantType t);
int water_plant(Player *pl, Plot *ps, int idx);
int fer_plant  (Player *pl, Plot *ps, int idx);
int harvest    (Player *pl, Plot *ps, int idx);

// 时间推进
void pushtime(Player *pl, Plot *ps);

// 商店
void shop(Player *pl);

// 存档
int  save_game(Player *pl, Plot *ps);
int  load_game(Player *pl, Plot *ps);

// UI
void show_main_ui(Player *pl, Plot *ps);
int  get_plot(Player *pl);
void flush_stdin(void);
void get_time_str(char *buf, int sz);
void clrscr(void);
void log_add(const char *fmt, ...);
void log_clear(void);
const char* log_get(int i);

// 辅助
const char *weather_name(Weather w);
const char *weather_icon(Weather w);
const char *stage_name(int s);
const char *stage_icon(int s);
const char *plant_cn_name(PlantType t);
void show_easter_egg(void);
void show_exit_summary(int start_coins, int end_coins, int days);

#ifdef _WIN32
void set_color(int fg, int bg);
void reset_color(void);
#endif

#endif
