/*
 * pgsm.c —— 游戏核心逻辑
 *
 * 这里只有规则，没有排版：所有提示都走 ui_* 接口，方便以后换前端。
 */

#include "pgsm.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* 植物表                                                              */
/* ------------------------------------------------------------------ */
static const PlantInfo PLANTS[PLANT_COUNT] = {
    /* 名称       代号      种  售  天  水 肥  解锁 当季     介绍 */
    { "胡萝卜", "Carrot", 30, 150, 4,  8, 3,  0, SEASON_AUTUMN, "四周就能收，新手之友" },
    { "土豆",   "Potato", 35, 200, 5,  7, 3,  0, SEASON_AUTUMN, "耐旱一点，稳赚不赔" },
    { "小麦",   "Wheat",  20,  90, 3,  6, 2,  0, SEASON_SUMMER, "便宜、长得快，积少成多" },
    { "雏菊",   "Daisy",  60, 330, 6, 10, 4,  0, SEASON_SPRING, "好看，但也娇气" },
    { "仙人掌", "Cactus", 55, 300, 6,  3, 2,  3, SEASON_SUMMER, "几乎不用浇水，慢热型" },
    { "玫瑰",   "Rose",   90, 520, 8, 12, 5,  6, SEASON_SPRING, "成本高，回报也高" },
};

const PlantInfo *plant_info(PlantType type)
{
    if (type < 0 || type >= PLANT_COUNT) type = PLANT_CARROT;
    return &PLANTS[type];
}

int plant_unlocked(const Player *player, PlantType type)
{
    return player->total_harvests >= plant_info(type)->unlock_harvests;
}

const char *season_name(Season season)
{
    static const char *names[SEASON_COUNT] = { "春", "夏", "秋", "冬" };
    if (season < 0 || season >= SEASON_COUNT) return "?";
    return names[season];
}

const char *weather_name(Weather weather)
{
    static const char *names[WEATHER_COUNT] = { "晴", "多云", "雨", "暴风雨" };
    if (weather < 0 || weather >= WEATHER_COUNT) return "?";
    return names[weather];
}

Season season_of_day(int day)
{
    if (day < 1) day = 1;
    return (Season)(((day - 1) / DAYS_PER_SEASON) % SEASON_COUNT);
}

int count_seeds(const Player *player)
{
    int n = 0;
    for (int i = 0; i < PLANT_COUNT; i++) n += player->seeds[i];
    return n;
}

/* ------------------------------------------------------------------ */
/* 随机事件                                                            */
/* ------------------------------------------------------------------ */
static int g_price_bonus = 0;      /* 当日售价加成（百分比） */

typedef enum {
    EVENT_NONE = 0,
    EVENT_MARKET,
    EVENT_FEST,
    EVENT_DEW,
    EVENT_PEST
} DayEvent;

/* ------------------------------------------------------------------ */
/* 初始化                                                              */
/* ------------------------------------------------------------------ */
void plot_reset(Plot *plot)
{
    plot->empty = 1;
    plot->soil_water = 0;
    plot->soil_nutrient = 0;
    plot->plant.type = PLANT_CARROT;
    plot->plant.growth = 0;
    plot->plant.health = 0;
    plot->plant.days_planted = 0;
    plot->plant.pest = 0;
}

void game_new(Player *player, Plot *plots)
{
    player->coins = 400;
    player->waters = 40;
    player->nutrients = 20;
    player->day = 1;
    player->plot_count = START_PLOTS;
    player->weather = WEATHER_SUNNY;
    player->total_harvests = 0;
    player->total_earned = 0;
    player->grown_mask = 0;
    player->achievements = 0;
    for (int i = 0; i < PLANT_COUNT; i++) player->seeds[i] = 0;

    /* 新手礼包：三颗胡萝卜、两颗小麦，够玩两轮 */
    player->seeds[PLANT_CARROT] = 3;
    player->seeds[PLANT_WHEAT] = 2;

    for (int i = 0; i < MAX_PLOTS; i++) plot_reset(&plots[i]);
    g_price_bonus = 0;
}

int count_planted(const Player *player, const Plot *plots)
{
    int n = 0;
    for (int i = 0; i < player->plot_count; i++)
        if (!plots[i].empty) n++;
    return n;
}

