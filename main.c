#include <stdio.h>
#include "pgsm.h"

int main(void) {
    Player player = {
        .coins = 1000,
        .waters = 50,
        .nutrients = 20,
        .day = 1,
        .plot_count = MPC,
    };
    Plot plots[4];

    for (int i=0;i< player.plot_count;i++) {
        plots[i].is_empty = 1;
        memset(&plots[i].plant,0,sizeof(Plant));
        plots[i].planted_days = 0;
        plots[i].soil_water = 0;
        plots[i].soil_nutrient = 0;
    }

    //啥都没写
    int choice;
    while (1) {
        show_detail(&player,plots);
        while (scanf("%d",&choice) != 1) {
            printf("输入无效，请输入数字：");
            while (getchar()!='\n');
        }
        while (getchar()!='\n');
        switch (choice) {
            case 1: {
                int plot_idx = get_plot(&player);
                printf("==========选择植物==========");
                printf("-->1.萝卜 <------> 2.土豆<--");
                printf("-->3.小麦 <------> 4.雏菊<--");
                printf("==========选择植物==========");
                int plant_choice;
                while (scanf("%d",&plant_choice) !=1 ||plant_choice <1 ||plant_choice >4 ) {
                    printf("无效选择！请输入1-4：");
                    while (getchar() != '\n');
                }
                plant_seed(&player,plots,plot_idx,plant_choice-1);
                break;
            }
            case 2: {
                int plot_idx = get_plot(&player);
                water_plant(&player,plots,plot_idx);   //浇水
                break;
            }
            case 3: {
                int plot_idx = get_plot(&player);
                fer_plant(&player,plots,plot_idx);  //施肥
                break;
            }
            case 4: {
                int plot_idx = get_plot(&player);
                harvest(&player,plots,plot_idx); //收获
                break;
            }
            case 5: {
                pushtime(&player,plots);
                break;
            }
            case 6: {
                save_game(&player,plots);
                break;
            }
            case 7: {
                if (load_game(&player,plots)) {
                    printf("加载成功!\n");
                }else{
                    printf("加载失败!\n");
                }
                break;
            }
            case 8: { // 商店
                shop(&player);
                break;
            }
            case 9: { // 退出游戏
                printf("感谢游玩！再见～\n");
                return 0;
            }
            default:
                printf("选择无效，请选择1-9之间的数字！\n");
                break;
        }
        printf("Press any key to continue");
        while (getchar()!='\n');
                //写一坨不知道有啥用
        //中文显示有bug
        //?

        }
        return 0;
    }