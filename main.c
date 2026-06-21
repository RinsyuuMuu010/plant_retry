//
// 像素花园模拟器 - 主入口
//
#include "pgsm.h"

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    system("title 像素花园模拟器 v1.0");
#endif

    srand((unsigned int)time(NULL));

    Player player;
    Plot   plots[MPC];

    init_player(&player);
    int start_coins = player.coins;  // 记录初始资金
    for (int i = 0; i < player.plot_count; i++) {
        plots[i].is_empty = 1;
        memset(&plots[i].plant, 0, sizeof(Plant));
        plots[i].planted_days = 0;
        plots[i].soil_water   = 0;
        plots[i].soil_nutrient = 0;
    }

    int choice;
    while (1) {
        show_main_ui(&player, plots);
        if (scanf("%d", &choice) != 1) {
            if (feof(stdin)) break;
            printf("\t输入无效\n");
            flush_stdin(); continue;
        }
        flush_stdin();

        switch (choice) {
            case 0: show_easter_egg(); break;  // 隐藏彩蛋
            case 1: {  // 种植
                int idx = get_plot(&player);
                printf("\n\t+===== 选种子 =====+\n");
                printf("\t| 1.萝卜%3d金      |\n", g_price[0]);
                printf("\t| 2.土豆%3d金      |\n", g_price[1]);
                printf("\t| 3.小麦%3d金      |\n", g_price[2]);
                printf("\t| 4.雏菊%3d金      |\n", g_price[3]);
                printf("\t+==================+\n\t选(1-4): ");
                int t;
                if (scanf("%d", &t) != 1 || t < 1 || t > 4) {
                    flush_stdin();
                    printf("\t无效选择\n"); break;
                }
                flush_stdin();
                plant_seed(&player, plots, idx, (PlantType)(t - 1));
                break;
            }
            case 2: water_plant(&player, plots, get_plot(&player)); break;
            case 3: fer_plant  (&player, plots, get_plot(&player)); break;
            case 4: harvest    (&player, plots, get_plot(&player)); break;
            case 5: pushtime   (&player, plots);                     break;
            case 6: save_game  (&player, plots);                     break;
            case 7:
                if (!load_game(&player, plots))
                    printf("\t加载失败\n");
                break;
            case 8: shop(&player);                                   break;
            case 9:
                show_exit_summary(start_coins, player.coins, player.day);
                return 0;
            default:
                printf("\t无效选项(1-9)\n");
        }
        if (feof(stdin)) break;
        printf("\t按回车继续...\n");
        flush_stdin();
    }
    return 0;
}
