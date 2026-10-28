/*
 * ui.c —— 终端界面层实现
 *
 * 三个坑：
 *   1. Windows 控制台默认 GBK，直接输出 UTF-8 会变乱码 → ui_init 切到 65001；
 *   2. 中文占两列，用 strlen 算对齐会错位 → ui_width 按码点算宽度；
 *   3. 老版控制台不认 ANSI 颜色 → 探测失败就整屏降级为无色，不影响可读性。
 */

#include "ui.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

/* 面板内容宽度（不含左右边框） */
#define PANEL 64

/* ------------------------------------------------------------------ */
/* 颜色                                                                */
/* ------------------------------------------------------------------ */
static int g_color = 0;

/* 颜色始终拼进字符串，输出时由 out() 决定是否丢掉转义序列：
   这样宏就能和相邻字面量拼接，宽度计算也不用区分有没有颜色。 */
#define RESET   "\x1b[0m"
#define BOLD    "\x1b[1m"
#define GREY    "\x1b[90m"
#define RED     "\x1b[91m"
#define GREEN   "\x1b[92m"
#define YELLOW  "\x1b[93m"
#define BLUE    "\x1b[96m"
#define MAGENTA "\x1b[95m"

/* ------------------------------------------------------------------ */
/* 宽度                                                                */
/* ------------------------------------------------------------------ */
static int cp_width(unsigned cp)
{
    if (cp == 0) return 0;
    if (cp < 0x20) return 0;
    if (cp < 0x7f) return 1;
    if (cp < 0xa0) return 0;
    if ((cp >= 0x1100 && cp <= 0x115f) ||
        (cp >= 0x2e80 && cp <= 0x303e) ||
        (cp >= 0x3041 && cp <= 0x33ff) ||
        (cp >= 0x3400 && cp <= 0x4dbf) ||
        (cp >= 0x4e00 && cp <= 0x9fff) ||
        (cp >= 0xa000 && cp <= 0xa4cf) ||
        (cp >= 0xac00 && cp <= 0xd7a3) ||
        (cp >= 0xf900 && cp <= 0xfaff) ||
        (cp >= 0xfe30 && cp <= 0xfe4f) ||
        (cp >= 0xff00 && cp <= 0xff60) ||
        (cp >= 0xffe0 && cp <= 0xffe6))
        return 2;
    return 1;
}

int ui_width(const char *text)
{
    int w = 0;
    const unsigned char *s = (const unsigned char *)text;

    while (*s) {
        if (*s == 0x1b) {                 /* ESC 序列整体跳过 */
            s++;
            if (*s == '[') {
                s++;
                while (*s && !(*s >= '@' && *s <= '~')) s++;
                if (*s) s++;
            }
            continue;
        }
        if (*s < 0x80) { w += 1; s++; continue; }

        unsigned cp = 0;
        int extra = 0;
        if ((*s & 0xe0) == 0xc0)      { cp = *s & 0x1fu; extra = 1; }
        else if ((*s & 0xf0) == 0xe0) { cp = *s & 0x0fu; extra = 2; }
        else if ((*s & 0xf8) == 0xf0) { cp = *s & 0x07u; extra = 3; }
        else { s++; continue; }

        s++;
        for (int i = 0; i < extra && *s; i++, s++)
            cp = (cp << 6) | (*s & 0x3fu);
        w += cp_width(cp);
    }
    return w;
}

/* ------------------------------------------------------------------ */
/* 输出原语                                                            */
/* ------------------------------------------------------------------ */
/* 统一出口：有颜色能力就原样输出，否则剥掉 ESC 序列 */
static void out(const char *s)
{
    if (g_color) {
        fputs(s, stdout);
        return;
    }

    while (*s) {
        if ((unsigned char)*s == 0x1b) {
            s++;
            if (*s == '[') {
                s++;
                while (*s && !(*s >= '@' && *s <= '~')) s++;
                if (*s) s++;
            }
            continue;
        }
        fputc((unsigned char)*s, stdout);
        s++;
    }
}

static void reps(const char *s, int n)
{
    for (int i = 0; i < n; i++) out(s);
}

static void hline(const char *left, const char *right)
{
    out(GREY);
    out(left);
    reps("─", PANEL);
    out(right);
    out(RESET);
    out("\n");
}

