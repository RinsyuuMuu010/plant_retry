//
// Created by Akimei1223 on 2026/1/10.
//

#ifndef PGSM_PLANT_GARDEN_SIMULATION_MANAGEMENT__PGSM_H
#define PGSM_PLANT_GARDEN_SIMULATION_MANAGEMENT__PGSM_H
//include

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//植物类型
typedef enum {
    Carrot,
    Potato,
    Wheat,
    Daisy,
    None,

}PlantType;

//植物Construct
typedef struct {
    char name[20];          // 植物名称
    PlantType type;         // 植物类型
    int growth_stage;       // 生长阶段（0-5，5为成熟）
    int growth_value;       // 生长值（0-100，满则成熟）
    int health;             // 健康值（0-100，0则枯萎）
    int water_need;         // 每日需水量
    int nutrient_need;      // 每日需养分量
    int total_growth_days;  // 总生长天数
    int sell_price;         // 出售价格
} Plant;

// 地块内容
typedef struct {
    int is_empty;           // 是否空置（1=空，0=有植物）
    Plant plant;            // 种植的植物
    int planted_days;       // 已种植天数
    int soil_water;         // 土壤水分
    int soil_nutrient;      // 土壤养分
} Plot;

// 玩家资产
typedef struct {
    int coins;              // 金币
    int waters ;        // 水资源库存
    int nutrients ;     // 养分库存（肥料）
    int day;                // 游戏天数
    int plot_count;         // 地块数量
} Player;
//初始化
void initial_plant(Plant *plant,PlantType type);
//种植
int plant_seed(Player *player, Plot *plots, int plot_idx, PlantType plant_type);
//浇水
int water_plant(Player *player, Plot *plots, int plot_idx);
//施肥
int fer_plant(Player *player, Plot *plots, int plot_idx);
//收获
int harvest(Player *player,Plot *plots, int plot_idx);

#endif //PGSM_PLANT_GARDEN_SIMULATION_MANAGEMENT__PGSM_H