/* ------------------------------------------------------------------ */
/* 成就                                                                */
/* ------------------------------------------------------------------ */
void check_achievements(Player *player, const Plot *plots)
{
    struct { unsigned bit; const char *name; int hit; } list[] = {
        { ACH_FIRST_HARVEST, "初来乍到",   player->total_harvests >= 1 },
        { ACH_HARVEST_10,    "熟练农夫",   player->total_harvests >= 10 },
        { ACH_RICH_1000,     "小有积蓄",   player->total_earned >= 1000 },
        { ACH_FULL_GARDEN,   "满园春色",   player->plot_count > 0 &&
                                          count_planted(player, plots) == player->plot_count },
        { ACH_COLLECTOR,     "植物收藏家", player->grown_mask == (1 << PLANT_COUNT) - 1 },
    };

    for (size_t i = 0; i < sizeof list / sizeof list[0]; i++) {
        if (list[i].hit && !(player->achievements & list[i].bit)) {
            player->achievements |= list[i].bit;
            ui_ok("成就解锁 · %s", list[i].name);
        }
    }
}

/* ------------------------------------------------------------------ */
/* 种植与日常操作                                                      */
/* ------------------------------------------------------------------ */
int plant_seed(Player *player, Plot *plots, int plot_idx, PlantType type)
{
    if (plot_idx < 0 || plot_idx >= player->plot_count) {
        ui_err("没有第 %d 号地块", plot_idx + 1);
        return 0;
    }
    if (type < 0 || type >= PLANT_COUNT) {
        ui_err("不存在的种子");
        return 0;
    }

    Plot *plot = &plots[plot_idx];
    const PlantInfo *info = plant_info(type);

    if (!plot->empty) {
        ui_warn("第 %d 号地块已经种了%s", plot_idx + 1, plant_info(plot->plant.type)->name);
        return 0;
    }
    if (!plant_unlocked(player, type)) {
        ui_warn("%s 还没解锁：累计收获 %d 次后才能买它的种子（当前 %d 次）",
                info->name, info->unlock_harvests, player->total_harvests);
        return 0;
    }
    if (player->seeds[type] <= 0) {
        ui_warn("背包里没有%s种子了，去商店买（%d 金币一颗）", info->name, info->seed_price);
        return 0;
    }

    player->seeds[type]--;
    plot->empty = 0;
    plot->soil_water = 50;
    plot->soil_nutrient = 40;
    plot->plant.type = type;
    plot->plant.growth = 0;
    plot->plant.health = HEALTH_CAP;
    plot->plant.days_planted = 0;
    plot->plant.pest = 0;

    player->grown_mask |= 1 << (int)type;

    ui_ok("第 %d 号地块种下了%s（背包还剩 %d 颗）",
          plot_idx + 1, info->name, player->seeds[type]);
    return 1;
}

int water_plant(Player *player, Plot *plots, int plot_idx)
{
    if (plot_idx < 0 || plot_idx >= player->plot_count) {
        ui_err("没有第 %d 号地块", plot_idx + 1);
        return 0;
    }
    Plot *plot = &plots[plot_idx];
    if (plot->empty) {
        ui_warn("第 %d 号地块是空地", plot_idx + 1);
        return 0;
    }
    if (player->waters < WATER_PER_USE) {
        ui_warn("水不够：浇一次要 %d，你有 %d（商店 1 金币 1 单位）", WATER_PER_USE, player->waters);
        return 0;
    }
    if (plot->soil_water >= SOIL_CAP) {
        ui_note("第 %d 号地块的水已经满了", plot_idx + 1);
        return 0;
    }

    player->waters -= WATER_PER_USE;
    plot->soil_water += SOIL_WATER_GAIN;
    if (plot->soil_water > SOIL_CAP) plot->soil_water = SOIL_CAP;

    ui_ok("浇过水了：土壤水分 %d，剩余水 %d", plot->soil_water, player->waters);
    return 1;
}

int fertilize_plant(Player *player, Plot *plots, int plot_idx)
{
    if (plot_idx < 0 || plot_idx >= player->plot_count) {
        ui_err("没有第 %d 号地块", plot_idx + 1);
        return 0;
    }
    Plot *plot = &plots[plot_idx];
    if (plot->empty) {
        ui_warn("第 %d 号地块是空地", plot_idx + 1);
        return 0;
    }
    if (player->nutrients < FERT_PER_USE) {
        ui_warn("肥料不够：施一次要 %d，你有 %d（商店 2 金币 1 单位）", FERT_PER_USE, player->nutrients);
        return 0;
    }
    if (plot->soil_nutrient >= SOIL_CAP) {
        ui_note("第 %d 号地块的养分已经满了", plot_idx + 1);
        return 0;
    }

    player->nutrients -= FERT_PER_USE;
    plot->soil_nutrient += SOIL_FERT_GAIN;
    if (plot->soil_nutrient > SOIL_CAP) plot->soil_nutrient = SOIL_CAP;

    ui_ok("施过肥了：土壤养分 %d，剩余肥料 %d", plot->soil_nutrient, player->nutrients);
    return 1;
}

