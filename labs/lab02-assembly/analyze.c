#include <stdio.h>

/*
 * Обчислює суму кроків від меншого числа до більшого:
 * спочатку порівнює параметри (if/else), визначає межі,
 * потім у циклі накопичує суму (lo + k) * 2 для кожного кроку.
 */
int range_score(int x, int y)
{
    int lo;
    int hi;

    if (x < y) {
        lo = x;
        hi = y;
    } else {
        lo = y;
        hi = x;
    }

    int total = 0;
    for (int k = lo; k <= hi; k++) {
        total = total + k * 2;
    }

    return total - lo;
}

int main(void)
{
    int r = range_score(7, 3);

    printf("range_score(7, 3) = %d\n", r);

    return 0;
}
