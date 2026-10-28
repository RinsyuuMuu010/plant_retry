/*
 * main.c —— 入口与主循环
 *
 * 这一层只做“把用户的操作翻译成一次函数调用”，
 * 规则在 pgsm.c，排版在 ui.c，持久化在 save.c。
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pgsm.h"
#include "ui.h"

/* 询问地块编号：返回 1 = 选好，0 = 输入结束，-1 = 用户取消 */
static int ask_plot(const Player *player, int *out)
{
    char prompt[80];
    int v = 0;

    snprintf(prompt, sizeof prompt, "第几号地块（1-%d，0 取消）", player->plot_count);
    if (!ui_prompt(prompt, 0, player->plot_count, &v)) return 0;
    if (v == 0) return -1;

    *out = v - 1;
    return 1;
}

int main(void)
{
    Player player;
    Plot plots[MAX_PLOTS];
    int running = 1;

    srand((unsigned)time(NULL));
    ui_init();
    game_new(&player, plots);

    ui_title();
    ui_note("输入数字后回车。1-5 打理花园，6 商店，7 下一天，8/9 存读档，0 退出。");
    ui_pause();

    while (running) {
        int choice = 0;

        ui_screen(&player, plots);
        if (!ui_prompt("选择操作", 0, 9, &choice)) {
            printf("\n");
            break;
        }
        if (choice == 0) break;

        if (choice >= 1 && choice <= 5) {
            int idx = 0;
            int rc = ask_plot(&player, &idx);

            if (rc == 0) break;        /* 输入流结束，直接收工 */
            if (rc < 0) continue;      /* 取消了，回主菜单 */

            if (choice == 1) {
                int type = 0;
                ui_seed_screen(&player, 0);
                if (!ui_prompt("种哪种（0 取消）", 0, PLANT_COUNT, &type)) break;
                if (type > 0) plant_seed(&player, plots, idx, (PlantType)(type - 1));
            } else if (choice == 2) {
                water_plant(&player, plots, idx);
            } else if (choice == 3) {
                fertilize_plant(&player, plots, idx);
            } else if (choice == 4) {
                cure_pest(&player, plots, idx);
            } else {
                harvest_plant(&player, plots, idx);
            }

            ui_pause();
            continue;
        }

        switch (choice) {
            case 6:
                shop(&player, plots);
                break;
            case 7:
                advance_day(&player, plots);
                ui_pause();
                break;
            case 8:
                save_game(&player, plots);
                ui_pause();
                break;
            case 9:
                load_game(&player, plots);
                ui_pause();
                break;
            default:
                break;
        }
    }

    printf("\n  再见，记得给花浇水。\n\n");
    return 0;
}