int cure_pest(Player *player, Plot *plots, int plot_idx)
{
    if (plot_idx < 0 || plot_idx >= player->plot_count) {
        ui_err("没有第 %d 号地块", plot_idx + 1);
        return 0;
    }
    Plot *plot = &plots[plot_idx];
    if (plot->empty) {
        ui_warn("第 %d 号地块是空地", plot_idx + 1);
        return 0;
    }
    if (!plot->plant.pest) {
        ui_note("第 %d 号地块没有虫害", plot_idx + 1);
        return 0;
    }
    if (player->coins < CURE_PRICE) {
        ui_warn("除虫要 %d 金币，你只有 %d", CURE_PRICE, player->coins);
        return 0;
    }

    player->coins -= CURE_PRICE;
    plot->plant.pest = 0;
    plot->plant.health += 15;
    if (plot->plant.health > HEALTH_CAP) plot->plant.health = HEALTH_CAP;

    ui_ok("除虫完成：%s 恢复精神（-%d 金币）", plant_info(plot->plant.type)->name, CURE_PRICE);
    return 1;
}

int harvest_plant(Player *player, Plot *plots, int plot_idx)
{
    if (plot_idx < 0 || plot_idx >= player->plot_count) {
        ui_err("没有第 %d 号地块", plot_idx + 1);
        return 0;
    }
    Plot *plot = &plots[plot_idx];
    if (plot->empty) {
        ui_warn("第 %d 号地块是空地", plot_idx + 1);
        return 0;
    }
    if (plot->plant.growth < PM_FULL) {
        ui_warn("%s 还没熟（%d%%），再等等", plant_info(plot->plant.type)->name,
                plot->plant.growth / 10);
        return 0;
    }

    const PlantInfo *info = plant_info(plot->plant.type);
    int quality = 50 + plot->plant.health / 2;            /* 50% ~ 100% */
    int price = info->sell_price * quality / 100;
    price = price * (100 + g_price_bonus) / 100;
    if (price < 1) price = 1;

    player->coins += price;
    player->total_earned += price;
    player->total_harvests++;

    ui_ok("收获%s：+%d 金币（品质 %d%%，现有 %d 金币）",
          info->name, price, quality, player->coins);

    plot_reset(plot);
    check_achievements(player, plots);
    return 1;
}

/* ------------------------------------------------------------------ */
/* 推进一天                                                            */
/* ------------------------------------------------------------------ */
static Weather roll_weather(Season season)
{
    int r = rand() % 100;
    switch (season) {
        case SEASON_SPRING: return r < 40 ? WEATHER_SUNNY : r < 70 ? WEATHER_CLOUDY : r < 92 ? WEATHER_RAIN : WEATHER_STORM;
        case SEASON_SUMMER: return r < 55 ? WEATHER_SUNNY : r < 75 ? WEATHER_CLOUDY : r < 90 ? WEATHER_RAIN : WEATHER_STORM;
        case SEASON_AUTUMN: return r < 45 ? WEATHER_SUNNY : r < 72 ? WEATHER_CLOUDY : r < 90 ? WEATHER_RAIN : WEATHER_STORM;
        default:            return r < 25 ? WEATHER_SUNNY : r < 55 ? WEATHER_CLOUDY : r < 70 ? WEATHER_RAIN : WEATHER_STORM;
    }
}

static DayEvent roll_event(void)
{
    int r = rand() % 100;
    if (r < 8)  return EVENT_MARKET;
    if (r < 16) return EVENT_FEST;
    if (r < 26) return EVENT_DEW;
    if (r < 36) return EVENT_PEST;
    return EVENT_NONE;
}

static int weather_bonus(Weather w)
{
    switch (w) {
        case WEATHER_SUNNY: return 10;
        case WEATHER_RAIN:  return 5;
        case WEATHER_STORM: return -10;
        default:            return 0;
    }
}

