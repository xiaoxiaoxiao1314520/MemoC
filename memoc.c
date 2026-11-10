/* ===========================================================
 * MemoC - 错题温习助手  v1.0
 * 仅使用标准库：stdio.h string.h stdlib.h ctype.h time.h limits.h
 * 编译：gcc -std=c99 -Wall -o memoc memoc.c
 * =========================================================== */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>

#define DATA_FILE  "memoc.dat"
#define TMP_FILE   "memoc.tmp"
#define BAK_FILE   "memoc.bak"
#define MAX_CODE   256
#define MAX_HINT   128
#define MAX_TAG    32
#define MAX_NOTE   500
#define CHECK_KEY  131

typedef struct Mistake {
    unsigned long fp;          /* 错题指纹 */
    char code[MAX_CODE];       /* 归一化后的代码片段 */
    char hint[MAX_HINT];       /* 订正要点 */
    char tag[MAX_TAG];         /* 知识点标签 */
    int  freq;                 /* 累计错误次数 F */
    int  mastery;              /* 掌握度 M (1~5) */
    long last;                 /* 上次复习日(距1970-01-01的天数) */
    long next;                 /* 下次复习日 */
    unsigned long sum;         /* 行校验和 */
    struct Mistake *next_ptr;  /* 单向链表指针 */
} Mistake;

static Mistake *head = NULL;
static const int GAP[5] = {1, 2, 4, 7, 15};
static long today(void) { return (long)(time(NULL) / 86400L); }

/* ---------- 归一化：去空白 + 转小写 ---------- */
static void normalize(const char *src, char *dst, size_t cap) {
    size_t k = 0;
    for (size_t i = 0; src[i] && k + 1 < cap; ++i) {
        unsigned char ch = (unsigned char)src[i];
        if (isspace(ch)) continue;
        dst[k++] = (char)tolower(ch);
    }
    dst[k] = '\0';
}

/* ---------- 错题指纹 ---------- */
static unsigned long fingerprint(const char *s) {
    unsigned long h = 5381UL;
    size_t len = strlen(s);
    for (size_t i = 0; s[i]; ++i) h = h * 33UL + (unsigned char)s[i];
    return h ^ (unsigned long)len;
}

/* ---------- 行校验和 ---------- */
static unsigned long checksum(const Mistake *m) {
    unsigned long v = m->fp ^ (unsigned long)m->freq ^ (unsigned long)m->mastery;
    v ^= (unsigned long)m->last ^ (unsigned long)m->next;
    for (size_t i = 0; m->code[i]; ++i) v = v * CHECK_KEY + (unsigned char)m->code[i];
    return v;
}

/* ---------- 复习分数 P = F*(D+1)*60/M ---------- */
static long priority(const Mistake *m, long now) {
    long d = now - m->last;
    if (d < 0) d = 0;
    long f = m->freq > 0 ? m->freq : 1;
    int  mm = m->mastery < 1 ? 1 : (m->mastery > 5 ? 5 : m->mastery);
    if (f > (LONG_MAX / 60L) / (d + 1)) return LONG_MAX;
    return f * (d + 1) * 60L / mm;
}