/* 一行文本，自动补足到 PANEL 列 */
static void row(const char *fmt, ...)
{
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    int pad = PANEL - ui_width(buf);
    if (pad < 0) pad = 0;

    out(GREY "│" RESET);
    out(buf);
    reps(" ", pad);
    out(GREY "│" RESET "\n");
}

/* 顶栏：左标题 + 右信息，中间用 ─ 填满 */
static void topbar(const char *left, const char *right)
{
    int lw = ui_width(left), rw = ui_width(right);
    int fill = PANEL - 1 - lw - rw;
    if (fill < 0) fill = 0;

    out(GREY "┌─" RESET);
    out(BOLD); out(left); out(RESET);
    out(GREY); reps("─", fill); out(RESET);
    out(right);
    out(GREY "┐" RESET "\n");
}

/* 在缓冲区里追加文本，并按显示宽度补空格 */
static void put_pad(char *buf, size_t cap, const char *text, int cols)
{
    size_t used = strlen(buf);
    if (used + 1 < cap) snprintf(buf + used, cap - used, "%s", text);
    int pad = cols - ui_width(text);
    used = strlen(buf);
    for (int i = 0; i < pad && used + 1 < cap; i++) buf[used++] = ' ';
    buf[used] = '\0';
}

static void putf(char *buf, size_t cap, const char *fmt, ...)
{
    size_t used = strlen(buf);
    if (used + 1 >= cap) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf + used, cap - used, fmt, ap);
    va_end(ap);
}

/* 生长进度条：█ 实心 / ░ 空心 */
static void put_bar(char *buf, size_t cap, int value, int maxv, int cells)
{
    int filled = maxv > 0 ? (int)((long)value * cells / maxv) : 0;
    if (filled > cells) filled = cells;
    if (filled < 0) filled = 0;

    putf(buf, cap, "%s", value >= maxv ? YELLOW : GREEN);
    for (int i = 0; i < filled; i++) putf(buf, cap, "█");
    putf(buf, cap, GREY);
    for (int i = filled; i < cells; i++) putf(buf, cap, "░");
    putf(buf, cap, RESET);
}

/* ------------------------------------------------------------------ */
/* 初始化与标题                                                        */
/* ------------------------------------------------------------------ */
void ui_init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
        if (SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            g_color = 1;
    }
#else
    g_color = 1;
#endif
}

void ui_title(void)
{
    out("\n");
    out("  " BLUE "╔════════════════════════════════════════════════╗" RESET "\n");
    out("  " BLUE "║" RESET "  " BOLD "像素花园" RESET "  " GREY "PIXEL GARDEN" RESET
        "                  " GREY "v" PGSM_VERSION RESET "  " BLUE "║" RESET "\n");
    out("  " BLUE "╚════════════════════════════════════════════════╝" RESET "\n");
    out("     " GREY "种下去，浇点水，等一场雨，然后收获。" RESET "\n\n");
}

/* ------------------------------------------------------------------ */
/* 主面板                                                              */
/* ------------------------------------------------------------------ */
static const char *status_of(const Plot *plot)
{
    const PlantInfo *info = plant_info(plot->plant.type);

    if (plot->plant.growth >= PM_FULL) return "★可收获";
    if (plot->plant.pest) return "虫害";
    if (plot->plant.health < 40) return "虚弱";
    if (plot->soil_water < info->water_need) return "缺水";
    if (plot->soil_nutrient < info->nutrient_need) return "缺肥";
    return "";
}

static const char *status_color(const char *status)
{
    if (strcmp(status, "★可收获") == 0) return YELLOW;
    if (status[0] == '\0') return "";
    return RED;
}

