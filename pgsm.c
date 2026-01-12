//
// Created by Akimei1223 on 2026/1/10.
//
#include "pgsm.h"

const int Seed_prices[]={30,40,30,40,60}; //价位表
void initial_plant(Plant *plant,PlantType type) {
    switch (type) {
        case Carrot:
            strcpy(plant->name, "Carrot");
            plant->water_need = 20;
            plant->nutrient_need = 5;
            plant->total_growth_days = 6;
            plant->sell_price = 30;
            break;

        case Potato:
            strcpy(plant->name, "Potato");
            plant->water_need = 18;
            plant->nutrient_need = 6;
            plant->total_growth_days = 9;
            plant->sell_price = 40;
            break;
    }
}