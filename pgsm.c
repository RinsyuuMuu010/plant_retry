//
// 像素花园模拟器 - 核心逻辑实现
//
#include "pgsm.h"
#include <stdarg.h>

#ifdef _WIN32
void set_color(int fg, int bg) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)(fg | (bg << 4)));
}
void reset_color(void) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}
#endif

// ─── 清屏 ─────────────────────────────────────────
void clrscr(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD topLeft = {0, 0};
    DWORD written;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    DWORD size = (DWORD)(csbi.dwSize.X * csbi.dwSize.Y);
    FillConsoleOutputCharacter(hOut, ' ', size, topLeft, &written);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, size, topLeft, &written);
    SetConsoleCursorPosition(hOut, topLeft);
#else
    printf("\033[2J\033[H");
#endif
}

// ─── 经济数据 ─────────────────────────────────────
// 种子价格表
const int g_price[] = {100, 120, 80, 200};
// 植物中文名表
const char *g_cname[] = {"萝卜", "土豆", "小麦", "雏菊"};

// ─── 输入辅助 ─────────────────────────────────────
void flush_stdin(void) {
    int c;
    if (feof(stdin)) return;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

// ─── 操作日志 ─────────────────────────────────────
static char  g_log[LOG_MAX][64];
static int   g_log_cnt = 0;
static int   g_log_idx = 0;

void log_clear(void) {
    g_log_cnt = 0; g_log_idx = 0;
    memset(g_log, 0, sizeof(g_log));
}

void log_add(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_log[g_log_idx], sizeof(g_log[0]), fmt, args);
    va_end(args);
    if (g_log_cnt < LOG_MAX) g_log_cnt++;
    g_log_idx = (g_log_idx + 1) % LOG_MAX;
}

// 获取第 i 条日志（0=最旧, g_log_cnt-1=最新）
const char* log_get(int i) {
    if (i < 0 || i >= g_log_cnt) return NULL;
    int idx = (g_log_idx - g_log_cnt + i + LOG_MAX) % LOG_MAX;
    return g_log[idx];
}

// ─── 本地时间 ─────────────────────────────────────
void get_time_str(char *buf, int sz) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    if (tm_info)
        strftime(buf, (size_t)sz, "%H:%M:%S", tm_info);
    else
        strncpy(buf, "--:--:--", (size_t)sz);
}

// ─── 名称/图标查询 ─────────────────────────────────
const char *weather_name(Weather w) {
    switch (w) {
        case WEATHER_SUNNY:  return "晴天";
        case WEATHER_CLOUDY: return "阴天";
        case WEATHER_RAINY:  return "雨天";
        default: return "---";
    }
}
const char *weather_icon(Weather w) {
    switch (w) {
        case WEATHER_SUNNY:  return "*";
        case WEATHER_CLOUDY: return "~";
        case WEATHER_RAINY:  return "\"\"";
        default: return "?";
    }
}
const char *stage_name(int s) {
    switch (s) {
        case 0: return "种子"; case 1: return "幼芽"; case 2: return "成长";
        case 3: return "茂盛"; case 4: return "开花"; case 5: return "成熟";
        default: return "???";
    }
}
const char *stage_icon(int s) {
    switch (s) {
        case 0: return "."; case 1: return ":"; case 2: return "o";
        case 3: return "O"; case 4: return "0"; case 5: return "@";
        default: return "?";
    }
}
const char *plant_cn_name(PlantType t) {
    if (t >= Carrot && t <= Daisy) return g_cname[t];
    return "???";
}

// ─── 评级 ─────────────────────────────────────────
static const char* get_rating(int profit, int days) {
    if (days < 1) days = 1;
    int daily = profit / days;
    if (daily >= 200) return "S | 园艺大师！你就是花之王者~";
    if (daily >= 100) return "A | 优秀园丁，花园欣欣向荣！";
    if (daily >= 50)  return "B | 合格园丁，经营有道~";
    if (daily >= 0)   return "C | 普通园丁，勉强不亏";
    if (daily >= -50) return "D | 亏损园丁，花都哭了";
    return "E | 破产园丁...花园已荒芜";
}