void ui_screen(const Player *player, const Plot *plots)
{
    char buf[2048];
    Season season = season_of_day(player->day);
    int season_left = DAYS_PER_SEASON - ((player->day - 1) % DAYS_PER_SEASON);
    int done = 0;

    for (unsigned m = 0; m < 5; m++)
        if (player->achievements & (1u << m)) done++;

    if (g_color) printf("\x1b[2J\x1b[H");

    {
        char left[256];
        char right[128];
        snprintf(left, sizeof left, " 像素花园 · 第 %d 天 · %s · %s ",
                 player->day, season_name(season), weather_name((Weather)player->weather));
        snprintf(right, sizeof right, GREY " 本季剩 %d 天 " RESET, season_left);
        topbar(left, right);
    }

    buf[0] = '\0';
    putf(buf, sizeof buf, " " BOLD "金币" RESET " %-6d " BOLD "水" RESET " %-5d "
                          BOLD "肥" RESET " %-5d ", player->coins, player->waters, player->nutrients);
    putf(buf, sizeof buf, BOLD "收获" RESET " %-4d " BOLD "收入" RESET " %-5d",
         player->total_harvests, player->total_earned);
    row("%s", buf);

    buf[0] = '\0';
    putf(buf, sizeof buf, " " GREY "地块 %d/%d   种子 %d   成就 %d/5   ",
         player->plot_count, MAX_PLOTS, count_seeds(player), done);
    if (player->achievements & ACH_FIRST_HARVEST) putf(buf, sizeof buf, "初收 ");
    if (player->achievements & ACH_HARVEST_10)    putf(buf, sizeof buf, "老农 ");
    if (player->achievements & ACH_RICH_1000)     putf(buf, sizeof buf, "富翁 ");
    if (player->achievements & ACH_FULL_GARDEN)   putf(buf, sizeof buf, "满园 ");
    if (player->achievements & ACH_COLLECTOR)     putf(buf, sizeof buf, "收藏家 ");
    putf(buf, sizeof buf, "%s", RESET);
    row("%s", buf);

    hline("├", "┤");

    for (int i = 0; i < player->plot_count; i++) {
        const Plot *plot = &plots[i];
        buf[0] = '\0';

        if (plot->empty) {
            putf(buf, sizeof buf, " " YELLOW "%d" RESET "  %s", i + 1, GREY "· 空地 · 可以播种" RESET);
            row("%s", buf);
            continue;
        }

        const PlantInfo *info = plant_info(plot->plant.type);
        const char *status = status_of(plot);

        putf(buf, sizeof buf, " %s%d" RESET "  ", BOLD, i + 1);
        put_pad(buf, sizeof buf, info->name, 10);

        put_bar(buf, sizeof buf, plot->plant.growth, PM_FULL, 14);
        putf(buf, sizeof buf, " %3d%%", plot->plant.growth / 10);

        {
            char health[32];
            snprintf(health, sizeof health, "健%d", plot->plant.health);
            putf(buf, sizeof buf, " ");
            if (plot->plant.health < 40) {
                putf(buf, sizeof buf, RED);
                put_pad(buf, sizeof buf, health, 5);
                putf(buf, sizeof buf, RESET);
            } else {
                put_pad(buf, sizeof buf, health, 5);
            }
        }

        {
            char soil[48];
            snprintf(soil, sizeof soil, "水%d 肥%d", plot->soil_water, plot->soil_nutrient);
            putf(buf, sizeof buf, " ");
            put_pad(buf, sizeof buf, soil, 11);
        }

        if (status[0]) putf(buf, sizeof buf, "%s%s" RESET, status_color(status), status);

        row("%s", buf);
    }

    hline("├", "┤");
    row("  " BOLD "1" RESET " 播种   " BOLD "2" RESET " 浇水   " BOLD "3" RESET " 施肥   "
        BOLD "4" RESET " 除虫   " BOLD "5" RESET " 收获");
    row("  " BOLD "6" RESET " 商店   " BOLD "7" RESET " 下一天 " BOLD "8" RESET " 存档   "
        BOLD "9" RESET " 读档   " BOLD "0" RESET " 退出");
    hline("└", "┘");
}

