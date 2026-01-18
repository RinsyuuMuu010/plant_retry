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
//摸了一天鱼
//不要再摸辣！
//浇水
int water_plant(Player *player,Plot *plots,int plot_idx) {
    if (plot_idx<0||plot_idx >= player->plot_count) {
        printf("无效地块！\n");
        return  0;
    }
    Plot *target_plot = &plots[plot_idx];

    //检查地块
    if (target_plot->is_empty) {
        printf("没有植物，无需浇水~\n");
        return  0;
    }

    //检查水资源
    const int Water_use = 5;
    if (player->waters<Water_use) {
        printf("水资源不足，需要%d，当前只有%d\n",Water_use,player->waters);
        return  0;
    }

    //浇水conduct
    player->waters -= Water_use;
    target_plot->soil_water += 20; //增加土壤水分
    if (target_plot->soil_water > 100)
        target_plot->soil_water =100;
    printf("成功给%s浇水，剩余水资源：%d,当前土壤水分：%d\n",target_plot->plant.name,player->waters,target_plot->soil_water);
    return 1;
}
//施肥
int fer_plant(Player *player,Plot *plots,int plot_idx) {
        if (plot_idx<0||plot_idx >= player->plot_count) {
            printf("无效地块！\n");
            return  0;
        }
        Plot *target_plot = &plots[plot_idx];
        if (target_plot->is_empty) {
            printf("该地块没有种植植物，无需施肥！\n");
            return  0;
        }
        //施肥消耗
        const int Nutrient_use = 2;
        if (player->nutrients<Nutrient_use) {
            printf("肥料不足，需要%d，当前只有%d\n",Nutrient_use,player->nutrients);
            return  0;
        }

        //检查后执行
        player->nutrients -= Nutrient_use;
        target_plot->soil_nutrient += 10; //增加土壤养分
        if (target_plot->soil_nutrient>100)
            target_plot->soil_nutrient = 100;
        printf("成功给%s施肥，剩余肥料：%d，当前土壤养分：%d\n",
               target_plot->plant.name,player->nutrients,target_plot->soil_nutrient);
        return 1;
    }
//收获
int harvest(Player *player,Plot *plots, int plot_idx) {
            //checkpoint
            if (plot_idx<0||plot_idx >= player->plot_count) {
                printf("无效地块！\n");
                return  0;
            }
            Plot *target_plot = &plots[plot_idx];
            if (target_plot->is_empty) {
                printf("该地块没有种植植物，无法收获！\n");
                return  0;
            }
            Plant *plant = &target_plot->plant;
            if (plant->health <=0) {
                printf("%s已枯萎,无法获得收益！\n",plant->name);
                target_plot->is_empty = 1 ;
                memset(plant,0,sizeof(Plant));
                return  0;
            }
            if (plant->growth_value < 100 && plant->growth_stage < 5) {
                printf("%s未成熟(当前生长值：%d/100），无法收获！\n",
                       plant->name,plant->growth_value);
                return  0;
            }
            player->coins += plant->sell_price;
            printf("成功收获，获得%d金币，当前总金币：%d\n",
                plant->sell_price,player->coins);

            //重制地块
            target_plot->is_empty = 1;
            target_plot->planted_days = 0;
            target_plot->soil_water = 0;
            target_plot->soil_nutrient = 0;
            memset(plant,0,sizeof(Plant)); //清空植物数据
            return  1;
        }