// ─── 彩蛋 ─────────────────────────────────────────
void show_easter_egg(void) {
    printf("\n");
    printf("\t+==========================================+\n");
    printf("\t|                                          |\n");
    printf("\t|     Ciallo~(/_/;)/  ☆                    |\n");
    printf("\t|                                          |\n");
    printf("\t|  欢迎来到秘密花园！                       |\n");
    printf("\t|  你发现了隐藏的精灵~                      |\n");
    printf("\t|  花园的每一株植物都在为你歌唱呢           |\n");
    printf("\t|                                          |\n");
    printf("\t|           (\\__/)                         |\n");
    printf("\t|           (='.'=)  喵~                   |\n");
    printf("\t|           (\")_(\")                        |\n");
    printf("\t|                                          |\n");
    printf("\t+==========================================+\n");
}

// ─── 退出总结 ─────────────────────────────────────
void show_exit_summary(int start_coins, int end_coins, int days) {
    int profit = end_coins - start_coins;
    printf("\n");
    printf("\t+==========================================+\n");
    printf("\t|           ~ 经营总结报告 ~               |\n");
    printf("\t|------------------------------------------|\n");
    printf("\t|  经营天数: %-4d                           |\n", days);
    printf("\t|  初始资金: %-6d                          |\n", start_coins);
    printf("\t|  最终资金: %-6d                          |\n", end_coins);
    if (profit >= 0) {
        printf("\t|  财富变化: +%-5d                         |\n", profit);
    } else {
        printf("\t|  财富变化: %-6d                          |\n", profit);
    }
    if (days > 0)
        printf("\t|  日均利润: %-4d/天                       |\n", profit / days);
    printf("\t|------------------------------------------|\n");
    printf("\t|  最终评级: %-31s|\n", get_rating(profit, days));
    printf("\t+==========================================+\n");
    printf("\n\t感谢游玩！再见~\n");
}

// ─── 初始化 ───────────────────────────────────────
void init_plant(Plant *p, PlantType t) {
    switch (t) {
        case Carrot:
            strncpy(p->name, "萝卜", sizeof(p->name) - 1);
            p->water_need = 12;  p->nutrient_need = 4;
            p->total_growth_days = 5;  p->sell_price = 250;  break;
        case Potato:
            strncpy(p->name, "土豆", sizeof(p->name) - 1);
            p->water_need = 15;  p->nutrient_need = 5;
            p->total_growth_days = 6;  p->sell_price = 320;  break;
        case Wheat:
            strncpy(p->name, "小麦", sizeof(p->name) - 1);
            p->water_need = 10;  p->nutrient_need = 4;
            p->total_growth_days = 4;  p->sell_price = 200;  break;
        case Daisy:
            strncpy(p->name, "雏菊", sizeof(p->name) - 1);
            p->water_need = 18;  p->nutrient_need = 6;
            p->total_growth_days = 8;  p->sell_price = 550;  break;
        default: return;
    }
    p->type = t;
    p->growth_stage = 0;
    p->growth_value = 0;
    p->health = 100;
}

void init_player(Player *p) {
    p->coins     = 300;
    p->waters    = 50;
    p->nutrients = 20;
    p->day       = 1;
    p->plot_count = MPC;
    p->weather   = WEATHER_SUNNY;
    memset(p->seed_inventory, 0, sizeof(p->seed_inventory));
    // 初始赠送 2 颗随机种子
    for (int i = 0; i < 2; i++)
        p->seed_inventory[rand() % 4]++;
}

// ─── 种植 ─────────────────────────────────────────
int plant_seed(Player *pl, Plot *ps, int idx, PlantType t) {
    if (idx < 0 || idx >= pl->plot_count) {
        log_add("无效的地块！"); return 0;
    }
    if (!ps[idx].is_empty) {
        log_add("已有植物，无法种植！"); return 0;
    }
    if (t < Carrot || t > Daisy) {
        log_add("不存在的植物！"); return 0;
    }

    int price = g_price[t];
    if (pl->seed_inventory[t] > 0) {
        pl->seed_inventory[t]--;
    } else if (pl->coins >= price) {
        pl->coins -= price;
    } else {
        log_add("金币不足！需%d金，当前%d金", price, pl->coins);
        return 0;
    }

    ps[idx].is_empty = 0;
    ps[idx].planted_days = 0;
    ps[idx].soil_water   = 40;
    ps[idx].soil_nutrient = 20;
    init_plant(&ps[idx].plant, t);

    log_add("种下%s！剩余金币:%d", ps[idx].plant.name, pl->coins);
    return 1;
}