/* ---------- 安全输入：fgets 取代裸 scanf ---------- */
static void get_line(const char *prompt, char *buf, size_t cap) {
    if (prompt) printf("%s", prompt);
    if (fgets(buf, (int)cap, stdin) == NULL) { buf[0] = '\0'; return; }
    size_t n = strlen(buf);
    while (n && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';
}

/* ---------- 查找重复：指纹 + 前 64 字符二次判据 ---------- */
static Mistake *find_same(unsigned long fp, const char *code) {
    for (Mistake *p = head; p; p = p->next_ptr)
        if (p->fp == fp && strncmp(p->code, code, 64) == 0) return p;
    return NULL;
}

/* ---------- 新增错题 ---------- */
static void input_mistake(void) {
    char raw[MAX_CODE], code[MAX_CODE], hint[MAX_HINT], tag[MAX_TAG];
    get_line("  代码片段> ", raw, sizeof(raw));
    if (raw[0] == '\0') { puts("  [取消] 代码片段不能为空"); return; }
    normalize(raw, code, sizeof(code));
    unsigned long fp = fingerprint(code);
    Mistake *same = find_same(fp, code);
    if (same) {
        same->freq++;
        if (same->mastery > 1) same->mastery--;
        same->last = today();
        same->next = same->last + GAP[same->mastery - 1];
        printf("  [去重] 命中已有错题，累计次数 -> %d\n", same->freq);
        return;
    }
    Mistake *m = (Mistake *)malloc(sizeof(Mistake));
    if (!m) { puts("  [错误] 内存不足"); return; }
    memset(m, 0, sizeof(Mistake));
    m->fp = fp;
    strcpy(m->code, code);
    get_line("  订正要点> ", hint, sizeof(hint));
    get_line("  知识点标签> ", tag, sizeof(tag));
    strcpy(m->hint, hint[0] ? hint : "(未填写)");
    strcpy(m->tag,  tag[0]  ? tag  : "未分类");
    m->freq = 1; m->mastery = 1;
    m->last = today();
    m->next = m->last + GAP[0];
    m->sum = checksum(m);
    m->next_ptr = head; head = m;
    puts("  [成功] 已录入，明日进入复习队列");
}

/* ---------- 生成复习队列：到期记录按 P 降序冒泡排序 ---------- */
static int build_queue(Mistake **q, int cap, long now) {
    int n = 0;
    for (Mistake *p = head; p && n < cap; p = p->next_ptr)
        if (p->next <= now) q[n++] = p;
    for (int i = 0; i < n - 1; ++i)
        for (int j = 0; j < n - 1 - i; ++j)
            if (priority(q[j], now) < priority(q[j+1], now)) {
                Mistake *t = q[j]; q[j] = q[j+1]; q[j+1] = t;
            }
    return n;
}

/* ---------- 温习与自评 ---------- */
static void do_review(void) {
    Mistake *q[MAX_NOTE];
    long now = today();
    int n = build_queue(q, MAX_NOTE, now);
    if (n == 0) { puts("  今日无到期错题，可以休息了"); return; }
    printf("  今日待复习 %d 条\n", n);
    for (int i = 0; i < n; ++i) {
        Mistake *m = q[i];
        printf("\n  ---- (%d/%d) [%s]  P=%ld  错过%d次 ----\n",
               i+1, n, m->tag, priority(m, now), m->freq);
        printf("  代码: %s\n  [答案已隐藏] 回想后按回车查看 ...", m->code);
        getchar();
        printf("  订正: %s\n", m->hint);
        char s[32]; int score = 0;
        get_line("  自评 1~5> ", s, sizeof(s));
        if (sscanf(s, "%d", &score) != 1 || score < 1 || score > 5) {
            puts("  [提示] 输入非法，按 3 处理"); score = 3;
        }
        if (score <= 2 && m->mastery > 1) m->mastery--;
        else if (score >= 4 && m->mastery < 5) m->mastery++;
        m->last = now;
        m->next = now + GAP[m->mastery - 1];
        printf("  -> 掌握度 %d，下次复习 +%d 天\n", m->mastery, GAP[m->mastery-1]);
    }
}

/* ---------- 载入：逐行校验，损坏行跳过并告警 ---------- */
static int load_ledger(void) {
    FILE *f = fopen(DATA_FILE, "r");
    if (!f) return 0;
    char line[1024]; int ok = 0, bad = 0;
    while (fgets(line, sizeof(line), f)) {
        Mistake *m = (Mistake *)malloc(sizeof(Mistake));
        if (!m) break;
        memset(m, 0, sizeof(Mistake));
        unsigned long saved = 0;
        int got = sscanf(line, "%lu|%255[^|]|%127[^|]|%31[^|]|%d|%d|%ld|%ld|%lu",
                  &m->fp, m->code, m->hint, m->tag, &m->freq, &m->mastery,
                  &m->last, &m->next, &saved);
        if (got != 9 || saved != checksum(m)) { free(m); bad++; continue; }
        m->sum = saved; m->next_ptr = head; head = m; ok++;
    }
    fclose(f);
    if (bad) printf("  [警告] 跳过 %d 条校验失败的记录\n", bad);
    return ok;
}

/* ---------- 安全落盘：tmp -> bak -> dat，失败回滚 ---------- */
static int save_ledger(void) {
    FILE *f = fopen(TMP_FILE, "w");
    if (!f) { puts("  [错误] 无法创建临时文件"); return 0; }
    for (Mistake *p = head; p; p = p->next_ptr) {
        p->sum = checksum(p);
        fprintf(f, "%lu|%s|%s|%s|%d|%d|%ld|%ld|%lu\n",
                p->fp, p->code, p->hint, p->tag, p->freq, p->mastery,
                p->last, p->next, p->sum);
    }
    if (fflush(f) != 0 || fclose(f) != 0) { puts("  [错误] 写盘失败"); return 0; }
    remove(BAK_FILE);
    rename(DATA_FILE, BAK_FILE);
    if (rename(TMP_FILE, DATA_FILE) != 0) {
        rename(BAK_FILE, DATA_FILE);
        puts("  [错误] 改名失败，已回滚");
        return 0;
    }
    remove(BAK_FILE);
    puts("  [成功] 账本已安全保存");
    return 1;
}

static void list_all(void) {
    long now = today(); int i = 0;
    for (Mistake *p = head; p; p = p->next_ptr) {
        printf("  %2d. [%s] %-40.40s F=%d M=%d P=%ld\n",
               ++i, p->tag, p->code, p->freq, p->mastery, priority(p, now));
        if (p->hint[0]) printf("      订正: %s\n", p->hint);
    }
    if (!i) puts("  (账本为空)");
}

static void search_by_tag(void) {
    char key[MAX_TAG]; int hit = 0;
    get_line("  输入知识点标签关键字> ", key, sizeof(key));
    for (Mistake *p = head; p; p = p->next_ptr)
        if (strstr(p->tag, key)) {
            printf("  [%s] %s -> %s\n", p->tag, p->code, p->hint);
            hit++;
        }
    printf("  共命中 %d 条\n", hit);
}

static void show_stat(void) {
    int total = 0, due = 0; long sum_f = 0; long now = today();
    for (Mistake *p = head; p; p = p->next_ptr) {
        total++; sum_f += p->freq;
        if (p->next <= now) due++;
    }
    printf("  错题总数 %d | 累计错误次数 %ld | 今日到期 %d\n", total, sum_f, due);
}

static void free_all(void) {
    while (head) { Mistake *t = head; head = head->next_ptr; free(t); }
}

int main(void) {
    int n;
    system("chcp 65001 > nul");   /* 让中文提示在 Windows 控制台正常显示 */
    n = load_ledger();
    printf("==================================================\n");
    printf("  MemoC  错题温习助手   v1.0  已载入 %d 条\n", n);
    printf("==================================================\n");
    for (;;) {
        puts("  1.录入错题   2.今日温习   3.浏览全部");
        puts("  4.标签检索   5.统计信息   6.保存账本   0.退出");
        char s[32]; int op = -1;
        get_line("  请选择> ", s, sizeof(s));
        if (sscanf(s, "%d", &op) != 1) { puts("  [提示] 请输入数字"); continue; }
        if (op == 0) { save_ledger(); break; }
        switch (op) {
            case 1: input_mistake(); break;
            case 2: do_review();    break;
            case 3: list_all();     break;
            case 4: search_by_tag();break;
            case 5: show_stat();    break;
            case 6: save_ledger();  break;
            default: puts("  [提示] 无此选项");
        }
    }
    free_all();
    puts("  再见，记得明天再来温习。");
    return 0;
}
