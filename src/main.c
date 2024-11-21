#include <stdlib.h>
#include <locale.h>
#include <ncurses.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define RING_L 80
#define RING_U 176
#define STR_SZ 32

typedef struct Point {
    int32_t x;
    int32_t z;
} Point;

void nc_init() {
    initscr();
    cbreak();
    curs_set(0);
    noecho();
}

void gen_sh(Point *coords) {
    int32_t dist = RING_L + random() % (RING_U - RING_L);
    double angle = (random() % 3599) / 10.0;
    angle = angle * M_PI / 180;

    coords->x = (int) (dist * sin(angle)) * 16 + 8;
    coords->z = (int) (dist * cos(angle)) * 16 + 8;
}

void gen_blind(Point *coords) {
    int32_t dist = random() % RING_U;
    double angle = (random() % 3599) / 100.0;
    angle = angle * M_PI / 180;

    coords->x = (int) (dist * sin(angle)) * 16;
    coords->z = (int) (dist * cos(angle)) * 16;
}

int32_t distance(Point *p1, Point *p2) {
    return sqrt(pow(p1->x - p2->x, 2) + pow(p1->z - p2->z, 2));
}

double angle_to_line(double angle) {
    if (angle < 0)
        angle = angle + 180;
    if (angle > 90)
        angle = angle - 90;
    if (angle > 45)
        angle = 90 - angle;
    return 8 * tan(angle * M_PI / 180);
}

double calc_first_angle(Point *sh, Point *blind) {
    // in mc 0° is pos z and -90° is pos x
    int x_diff = -sh->x + blind->x;
    int z_diff = sh->z - blind->z;
    return atan2(x_diff, z_diff) * 180 / M_PI;
}

int calc_x_trav(double angle, double line) {
    // in mc 0° is pos z and -90° is pos x
    if (45 <= angle && angle <= 135)
        return -16;
    if (-45 >= angle && angle >= -135)
        return 16;
    if (fabs(angle) >= 0)
        return round(-line * 2);
    return round(line * 2);
}

int calc_z_trav(double angle, double line) {
    // in mc 0° is pos z and -90° is pos x
    if (fabs(angle) > 135)
        return -16;
    if (fabs(angle) < 45)
        return 16;
    if (fabs(angle) > 90)
        return round(-line * 2);
    return round(line * 2);
}

double calc_second_angle(Point *sh, Point *blind) {
    double fangle = calc_first_angle(sh, blind);
    double line = angle_to_line(fangle);

    double trav_angle = fangle + 90;
    if (trav_angle > 180)
        trav_angle -= 360;

    int x_trav = calc_x_trav(trav_angle, line);
    int z_trav = calc_z_trav(trav_angle, line);

    Point second = {
        .x = blind->x + x_trav,
        .z = blind->z + z_trav
    };

    return calc_first_angle(sh, &second);
}

int chunk_x(Point *blind, double angle) {
    double trav_angle = angle + 90;
    if (trav_angle > 180)
        trav_angle -= 360;

    int mid_x = blind->x + (trav_angle > 0 ? -8 : 8);

    return (int) round((mid_x / 16.0) - 0.5);
}

int chunk_z(Point *blind, double angle) {
    double trav_angle = angle + 90;
    if (trav_angle > 180)
        trav_angle -= 360;

    int mid_z = blind->z + (fabs(trav_angle) > 90 ? -8 : 8);

    return (int) round((mid_z / 16.0) - 0.5);
}

void print_mid(char *str, int32_t y_off) {
    mvprintw(LINES / 2 + y_off, COLS / 2 - (strlen(str) / 2), "%s", str);
}

int8_t test_axis() {
    clear();
    Point sh = { 0 };
    Point blind = { 0 };
    gen_sh(&sh);
    do gen_blind(&blind); while (distance(&sh, &blind) > 3200);

    // First measurement
    double fangle = calc_first_angle(&sh, &blind);
    char fangle_str[STR_SZ] = { 0 };
    char line_str[STR_SZ] = { 0 };
    sprintf(fangle_str, "1st angle %.1f", fangle);
    sprintf(line_str, "line %.2f", angle_to_line(fangle));

    print_mid(fangle_str, 0);
    print_mid(line_str, 1);

    while (getch() == KEY_RESIZE);
    clear();

    // Second measurement
    double sangle = calc_second_angle(&sh, &blind);
    char sangle_str[STR_SZ] = { 0 };
    sprintf(sangle_str, "2nd angle %.1f", sangle);

    print_mid(sangle_str, 0);

    while (getch() == KEY_RESIZE);
    clear();

    // Chunk coords and major/minior
    char chunk_str[STR_SZ] = { 0 };
    char major_str[STR_SZ] = { 0 };
    char dir_str[STR_SZ] = { 0 };
    sprintf(chunk_str, "chunk %d %d", chunk_x(&blind, fangle), chunk_z(&blind, fangle));
    sprintf(major_str, "major %s", (45 <= fabs(fangle) && fabs(fangle) <= 135) ? "x" : "z");
    sprintf(dir_str, "%s %s", fangle < 0 ? "+" : "-", fabs(fangle) <= 90 ? "+" : "-");

    print_mid(chunk_str, -1);
    print_mid(major_str, 0);
    print_mid(dir_str, 1);

    while (getch() == KEY_RESIZE);
    clear();

    // Check
    char sh_str[STR_SZ] = { 0 };
    sprintf(sh_str, "sh %d %d", (int) round(sh.x / 8.0), (int) round(sh.z / 8.0));
    print_mid(fangle_str, -4);
    print_mid(line_str, -3);
    print_mid(sangle_str, -2);
    print_mid(chunk_str, -1);
    print_mid(major_str, 0);
    print_mid(dir_str, 1);

    print_mid(sh_str, 2);

    int ch = getch();
    while (ch == KEY_RESIZE) ch = getch();

    return !(ch == 'q' || ch == 27);
}

int main(void) {
    setlocale(LC_ALL, "");
    srandom(time(NULL));
    nc_init();

    while (test_axis());

    endwin();

    return 0;
}