void advance_day(Player *player, Plot *plots)
{
    player->day++;
    Season season = season_of_day(player->day);
    Weather weather = roll_weather(season);
    DayEvent event = roll_event();
    player->weather = weather;
    g_price_bonus = 0;

    ui_info("── 第 %d 天 · %s · %s ──", player->day, season_name(season), weather_name(weather));

    if (season != season_of_day(player->day - 1)) {
        const char *best = NULL;
        for (int i = 0; i < PLANT_COUNT; i++) {
            if (PLANTS[i].best_season == season) { best = PLANTS[i].name; break; }
        }
        if (best) ui_warn("换季了：%s 到了，%s 正当季，长得更快", season_name(season), best);
        else      ui_warn("换季了：%s 到了，作物普遍长得慢一些", season_name(season));
    }

    switch (event) {
        case EVENT_MARKET:
            g_price_bonus = 30;
            ui_warn("集市开张：今天卖出的作物多卖 30%%");
            break;
        case EVENT_FEST:
            ui_warn("丰收日：今天所有作物生长速度 +25%%");
            break;
        case EVENT_DEW:
            for (int i = 0; i < player->plot_count; i++) {
                if (plots[i].empty) continue;
                plots[i].soil_water += 15;
                if (plots[i].soil_water > SOIL_CAP) plots[i].soil_water = SOIL_CAP;
            }
            ui_info("夜里有露水：所有地块水分 +15");
            break;
        case EVENT_PEST:
            ui_warn("虫害爆发：有些地块的作物被虫子盯上了");
            break;
        default:
            break;
    }

    int bonus = weather_bonus(weather);
    if (season == SEASON_WINTER) bonus -= 15;      /* 冬天长得慢 */
    if (event == EVENT_FEST) bonus += 25;

    for (int i = 0; i < player->plot_count; i++) {
        Plot *plot = &plots[i];
        if (plot->empty) continue;

        Plant *plant = &plot->plant;
        const PlantInfo *info = plant_info(plant->type);

        /* 成熟后不再消耗，也不会枯萎：让玩家从容卖出 */
        if (plant->growth >= PM_FULL) {
            ui_note("  %d 号地块  %s 已成熟，等你收获", i + 1, info->name);
            continue;
        }

        plant->days_planted++;

        int need_water = info->water_need;
        int need_food = info->nutrient_need;
        if (weather == WEATHER_RAIN && need_water > 1) need_water /= 2;
        if (weather == WEATHER_SUNNY) need_water += 2;
        if (weather == WEATHER_STORM) need_water += 3;

        int ok_water = plot->soil_water >= need_water;
        int ok_food = plot->soil_nutrient >= need_food;

        plot->soil_water -= need_water;
        if (plot->soil_water < 0) plot->soil_water = 0;
        plot->soil_nutrient -= need_food;
        if (plot->soil_nutrient < 0) plot->soil_nutrient = 0;

        if (ok_water && ok_food) {
            int base = PM_FULL / info->growth_days;
            int gain = base + base * bonus / 100;
            if (season == info->best_season) gain += gain / 5;
            if (plant->pest) gain /= 2;
            if (gain < 1) gain = 1;

            plant->growth += gain;
            if (plant->growth > PM_FULL) plant->growth = PM_FULL;

            if (plant->health < HEALTH_CAP) {
                plant->health += 5;
                if (plant->health > HEALTH_CAP) plant->health = HEALTH_CAP;
            }

            ui_note("  %d 号地块  %s 生长 +%d（%d%%）", i + 1, info->name,
                    gain, plant->growth / 10);
        } else {
            int loss = (ok_water || ok_food) ? 12 : 20;
            if (plant->pest) loss += 8;

            plant->health -= loss;
            if (plant->health < 0) plant->health = 0;

            char lack[32] = "";
            if (!ok_water) strcpy(lack, "缺水");
            if (!ok_food)  strcat(lack, "缺肥");

            ui_warn("  %d 号地块  %s %s，健康 -%d（剩 %d）",
                    i + 1, info->name, lack, loss, plant->health);
        }

        if (weather == WEATHER_STORM && plant->health > 0) {
            plant->health -= 8;
            if (plant->health < 0) plant->health = 0;
        }

        if (event == EVENT_PEST && rand() % 100 < 40 && !plant->pest && plant->growth < PM_FULL) {
            plant->pest = 1;
            ui_warn("  %d 号地块  %s 长了虫子，快去按 4 除虫", i + 1, info->name);
        } else if (rand() % 100 < 6 && !plant->pest && plant->growth < PM_FULL * 9 / 10) {
            plant->pest = 1;
            ui_warn("  %d 号地块  %s 长了虫子", i + 1, info->name);
        }

        if (plant->health <= 0) {
            ui_err("  %d 号地块  %s 枯萎了，地块已清空", i + 1, info->name);
            plot_reset(plot);
        }
    }

    /* 破产保护：没地、没种子、没钱时送一颗胡萝卜种子，避免卡死 */
    if (player->coins < 1 && count_seeds(player) == 0 && count_planted(player, plots) == 0) {
        player->seeds[PLANT_CARROT] += 1;
        ui_info("邻居送了你一颗胡萝卜种子，别放弃");
    }

    check_achievements(player, plots);
}

