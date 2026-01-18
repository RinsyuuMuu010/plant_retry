#include <stdio.h>
#include "pgsm.h"

int main(void) {
    Player player = {
        .coins = 1000,
        .waters = 50,
        .nutrients = 20,
        .day = 1,
        .plot_count = 4
    };
    Plot plots[4];

    for (int i=0;i< player.plot_count;i++) {
        plots[i].is_empty = 1;
        memset(&plots[i].plant,0,sizeof(Plant));
        plots[i].planted_days = 0;
        plots[i].soil_water = 0;
        plots[i].soil_nutrient = 0;
    }
//啥都没写
}