/* ------------------------------------------------------------------ */
/* 商店与选种                                                          */
/* ------------------------------------------------------------------ */
void ui_shop_screen(const Player *player)
{
    char buf[512];

    printf("\n");
    topbar(" 商店 · SHOP ", "");

    snprintf(buf, sizeof buf, " " BOLD "金币" RESET " %d", player->coins);
    row("%s", buf);
    hline("├", "┤");

    row("  " BOLD "1" RESET "  水 x10        " GREY "%d 金币" RESET "      现有 %d 单位",
        10 * WATER_PRICE, player->waters);
    row("  " BOLD "2" RESET "  肥料 x10      " GREY "%d 金币" RESET "      现有 %d 单位",
        10 * FERT_PRICE, player->nutrients);
    row("  " BOLD "3" RESET "  种子货架      " GREY "按颗购买" RESET "      背包 %d 颗",
        count_seeds(player));

    if (player->plot_count < MAX_PLOTS)
        row("  " BOLD "4" RESET "  扩建地块      " GREY "%d 金币" RESET "      当前 %d/%d 块",
            PLOT_PRICE, player->plot_count, MAX_PLOTS);
    else
        row("  " BOLD "4" RESET "  扩建地块      " GREY "已达上限 %d 块" RESET, MAX_PLOTS);

    row("  " BOLD "0" RESET "  返回");
    hline("└", "┘");
}

void ui_seed_screen(const Player *player, int buying)
{
    char buf[1024];

    printf("\n");
    if (buying) topbar(" 种子货架 · SEEDS ", "");
    else        topbar(" 播种 · SEEDS ", "");

    row(" " GREY "序号  植物        种子   成熟   售价   净利   库存" RESET);
    hline("├", "┤");

    for (int t = 0; t < PLANT_COUNT; t++) {
        const PlantInfo *info = plant_info((PlantType)t);
        int unlocked = plant_unlocked(player, (PlantType)t);

        buf[0] = '\0';
        putf(buf, sizeof buf, "  %s%d" RESET "    ", BOLD, t + 1);
        put_pad(buf, sizeof buf, info->name, 12);
        putf(buf, sizeof buf, "%3d   %2d天   %4d   %+4d   ",
             info->seed_price, info->growth_days, info->sell_price,
             info->sell_price - info->seed_price);

        if (!unlocked) {
            putf(buf, sizeof buf, RED "锁 需收获%d次" RESET, info->unlock_harvests);
        } else if (player->seeds[t] <= 0) {
            putf(buf, sizeof buf, GREY "0" RESET);
        } else {
            putf(buf, sizeof buf, GREEN "%d" RESET, player->seeds[t]);
        }
        row("%s", buf);
    }

    hline("└", "┘");
    if (!buying) ui_note("选一种已经买好的种子；没有的话按 6 去商店");
}

/* ------------------------------------------------------------------ */
/* 消息                                                                */
/* ------------------------------------------------------------------ */
static void vmsg(const char *color, const char *fmt, va_list ap)
{
    char buf[1024];
    vsnprintf(buf, sizeof buf, fmt, ap);
    out("  ");
    out(color);
    out(buf);
    out(RESET);
    out("\n");
}

void ui_info(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vmsg(BLUE, fmt, ap);   va_end(ap); }
void ui_ok(const char *fmt, ...)   { va_list ap; va_start(ap, fmt); vmsg(GREEN, fmt, ap);  va_end(ap); }
void ui_warn(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vmsg(YELLOW, fmt, ap); va_end(ap); }
void ui_err(const char *fmt, ...)  { va_list ap; va_start(ap, fmt); vmsg(RED, fmt, ap);    va_end(ap); }
void ui_note(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vmsg(GREY, fmt, ap);   va_end(ap); }

/* ------------------------------------------------------------------ */
/* 输入                                                                */
/* ------------------------------------------------------------------ */
static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) *--end = '\0';
    return s;
}

int ui_prompt(const char *prompt, int lo, int hi, int *value)
{
    char line[128];

    for (;;) {
        printf("  ");
        out(BOLD);
        out(prompt);
        out(RESET);
        printf(" > ");
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) {
            printf("\n");
            return 0;                       /* 输入流结束 */
        }

        char *text = trim(line);
        if (*text == '\0') continue;

        char *end = NULL;
        long v = strtol(text, &end, 10);
        if (end == text || *trim(end) != '\0' || v < lo || v > hi) {
            printf("  请输入 %d ~ %d 之间的数字\n", lo, hi);
            continue;
        }
        *value = (int)v;
        return 1;
    }
}

void ui_pause(void)
{
    char line[128];
    printf("  ");
    out(GREY);
    out("按回车继续…");
    out(RESET);
    fflush(stdout);
    (void)!fgets(line, sizeof line, stdin);
}
