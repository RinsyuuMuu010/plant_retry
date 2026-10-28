/*
 * save.c —— 文本存档
 *
 * 为什么不用 fwrite 整个结构体：
 *   结构体布局随编译器/对齐/字段增删而变，旧档会读成乱码；
 *   文本格式能手改、能 diff、能在版本升级时逐字段兼容。
 *
 * 写入策略：先写 garden_save.dat.tmp，成功后替换正式档，
 * 避免写到一半断电/被杀导致存档半截损坏。
 */

#include "pgsm.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>

static int in_range(int v, int lo, int hi)
{
    return v >= lo && v <= hi;
}

int save_game(const Player *player, const Plot *plots)
{
    char tmp[128];
    FILE *fp;

    snprintf(tmp, sizeof tmp, "%s.tmp", SAVE_FILE);
    fp = fopen(tmp, "w");
    if (!fp) {
        ui_err("存档失败：无法写入 %s（目录只读？）", tmp);
        return 0;
    }

    fprintf(fp, "PGSM %d\n", SAVE_VERSION);
    fprintf(fp, "coins %d\n", player->coins);
    fprintf(fp, "waters %d\n", player->waters);
    fprintf(fp, "nutrients %d\n", player->nutrients);
    fprintf(fp, "day %d\n", player->day);
    fprintf(fp, "plots %d\n", player->plot_count);
    fprintf(fp, "weather %d\n", player->weather);
    fprintf(fp, "harvests %d\n", player->total_harvests);
    fprintf(fp, "earned %d\n", player->total_earned);
    fprintf(fp, "grown %d\n", player->grown_mask);
    fprintf(fp, "ach %u\n", player->achievements);

    for (int i = 0; i < PLANT_COUNT; i++)
        fprintf(fp, "seed %d %d\n", i, player->seeds[i]);

    for (int i = 0; i < player->plot_count; i++) {
        const Plot *p = &plots[i];
        fprintf(fp, "plot %d %d %d %d %d %d %d %d %d\n",
                i, p->empty ? 1 : 0, (int)p->plant.type, p->plant.growth,
                p->plant.health, p->plant.days_planted, p->plant.pest,
                p->soil_water, p->soil_nutrient);
    }

    if (fclose(fp) != 0) {
        ui_err("存档失败：数据没写完");
        remove(tmp);
        return 0;
    }

    remove(SAVE_FILE);                 /* Windows 的 rename 不覆盖已存在文件 */
    if (rename(tmp, SAVE_FILE) != 0) {
        ui_err("存档失败：无法替换旧存档");
        remove(tmp);
        return 0;
    }

    ui_ok("已保存（第 %d 天，%d 金币）", player->day, player->coins);
    return 1;
}

int load_game(Player *player, Plot *plots)
{
    FILE *fp;
    char line[256];
    int version = 0;
    int bad = 0;

    Player np;
    Plot nplots[MAX_PLOTS];

    fp = fopen(SAVE_FILE, "r");
    if (!fp) {
        ui_warn("没有找到存档 %s，先按 8 存一份吧", SAVE_FILE);
        return 0;
    }

    if (!fgets(line, sizeof line, fp) || sscanf(line, "PGSM %d", &version) != 1) {
        ui_err("存档读取失败：文件头不对");
        fclose(fp);
        return 0;
    }
    if (version != SAVE_VERSION) {
        ui_err("存档是 v%d 格式，当前程序只认 v%d（旧档已不兼容）", version, SAVE_VERSION);
        fclose(fp);
        return 0;
    }

    memset(&np, 0, sizeof np);
    np.plot_count = START_PLOTS;
    for (int i = 0; i < MAX_PLOTS; i++) plot_reset(&nplots[i]);

    while (fgets(line, sizeof line, fp)) {
        char key[16];

        if (sscanf(line, "%15s", key) != 1) continue;

        if      (!strcmp(key, "coins"))     sscanf(line, "%*s %d", &np.coins);
        else if (!strcmp(key, "waters"))    sscanf(line, "%*s %d", &np.waters);
        else if (!strcmp(key, "nutrients")) sscanf(line, "%*s %d", &np.nutrients);
        else if (!strcmp(key, "day"))       sscanf(line, "%*s %d", &np.day);
        else if (!strcmp(key, "plots"))     sscanf(line, "%*s %d", &np.plot_count);
        else if (!strcmp(key, "weather"))   sscanf(line, "%*s %d", &np.weather);
        else if (!strcmp(key, "harvests"))  sscanf(line, "%*s %d", &np.total_harvests);
        else if (!strcmp(key, "earned"))    sscanf(line, "%*s %d", &np.total_earned);
        else if (!strcmp(key, "grown"))     sscanf(line, "%*s %d", &np.grown_mask);
        else if (!strcmp(key, "ach"))       sscanf(line, "%*s %u", &np.achievements);
        else if (!strcmp(key, "seed")) {
            int t = -1, n = 0;
            if (sscanf(line, "%*s %d %d", &t, &n) == 2 && in_range(t, 0, PLANT_COUNT - 1) &&
                in_range(n, 0, 999))
                np.seeds[t] = n;
        } else if (!strcmp(key, "plot")) {
            int i = -1, empty = 0, type = 0, growth = 0, health = 0;
            int days = 0, pest = 0, sw = 0, sn = 0;
            if (sscanf(line, "%*s %d %d %d %d %d %d %d %d %d",
                       &i, &empty, &type, &growth, &health, &days, &pest, &sw, &sn) == 9 &&
                in_range(i, 0, MAX_PLOTS - 1)) {
                Plot *p = &nplots[i];
                p->empty = empty ? 1 : 0;
                p->plant.type = (PlantType)(in_range(type, 0, PLANT_COUNT - 1) ? type : 0);
                p->plant.growth = growth < 0 ? 0 : growth > PM_FULL ? PM_FULL : growth;
                p->plant.health = health < 0 ? 0 : health > HEALTH_CAP ? HEALTH_CAP : health;
                p->plant.days_planted = days < 0 ? 0 : days;
                p->plant.pest = pest ? 1 : 0;
                p->soil_water = sw < 0 ? 0 : sw > SOIL_CAP ? SOIL_CAP : sw;
                p->soil_nutrient = sn < 0 ? 0 : sn > SOIL_CAP ? SOIL_CAP : sn;
            }
        }
    }
    fclose(fp);

    /* 全量校验：越界就直接拒绝，不把坏档带进游戏 */
    if (!in_range(np.coins, 0, 9999999) || !in_range(np.waters, 0, 999999) ||
        !in_range(np.nutrients, 0, 999999) || !in_range(np.day, 1, 999999) ||
        !in_range(np.plot_count, START_PLOTS, MAX_PLOTS) ||
        !in_range(np.weather, 0, WEATHER_COUNT - 1) ||
        !in_range(np.total_harvests, 0, 9999999) ||
        !in_range(np.total_earned, 0, 99999999))
        bad = 1;

    if (bad) {
        ui_err("存档校验失败：数值越界，已拒绝载入（原存档未被修改）");
        return 0;
    }

    *player = np;
    for (int i = 0; i < MAX_PLOTS; i++) plots[i] = nplots[i];

    ui_ok("读档成功：第 %d 天，%d 金币，%d 块地", player->day, player->coins, player->plot_count);
    return 1;
}
