/*
 * ui.h —— 终端界面层
 * 负责：UTF-8 / ANSI 初始化、宽度对齐（中文占两列）、面板绘制、输入读取。
 */

#ifndef PGSM_UI_H
#define PGSM_UI_H

#include "pgsm.h"

/* 初始化控制台：UTF-8 代码页 + 虚拟终端（颜色） */
void ui_init(void);

/* 启动横幅 */
void ui_title(void);

/* 清屏并绘制主面板 */
void ui_screen(const Player *player, const Plot *plots);

/* 商店面板 */
void ui_shop_screen(const Player *player);

/* 选种 / 种子货架；buying = 1 时显示“购买” */
void ui_seed_screen(const Player *player, int buying);

/* 带颜色的消息 */
void ui_info(const char *fmt, ...);
void ui_ok(const char *fmt, ...);
void ui_warn(const char *fmt, ...);
void ui_err(const char *fmt, ...);
void ui_note(const char *fmt, ...);

/* 读取 [lo, hi] 内的整数；返回 0 表示输入流结束（Ctrl+C / 管道读完） */
int ui_prompt(const char *prompt, int lo, int hi, int *out);

/* 等待回车 */
void ui_pause(void);

/* 计算字符串的终端显示宽度（跳过 ANSI 转义，中文按 2 列） */
int ui_width(const char *text);

#endif /* PGSM_UI_H */
