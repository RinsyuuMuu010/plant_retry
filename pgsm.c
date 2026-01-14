//
// Created by Akimei1223 on 2026/1/10.
//
#include "pgsm.h"

const int Seed_prices[]={30,30,20,50}; //价位表
void initial_plant(Plant *plant,PlantType type) {
    switch (type) {   //如何设置合理的需求和价位？
        case Carrot: //胡萝卜
            strcpy(plant->name, "Carrot");
            plant->water_need = 20;
            plant->nutrient_need = 5;
            plant->total_growth_days = 4;
            plant->sell_price = 90;
            break;

        case Potato:  //土豆
            strcpy(plant->name, "Potato");
            plant->water_need = 20;
            plant->nutrient_need = 5;
            plant->total_growth_days = 4;
            plant->sell_price = 90;
            break;
        case Wheat:  //小麦
            strcpy(plant->name, "Wheat");
            plant->water_need = 15;
            plant->nutrient_need = 6;
            plant->total_growth_days = 3;
            plant->sell_price = 60;
            break;

        case Daisy:  //雏菊
            strcpy(plant->name, "Daisy");
            plant->water_need = 25;
            plant->nutrient_need = 6;
            plant->total_growth_days = 7;
            plant->sell_price = 150;
            break;
            default:
            break;
    }
    plant->type = type ;
    plant->growth_stage = 0 ;
    plant->growth_value = 0 ;
    plant->health = 100;
}

// 种植种子
int plant_seed(Player *player, Plot *plots, int plot_idx, PlantType plant_type) {
    // 边界检查
    if (plot_idx < 0 || plot_idx >= player->plot_count) {
        printf("无效的地块！\n");
        return 0;
    }
    Plot *target_plot = &plots[plot_idx];

    // 检查地块是否非空
    if (!target_plot->is_empty) {
        printf("已有植物，无法种植！\n");
        return 0;
    }

    // 检查植物类型是否有效
    if (plant_type == None || plant_type >= sizeof(Seed_prices)/sizeof(int)) {
        printf("不存在的植物\n");
        return 0;
    }
    int seed_price = Seed_prices[plant_type];
    if (player->coins < seed_price) {
        printf("金币不足！需要%d金币，当前只有%d\n", seed_price, player->coins);
        return 0;
    }
    // 执行种植
    player->coins -= seed_price; // 扣减金币
    target_plot->is_empty = 0;   // 标记地块非空
    target_plot->planted_days = 0; //标记已生长时间
    target_plot->soil_water = 20; // 初始土壤水分
    target_plot->soil_nutrient = 10; // 初始土壤养分
    initial_plant(&target_plot->plant, plant_type); // 初始化植物

    printf("成功种植%s！消耗%d金币，剩余金币：%d\n",
           target_plot->plant.name, seed_price, player->coins);
    return 1;
}
