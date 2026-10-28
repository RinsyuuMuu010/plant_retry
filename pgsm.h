/*
 * pgsm.h —— 像素花园 PIXEL GARDEN
 * 终端花园模拟经营游戏：数据结构、常量与对外接口。
 *
 * 设计约定：
 *   1. 生长值使用千分制（0 ~ 1000），避免整数除法把“7 天成熟”算成 98%；
 *   2. 种子在商店按棵购买，播种时消耗背包库存，不重复扣钱；
 *   3. 存档是带版本号的文本格式，方便手改、校验与向后兼容。
 */

#ifndef PGSM_PGSM_H
#define PGSM_PGSM_H

#include <stddef.h>

/* ------------------------------------------------------------------ */
/* 版本与路径                                                          */
/* ------------------------------------------------------------------ */
#define PGSM_NAME       "像素花园 · PIXEL GARDEN"
#define PGSM_SHORT      "PIXEL GARDEN"
#define PGSM_VERSION    "3.0.0"
#define SAVE_FILE       "garden_save.dat"
#define SAVE_VERSION    2

/* ------------------------------------------------------------------ */
/* 数值常量                                                            */
/* ------------------------------------------------------------------ */
#define START_PLOTS     4       /* 初始地块数 */
#define MAX_PLOTS       8       /* 地块上限 */
#define PLOT_PRICE      150     /* 扩建一块地块的价格 */

#define PM_FULL         1000    /* 生长值满值（千分制，1000 = 100%） */
#define SOIL_CAP        100     /* 土壤水分 / 养分上限 */
#define HEALTH_CAP      100     /* 健康值上限 */

#define WATER_PRICE     1       /* 每单位水的价格 */
#define FERT_PRICE      2       /* 每单位肥料的价格 */
#define CURE_PRICE      12      /* 除虫一次的价格 */

#define WATER_PER_USE   6       /* 一次浇水消耗的水 */
#define FERT_PER_USE    3       /* 一次施肥消耗的肥料 */
#define SOIL_WATER_GAIN 40      /* 一次浇水提升的土壤水分 */
#define SOIL_FERT_GAIN  35      /* 一次施肥提升的土壤养分 */

#define DAYS_PER_SEASON 15      /* 每个季节的天数 */

/* ------------------------------------------------------------------ */
/* 枚举                                                                */
/* ------------------------------------------------------------------ */
typedef enum {
    PLANT_CARROT = 0,
    PLANT_POTATO,
    PLANT_WHEAT,
    PLANT_DAISY,
    PLANT_CACTUS,
    PLANT_ROSE,
    PLANT_COUNT
} PlantType;

typedef enum {
    SEASON_SPRING = 0,
    SEASON_SUMMER,
    SEASON_AUTUMN,
    SEASON_WINTER,
    SEASON_COUNT
} Season;

typedef enum {
    WEATHER_SUNNY = 0,
    WEATHER_CLOUDY,
    WEATHER_RAIN,
    WEATHER_STORM,
    WEATHER_COUNT
} Weather;

/* 成就用位标记，存档里只存一个整数 */
typedef enum {
    ACH_FIRST_HARVEST = 1u << 0,   /* 第一次收获 */
    ACH_HARVEST_10    = 1u << 1,   /* 累计收获 10 次 */
    ACH_RICH_1000     = 1u << 2,   /* 累计收入达到 1000 金币 */
    ACH_FULL_GARDEN   = 1u << 3,   /* 所有地块都种上植物 */
    ACH_COLLECTOR     = 1u << 4    /* 六种植物全部种过 */
} Achievement;

/* ------------------------------------------------------------------ */
/* 结构体                                                              */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *name;            /* 中文名 */
    const char *tag;             /* 英文名，用于紧凑显示 */
    int seed_price;              /* 种子价格 */
    int sell_price;              /* 成熟售价 */
    int growth_days;             /* 理论成熟天数 */
    int water_need;              /* 每日基础耗水 */
    int nutrient_need;           /* 每日基础耗肥 */
    int unlock_harvests;         /* 解锁所需累计收获次数 */
    Season best_season;          /* 当季加成 */
    const char *flavor;          /* 一句话介绍 */
} PlantInfo;

typedef struct {
    PlantType type;
    int growth;                  /* 千分制 0 ~ PM_FULL */
    int health;                  /* 0 ~ HEALTH_CAP */
    int days_planted;            /* 已种植天数 */
    int pest;                    /* 0 = 无虫害，1 = 有虫害 */
} Plant;

typedef struct {
    int empty;                   /* 1 = 空地 */
    Plant plant;
    int soil_water;              /* 土壤水分 0 ~ SOIL_CAP */
    int soil_nutrient;           /* 土壤养分 0 ~ SOIL_CAP */
} Plot;

typedef struct {
    int coins;
    int waters;
    int nutrients;
    int day;
    int plot_count;
    int weather;                 /* Weather，存入存档以便读档后界面一致 */
    int total_harvests;
    int total_earned;
    int grown_mask;              /* 种过的植物位图，用于“收藏家”成就 */
    unsigned achievements;       /* 已解锁成就位图 */
    int seeds[PLANT_COUNT];      /* 背包里的种子数量 */
} Player;

/* ------------------------------------------------------------------ */
/* 植物表与查询                                                        */
/* ------------------------------------------------------------------ */
const PlantInfo *plant_info(PlantType type);
int              plant_unlocked(const Player *player, PlantType type);
const char      *season_name(Season season);
const char      *weather_name(Weather weather);
Season           season_of_day(int day);
int              count_seeds(const Player *player);

/* ------------------------------------------------------------------ */
/* 初始化                                                              */
/* ------------------------------------------------------------------ */
void game_new(Player *player, Plot *plots);
void plot_reset(Plot *plot);
int  count_planted(const Player *player, const Plot *plots);

/* ------------------------------------------------------------------ */
/* 玩家操作：返回 1 表示成功，0 表示失败（失败原因已打印）              */
/* ------------------------------------------------------------------ */
int plant_seed(Player *player, Plot *plots, int plot_idx, PlantType type);
int water_plant(Player *player, Plot *plots, int plot_idx);
int fertilize_plant(Player *player, Plot *plots, int plot_idx);
int cure_pest(Player *player, Plot *plots, int plot_idx);
int harvest_plant(Player *player, Plot *plots, int plot_idx);

/* 推进一天：更新天气、随机事件、水分养分、生长与健康 */
void advance_day(Player *player, Plot *plots);

/* ------------------------------------------------------------------ */
/* 商店                                                                */
/* ------------------------------------------------------------------ */
int  buy_seed(Player *player, PlantType type);
int  buy_water(Player *player, int amount);
int  buy_fertilizer(Player *player, int amount);
int  buy_plot(Player *player, Plot *plots);
void shop(Player *player, Plot *plots);

/* ------------------------------------------------------------------ */
/* 成就                                                                */
/* ------------------------------------------------------------------ */
void check_achievements(Player *player, const Plot *plots);

/* ------------------------------------------------------------------ */
/* 存档：返回 1 表示成功                                               */
/* ------------------------------------------------------------------ */
int save_game(const Player *player, const Plot *plots);
int load_game(Player *player, Plot *plots);

#endif /* PGSM_PGSM_H */