// ─── 浇水 ─────────────────────────────────────────
int water_plant(Player *pl, Plot *ps, int idx) {
    if (idx < 0 || idx >= pl->plot_count) {
        log_add("无效地块！"); return 0;
    }
    if (ps[idx].is_empty) {
        log_add("没有植物，无需浇水~"); return 0;
    }
    if (pl->waters < WU) {
        log_add("水资源不足！需%d，当前%d", WU, pl->waters);
        return 0;
    }
    pl->waters -= WU;
    ps[idx].soil_water += WA;
    if (ps[idx].soil_water > 100) ps[idx].soil_water = 100;

    log_add("给%s浇水完成。水+%d，剩余水:%d",
            ps[idx].plant.name, WA, pl->waters);
    return 1;
}

// ─── 施肥 ─────────────────────────────────────────
int fer_plant(Player *pl, Plot *ps, int idx) {
    if (idx < 0 || idx >= pl->plot_count) {
        log_add("无效地块！"); return 0;
    }
    if (ps[idx].is_empty) {
        log_add("没有植物，无需施肥！"); return 0;
    }
    if (pl->nutrients < NU) {
        log_add("肥料不足！需%d，当前%d", NU, pl->nutrients);
        return 0;
    }
    pl->nutrients -= NU;
    ps[idx].soil_nutrient += NA;
    if (ps[idx].soil_nutrient > 100) ps[idx].soil_nutrient = 100;

    log_add("给%s施肥完成。肥+%d，剩余肥:%d",
            ps[idx].plant.name, NA, pl->nutrients);
    return 1;
}

// ─── 收获 ─────────────────────────────────────────
int harvest(Player *pl, Plot *ps, int idx) {
    if (idx < 0 || idx >= pl->plot_count) {
        log_add("无效地块！"); return 0;
    }
    if (ps[idx].is_empty) {
        log_add("该地块没有植物！"); return 0;
    }

    Plant *p = &ps[idx].plant;
    if (p->health <= 0) {
        log_add("%s已枯萎，铲除无收益。", p->name);
        ps[idx].is_empty = 1;
        memset(p, 0, sizeof(Plant));
        return 0;
    }
    if (p->growth_value < 100 && p->growth_stage < 5) {
        log_add("%s未成熟(生长%d%%)，无法收获！", p->name, p->growth_value);
        return 0;
    }

    pl->coins += p->sell_price;
    log_add("收获%s！+%d金币，总计%d金币",
            p->name, p->sell_price, pl->coins);

    ps[idx].is_empty = 1;
    ps[idx].planted_days = 0;
    ps[idx].soil_water = 0;
    ps[idx].soil_nutrient = 0;
    memset(p, 0, sizeof(Plant));
    return 1;
}

// ─── 时间推进 + 天气 ──────────────────────────────
void pushtime(Player *pl, Plot *ps) {
    pl->day++;
    // 天气概率: 晴50% 阴30% 雨20%
    int r = rand() % 100;
    if (r < 50)       pl->weather = WEATHER_SUNNY;
    else if (r < 80)  pl->weather = WEATHER_CLOUDY;
    else              pl->weather = WEATHER_RAINY;

    log_add("第%d天 | %s", pl->day, weather_name(pl->weather));

    for (int i = 0; i < pl->plot_count; i++) {
        if (ps[i].is_empty) continue;
        Plant *p = &ps[i].plant;

        // 消耗资源
        ps[i].soil_water   -= p->water_need;
        ps[i].soil_nutrient -= p->nutrient_need;
        // 雨天补水
        if (pl->weather == WEATHER_RAINY) {
            ps[i].soil_water += 15;
            if (ps[i].soil_water > 100) ps[i].soil_water = 100;
        }
        // 边界夹紧
        if (ps[i].soil_water   < 0) ps[i].soil_water   = 0;
        if (ps[i].soil_nutrient < 0) ps[i].soil_nutrient = 0;

        // 生长值计算（含天气系数）
        int base = 100 / p->total_growth_days;
        float coeff = 1.0f;
        if (pl->weather == WEATHER_SUNNY)  coeff = 1.2f;
        if (pl->weather == WEATHER_RAINY)  coeff = 1.5f;
        if (pl->weather == WEATHER_CLOUDY) coeff = 0.8f;
        int bonus = (int)(base * coeff);
        if (bonus < 1) bonus = 1;

        if (ps[i].soil_water >= p->water_need &&
            ps[i].soil_nutrient >= p->nutrient_need) {
            p->growth_value += bonus;
            if (p->growth_value > 100) p->growth_value = 100;
            p->growth_stage = p->growth_value / 20;
            if (p->growth_stage > 5) p->growth_stage = 5;
            log_add("地块%d:%s[%s] +%d(%d%%)",
                    i, p->name, stage_name(p->growth_stage),
                    bonus, p->growth_value);
        } else {
            p->health -= 15;
            if (p->health < 0) p->health = 0;
            log_add("地块%d:%s 健康-%d(当前%d)",
                    i, p->name, 15, p->health);
            if (p->health <= 0) {
                log_add("地块%d:%s 已枯萎...", i, p->name);
                ps[i].is_empty = 1;
                memset(p, 0, sizeof(Plant));
            }
        }
    }
}