/* ------------------------------------------------------------------ */
/* 商店                                                                */
/* ------------------------------------------------------------------ */
int buy_seed(Player *player, PlantType type)
{
    const PlantInfo *info = plant_info(type);

    if (type < 0 || type >= PLANT_COUNT) {
        ui_err("没有这种种子");
        return 0;
    }
    if (!plant_unlocked(player, type)) {
        ui_warn("%s 还没解锁：累计收获 %d 次（当前 %d 次）",
                info->name, info->unlock_harvests, player->total_harvests);
        return 0;
    }
    if (player->coins < info->seed_price) {
        ui_warn("金币不够：%s种子 %d 金币，你有 %d", info->name, info->seed_price, player->coins);
        return 0;
    }

    player->coins -= info->seed_price;
    player->seeds[type]++;
    ui_ok("买了 1 颗%s种子（-%d 金币），背包共 %d 颗",
          info->name, info->seed_price, player->seeds[type]);
    return 1;
}

int buy_water(Player *player, int amount)
{
    int cost;
    if (amount <= 0) {
        ui_warn("数量要大于 0");
        return 0;
    }
    cost = amount * WATER_PRICE;
    if (player->coins < cost) {
        ui_warn("金币不够：水要 %d，你只有 %d", cost, player->coins);
        return 0;
    }
    player->coins -= cost;
    player->waters += amount;
    ui_ok("买了 %d 单位水（-%d 金币），现在有 %d", amount, cost, player->waters);
    return 1;
}

int buy_fertilizer(Player *player, int amount)
{
    int cost;
    if (amount <= 0) {
        ui_warn("数量要大于 0");
        return 0;
    }
    cost = amount * FERT_PRICE;
    if (player->coins < cost) {
        ui_warn("金币不够：肥料要 %d，你只有 %d", cost, player->coins);
        return 0;
    }
    player->coins -= cost;
    player->nutrients += amount;
    ui_ok("买了 %d 单位肥料（-%d 金币），现在有 %d", amount, cost, player->nutrients);
    return 1;
}

int buy_plot(Player *player, Plot *plots)
{
    if (player->plot_count >= MAX_PLOTS) {
        ui_warn("最多只能有 %d 块地", MAX_PLOTS);
        return 0;
    }
    if (player->coins < PLOT_PRICE) {
        ui_warn("扩建要 %d 金币，你只有 %d", PLOT_PRICE, player->coins);
        return 0;
    }
    player->coins -= PLOT_PRICE;
    plot_reset(&plots[player->plot_count]);
    player->plot_count++;
    ui_ok("扩建完成：现在有 %d 块地（-%d 金币）", player->plot_count, PLOT_PRICE);
    return 1;
}

static void seed_shop(Player *player)
{
    for (;;) {
        int choice = 0;
        ui_seed_screen(player, 1);
        if (!ui_prompt("买哪种（0 返回）", 0, PLANT_COUNT, &choice)) return;
        if (choice == 0) return;
        buy_seed(player, (PlantType)(choice - 1));
    }
}

void shop(Player *player, Plot *plots)
{
    for (;;) {
        int choice = 0;
        ui_shop_screen(player);
        if (!ui_prompt("选一项（0 返回）", 0, 4, &choice)) return;

        switch (choice) {
            case 1: buy_water(player, 10); break;
            case 2: buy_fertilizer(player, 10); break;
            case 3: seed_shop(player); break;
            case 4: buy_plot(player, plots); break;
            default: return;
        }
        if (choice != 3) ui_pause();
    }
}
