//
// Created by Akimei1223 on 2026/1/10.
//
#include "pgsm.h"

const int Seed_prices[]={30,30,20,50,0}; //价位表
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
    // 执行种植 这里似乎有bug？
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
//游戏界面
void show_detail(Player *player,Plot *plots){
printf("==========迷你植物花园模拟经营 | 第 %d 天 | ==========\n",player->day);
printf("=-=-=-=-=-金币:%d | 水资源:%d | 肥料:%d | -=-=-=-=-=\n",player->coins,player->waters,player->nutrients);
printf(".................................................\n");
for(int i = 0; i < player->plot_count;i++){
   if(plots[i].is_empty){
      printf("地块%d:无植物\n",i);
                        }
   else{
printf("地块 %d:%s | 生长值:%d | 健康值:%d | 水分:%d | 养分:%d\n",
        i, plots[i].plant.name, plots[i].plant.growth_value,plots[i].plant.health, plots[i].soil_water, plots[i].soil_nutrient);
       }
                                          }
printf(".................................................\n");
printf("=================================================\n");

//
    printf("=================================================\n");
    printf("1. 种植种子  2. 浇水     3. 施肥     4. 收获\n");
    printf("5. 推进一天  6. 保存游戏  7. 加载游戏  8. 退出\n");
    printf("请选择操作（输入数字键）：");
    printf("=================================================\n");
}
//计时器
void pushtime(Player *player,Plot *plots) {
    player->day++;
    printf("==========进入第 %d 天==========\n",player->day);
    //更新地块状态
    for (int i=0;i<player->plot_count;i++) {
        Plot *plot = &plots[i];
        if (plot->is_empty) //跳过空地块
            continue;
        Plant *plant = &plot->plant;
        //每日消耗
        plot->soil_water -= plant->water_need;
        plot->soil_nutrient -= plant->nutrient_need;
        //防止负数
        if (plot->soil_water<0) {
            plot->soil_water = 0;
        }
        if (plot->soil_nutrient<0) {
            plot->soil_nutrient = 0;
        }
        //判断生长
        int enough_water = (plot->soil_water>=plant->water_need);
        int enough_nutri = (plot->soil_nutrient>=plant->nutrient_need);
        if (enough_nutri && enough_water) {
            //满足时
            plant->growth_value += 100 / plant->total_growth_days;
            if (plant->growth_value >100) {
                plant->growth_value = 100; //封顶
            }//debug 每次都打印阶段 此前此处未闭合
                plant->growth_stage = plant->growth_value / 20;//更新生长阶段
                printf("地块 %d： %s 正常，生长值+%d（当前为：%d）\n",
                    i,plant->name,100/plant->total_growth_days,plant->growth_value);
            //此前的}
        }
            else {
                //不满足时
                plant->health -= 20;
                if (plant->health < 0) {
                    plant->health = 0;
                }
                printf("地块 %d： %s 缺水或养分不足，健康值-%d（当前为：%d）\n",
                    i,plant->name,20,plant->health);
                //枯萎
                if (plant->health == 0) {
                    printf("地块 %d: %s 已枯萎ToT。\n",i,plant->name);
                    plot->is_empty = 1;
                    memset(plant,0,sizeof(Plant)); //清空
                }
            }
        }
    printf("==============================\n");
}
//输入
int get_plot(Player *player) {
    int idx;
    while (1) {
        printf("请输入0~%d之间的地块",player->plot_count-1);
        if (scanf("%d",&idx)!=1||idx<0||idx>=player->plot_count) {
            printf("无效地块,请输入存在的地块");
            while (getchar() != '\n');
        }else {
            break;
        }
    }
    return idx;
}
//商店？