// ─── 地块输入 ─────────────────────────────────────
int get_plot(Player *pl) {
    int idx;
    while (1) {
        printf("\t选地块 0~%d: ", pl->plot_count - 1);
        if (scanf("%d", &idx) == 1 && idx >= 0 && idx < pl->plot_count) {
            flush_stdin();
            return idx;
        }
        if (feof(stdin)) return -1;
        printf("\t无效！请输入有效编号\n");
        flush_stdin();
    }
}

// ─── 商店 ─────────────────────────────────────────
static void shop_ui(Player *pl) {
    printf("\n");
    printf("\t+==========  ~ 花园商店 ~  ==========+\n");
    printf("\t|        金币: %-5d                 |\n", pl->coins);
    printf("\t|----------------------------------|\n");
    printf("\t| [种子]                            |\n");
    printf("\t| 1.萝卜种 %3d金 (库存:%d)          |\n", g_price[0], pl->seed_inventory[0]);
    printf("\t| 2.土豆种 %3d金 (库存:%d)          |\n", g_price[1], pl->seed_inventory[1]);
    printf("\t| 3.小麦种 %3d金 (库存:%d)          |\n", g_price[2], pl->seed_inventory[2]);
    printf("\t| 4.雏菊种 %3d金 (库存:%d)          |\n", g_price[3], pl->seed_inventory[3]);
    printf("\t|----------------------------------|\n");
    printf("\t| [道具]                            |\n");
    printf("\t| 5.水资源   %d金/单位              |\n", WP);
    printf("\t| 6.肥料     %d金/单位              |\n", NP);
    printf("\t|----------------------------------|\n");
    printf("\t| 7.离开商店                        |\n");
    printf("\t+==================================+\n");
    printf("\t选(1-7): ");
}

static int shop_buy_seed(Player *pl, PlantType t) {
    if (t < Carrot || t > Daisy) { log_add("非法种子"); return 0; }
    int price = g_price[t];
    if (pl->coins < price) {
        log_add("金币不足！需%d，当前%d", price, pl->coins); return 0;
    }
    pl->coins -= price;
    pl->seed_inventory[t]++;
    log_add("买下%s种子，库存%d，余%d金",
            g_cname[t], pl->seed_inventory[t], pl->coins);
    return 1;
}

static int shop_buy_water(Player *pl, int n) {
    if (n <= 0) { log_add("数量必须>0"); return 0; }
    int cost = n * WP;
    if (pl->coins < cost) {
        log_add("金币不足！需%d，当前%d", cost, pl->coins); return 0;
    }
    pl->coins  -= cost;
    pl->waters += n;
    log_add("购买%d水，余水%d，余%d金", n, pl->waters, pl->coins);
    return 1;
}

static int shop_buy_fert(Player *pl, int n) {
    if (n <= 0) { log_add("数量必须>0"); return 0; }
    int cost = n * NP;
    if (pl->coins < cost) {
        log_add("金币不足！需%d，当前%d", cost, pl->coins); return 0;
    }
    pl->coins     -= cost;
    pl->nutrients += n;
    log_add("购买%d肥，余肥%d，余%d金", n, pl->nutrients, pl->coins);
    return 1;
}

