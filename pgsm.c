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
    if (plot_idx < 0 || plot_idx >= MPC) {
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
        printf("不存在的植物。\n");
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
    if (plot_idx<0||plot_idx >= MPC ) {
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
        if (plot_idx<0||plot_idx >= MPC) {
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
            if (plot_idx<0||plot_idx >= MPC) {
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
void show_detail(Player *player, Plot *plots) {
    printf("==========迷你植物花园模拟经营 | 第 %d 天 | ==========\n", player->day);
    printf("=-=-=-=-=-金币:%d | 水资源:%d | 肥料:%d | -=-=-=-=-=\n",
           player->coins, player->waters, player->nutrients);
    printf(".................................................\n");
    for (int i = 0; i < player->plot_count; i++) {
        if (plots[i].is_empty) {
            printf("地块%d: 无植物\n", i);
        } else {
            printf("地块 %d: %s | 生长值:%d | 健康值:%d | 水分:%d | 养分:%d\n",
                   i, plots[i].plant.name, plots[i].plant.growth_value,
                   plots[i].plant.health, plots[i].soil_water, plots[i].soil_nutrient);
        }
    }
    printf(".................................................\n");
    printf("=================================================\n");
    printf("1. 种植种子  2. 浇水     3. 施肥     4. 收获\n");
    printf("5. 推进一天  6. 保存游戏  7. 加载游戏  8. 商店  9. 退出\n");
    printf("=================================================\n");
    printf("请选择操作（输入数字键）：");
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
        printf("请输入0~%d之间的地块",MPC -1);
        if (scanf("%d",&idx)!=1||idx<0||idx>=MPC) {
            printf("无效地块,请输入存在的地块");
            while (getchar() != '\n');
        }else {
            break;
        }
    }
    return idx;
}
//商店？
void shop_menu(Player *player) {
    printf("\n========== 植物花园商店 ==========\n");
    printf("当前金币：%d\n", player->coins);
    printf("----------------------------------\n");
    printf("【种子区】\n");
    printf("1. 胡萝卜种子   - 30金币\n");
    printf("2. 土豆种子   - 30金币\n");
    printf("3. 小麦种子   - 20金币\n");
    printf("4. 雏菊种子   - 50金币\n");
    printf("----------------------------------\n");
    printf("【道具区】\n");
    printf("5. 水资源     - 5金币/单位\n");
    printf("6. 肥料       - 8金币/单位\n");
    printf("----------------------------------\n");
    printf("7. 退出商店\n"); // 仅保留退出选项
    printf("==================================\n");
    printf("请选择购买项（输入数字）：");
}
//买种
int buy_seed(Player *player,PlantType plant_type){
    if (plant_type< Carrot || plant_type > Daisy ) {
        printf("非法种子");
        return  0;
    }
    int seed_price = Seed_prices[plant_type];
    if (player->coins < seed_price ) {
        printf("金币不足，需要%d金币，当前只有%d\n",seed_price,player->coins);
        return  0;
    }
    player->coins -= seed_price;
    const char *seed_names[]={"胡萝卜","土豆","小麦","雏菊"};
    printf("成功购买%s种子，剩余金币：%d\n",seed_names[plant_type],player->coins);
    return  1;
}
//买水
int buy_water(Player *player,int amount) {
    if (amount<=0){
        printf("你买个锤子呢，发你空气？！");
        return  0;
    }
    int total_cost = amount * WP;
    if (player->coins < total_cost ) {
        printf("金币不足，需要%d金币，当前只有%d\n，请攒够再来吧~",total_cost,player->coins);
        return 0;
    }
    player->coins -= total_cost;
    player->waters += amount;
    printf("成功购买 %d 水资源,剩余金币：%d,当前水资源 %d\n",amount,player->coins,player->waters);
    return 1;
}
//买肥料
int buy_nutrient(Player *player, int amount) {
    if (amount<=0){
    printf("你买个锤子呢，发你空气？！");
    return  0;
}
    int total_cost = amount * NP;
    if (player->coins < total_cost ) {
        printf("金币不足，需要%d金币，当前只有%d\n，请攒够再来吧~",total_cost,player->coins);
        return 0;
    }
    player->coins -= total_cost;
    player->nutrients += amount;
    printf("成功购买 %d 肥料,剩余金币：%d,当前肥料 %d\n",amount,player->coins,player->nutrients);
    return 1;
}
//商店输入?
void shop(Player *player) {
    int choice;
    while (1) {
        shop_menu(player);
        while (scanf("%d", &choice) != 1) {
            printf("输入无效，请输入数字！\n");
            while (getchar() != '\n');
        }
        while (getchar() != '\n'); // 清空换行符

        switch (choice) {
            case 1: // 胡萝卜种子
                buy_seed(player, Carrot);
                break;
            case 2: // 土豆种子
                buy_seed(player, Potato);
                break;
            case 3: // 小麦种子
                buy_seed(player, Wheat);
                break;
            case 4: // 雏菊种子
                buy_seed(player, Daisy);
                break;
            case 5: { // 购买水
                int amount;
                printf("请输入购买水资源的数量：");
                while (scanf("%d", &amount) != 1 || amount <= 0) {
                    printf("无效数量！请输入大于0的数字：");
                    while (getchar() != '\n');
                }
                buy_water(player, amount);
                break;
            }
            case 6: { // 购买肥料
                int amount;
                printf("请输入购买肥料的数量：");
                while (scanf("%d", &amount) != 1 || amount <= 0) {
                    printf("无效数量！请输入大于0的数字：");
                    while (getchar() != '\n');
                }
                buy_nutrient(player, amount);
                break;
            }
            case 7: // 退出商店
                printf("退出商店！\n");
                return;
            default:
                printf("无效选择！请重新输入\n");
        }
        printf("按任意键继续...");
        getchar();
    }
}

//保存&加载
// 保存游戏
void save_game(Player *player, Plot *plots) {
    FILE *fp = fopen(SL, "wb");
    if (!fp) {
        printf("存档失败！无法打开文件\n");
        return;
    }
    // 先保存玩家数据，再保存地块数据
    fwrite(player, sizeof(Player), 1, fp);
    fwrite(plots, sizeof(Plot), player->plot_count, fp);
    fclose(fp);
    printf("游戏保存成功！\n");
}

// 加载游戏
int load_game(Player *player, Plot *plots) {
    FILE *fp = fopen(SL, "rb");
    if (!fp) {
        printf("读档失败！存档文件不存在\n");
        return 0;
    }
    // 读取玩家数据
    if (fread(player, sizeof(Player), 1, fp) != 1) {
        fclose(fp);
        printf("读档失败！玩家数据损坏\n");
        return 0;
    }
    // 读取地块数据
    if (fread(plots, sizeof(Plot), player->plot_count, fp) != player->plot_count) {
        fclose(fp);
        printf("读档失败！地块数据损坏\n");
        return 0;
    }
    fclose(fp);
    printf("游戏加载成功！\n");
    return 1;
}