void shop(Player *pl) {
    int ch;
    while (1) {
        shop_ui(pl);
        if (scanf("%d", &ch) != 1) {
            if (feof(stdin)) return;
            flush_stdin(); continue;
        }
        flush_stdin();

        switch (ch) {
            case 1: shop_buy_seed(pl, Carrot); break;
            case 2: shop_buy_seed(pl, Potato); break;
            case 3: shop_buy_seed(pl, Wheat);  break;
            case 4: shop_buy_seed(pl, Daisy);  break;
            case 5: case 6: {
                int n;
                printf("\t数量: ");
                if (scanf("%d", &n) != 1) { flush_stdin(); break; }
                flush_stdin();
                if (ch == 5) shop_buy_water(pl, n);
                else         shop_buy_fert(pl, n);
                break;
            }
            case 7: return;
            default: log_add("无效选项");
        }
        printf("\t按回车继续...\n"); flush_stdin();
    }
}

// ─── 主界面 ───────────────────────────────────────
#define GW 60  // 界面总宽度

void show_main_ui(Player *pl, Plot *ps) {
    char tbuf[16]; get_time_str(tbuf, sizeof(tbuf));

    printf("\n");
    // ═══ 标题栏 ═══
    printf(" +");  for (int i = 0; i < GW - 2; i++) printf("=");  printf("+\n");

#ifdef _WIN32
    set_color(11, 0);
#endif
    printf(" |   植物花园模拟器");
#ifdef _WIN32
    reset_color();
#endif
    printf("  | 第 %-3d 天 | %s ", pl->day, tbuf);

#ifdef _WIN32
    if      (pl->weather == WEATHER_SUNNY)  set_color(14, 0);
    else if (pl->weather == WEATHER_RAINY)  set_color(9, 0);
    else                                    set_color(8, 0);
#endif
    printf("%s%s", weather_icon(pl->weather), weather_name(pl->weather));
#ifdef _WIN32
    reset_color();
#endif
    printf("  |\n");

    // ─── 资源栏 ───
    printf(" +");  for (int i = 0; i < GW - 2; i++) printf("-");  printf("+\n");
    printf(" | ");
#ifdef _WIN32
    set_color(14, 0);
#endif
    printf("金币:%-6d", pl->coins);
#ifdef _WIN32
    set_color(11, 0);
#endif
    printf(" 水:%-6d", pl->waters);
#ifdef _WIN32
    set_color(10, 0);
#endif
    printf(" 肥:%-6d", pl->nutrients);
#ifdef _WIN32
    reset_color();
#endif
    printf("     种子: ");
    for (int i = 0; i < 4; i++)
        printf("%s%d ", g_cname[i], pl->seed_inventory[i]);
    printf(" |\n");

    // ─── 地块区（2列宽盒，25列内容区）───
    printf(" +");  for (int i = 0; i < GW - 2; i++) printf("-");  printf("+\n");
    printf(" |");
    { int pad = (GW - 2 - 15) / 2; for (int i = 0; i < pad; i++) printf(" "); }
    printf("~~~ 花园一览 ~~~");
    { int pad = GW - 2 - 15 - (GW - 2 - 15) / 2;
      for (int i = 0; i < pad; i++) printf(" "); }
    printf("|\n");

    for (int row = 0; row < 2; row++) {
        // 地块标签
        printf(" |  ");
        for (int col = 0; col < 2; col++) {
            int idx = row * 2 + col;
            if (idx < pl->plot_count)
                printf("地块[%d]                       ", idx);
        }
        printf("|\n");

        // 盒子顶框
        printf(" |  ");
        for (int col = 0; col < 2; col++)
            if (row * 2 + col < pl->plot_count)
                printf("+-------------------------+ ");
        printf("|\n");

        // 内容行1: 名称 (name=4显示列, 20空格补齐→25)
        printf(" |  ");
        for (int col = 0; col < 2; col++) {
            int idx = row * 2 + col;
            if (idx >= pl->plot_count) continue;
            printf("|");
            if (ps[idx].is_empty)
                printf("         (空地)          ");
            else {
                printf(" %s", ps[idx].plant.name);
                for (int s = 0; s < 20; s++) printf(" ");
            }
            printf("| ");
        }
        printf("|\n");

        // 内容行2: 阶段+成长 (21+4空格=25)
        printf(" |  ");
        for (int col = 0; col < 2; col++) {
            int idx = row * 2 + col;
            if (idx >= pl->plot_count) continue;
            printf("|");
            if (ps[idx].is_empty) {
                printf("                         ");
            } else {
                Plant *p = &ps[idx].plant;
                int st = p->growth_stage;
                int bg = p->growth_value * 8 / 100;
#ifdef _WIN32
                if (st >= 5) set_color(10, 0);
                else if (p->health < 30) set_color(12, 0);
                else set_color(14, 0);
#endif
                printf(" %s%s", stage_icon(st), stage_name(st));
#ifdef _WIN32
                reset_color();
#endif
                printf("  %s[", "成长");
                for (int j = 0; j < 8; j++) printf("%c", j < bg ? '#' : '-');
                printf("]    ");
            }
            printf("| ");
        }
        printf("|\n");

        // 内容行3: 生命 (15+10空格=25)
        printf(" |  ");
        for (int col = 0; col < 2; col++) {
            int idx = row * 2 + col;
            if (idx >= pl->plot_count) continue;
            printf("|");
            if (ps[idx].is_empty) {
                printf("                         ");
            } else {
                int bh = ps[idx].plant.health * 8 / 100;
                printf(" %s[", "生命");
                for (int j = 0; j < 8; j++) {
                    if (j < bh) {
#ifdef _WIN32
                        if (bh <= 2) set_color(12, 0);
                        else if (bh <= 4) set_color(14, 0);
                        else set_color(10, 0);
#endif
                        printf("#");
#ifdef _WIN32
                        reset_color();
#endif
                    } else printf("-");
                }
                printf("]");
                for (int s = 0; s < 10; s++) printf(" ");
            }
            printf("| ");
        }
        printf("|\n");

        // 内容行4: 水/肥 (最宽14+11空格=25)
        printf(" |  ");
        for (int col = 0; col < 2; col++) {
            int idx = row * 2 + col;
            if (idx >= pl->plot_count) continue;
            printf("|");
            if (ps[idx].is_empty) {
                printf("                         ");
            } else {
                int w = ps[idx].soil_water;
                int f = ps[idx].soil_nutrient;
                printf(" 水%3d  肥%3d", w, f);
                for (int s = 0; s < 11; s++) printf(" ");
            }
            printf("| ");
        }
        printf("|\n");

        // 盒子底框
        printf(" |  ");
        for (int col = 0; col < 2; col++)
            if (row * 2 + col < pl->plot_count)
                printf("+-------------------------+ ");
        printf("|\n");
    }

    // ─── 操作日志 ───
    printf(" +");  for (int i = 0; i < GW - 2; i++) printf("-");  printf("+\n");
    printf(" | [操作日志]                                              |\n");
    for (int i = 0; i < LOG_MAX; i++) {
        const char *msg = log_get(i);
        printf(" | ");
        if (msg && msg[0]) {
            printf("> %-54s", msg);
        } else {
            printf("                                                        ");
        }
        printf("|\n");
    }

    // ─── 菜单 ───
    printf(" +");  for (int i = 0; i < GW - 2; i++) printf("-");  printf("+\n");
    printf(" | 1.种植  2.浇水  3.施肥  4.收获  5.下一天     |\n");
    printf(" | 6.保存  7.加载  8.商店  9.退出              |\n");
    printf(" +");  for (int i = 0; i < GW - 2; i++) printf("=");  printf("+\n");
    printf(" 操作(1-9): ");
}
#undef GW

// ─── 存档/读档 ────────────────────────────────────
int save_game(Player *pl, Plot *ps) {
    FILE *fp = fopen(SL, "wb");
    if (!fp) { log_add("存档失败！"); return 0; }
    int ver = SAVE_VERSION;
    fwrite(&ver,  sizeof(int),  1, fp);
    fwrite(pl,   sizeof(Player), 1, fp);
    fwrite(ps,   sizeof(Plot),   pl->plot_count, fp);
    fclose(fp);
    log_add("游戏保存成功！");
    return 1;
}

int load_game(Player *pl, Plot *ps) {
    FILE *fp = fopen(SL, "rb");
    if (!fp) { log_add("无存档文件"); return 0; }

    int ver;
    if (fread(&ver, sizeof(int), 1, fp) != 1 || ver != SAVE_VERSION) {
        fclose(fp);
        log_add("存档版本不兼容"); return 0;
    }
    if (fread(pl, sizeof(Player), 1, fp) != 1) {
        fclose(fp);
        log_add("玩家数据损坏"); return 0;
    }
    if (fread(ps, sizeof(Plot), pl->plot_count, fp) != (size_t)pl->plot_count) {
        fclose(fp);
        log_add("地块数据损坏"); return 0;
    }
    fclose(fp);
    log_add("游戏加载成功！");
    return 1;
}
