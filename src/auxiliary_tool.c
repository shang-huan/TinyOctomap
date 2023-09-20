#include "octoMap.h"
#include "octoTree.h"
#include "auxiliary_tool.h"

void calPoint(coordinate_t* Start_Point,direction_t direction,int length,coordinate_t* End_Point)
{
    switch (direction) {
    case UP:
        End_Point->x = Start_Point->x;
        End_Point->y = Start_Point->y;
        End_Point->z = Start_Point->z + length;
        break;
    case DOWN:
        End_Point->x = Start_Point->x;
        End_Point->y = Start_Point->y;
        End_Point->z = Start_Point->z - length;
        break;
    case LEFT:
        End_Point->x = Start_Point->x;
        End_Point->y = Start_Point->y - length;
        End_Point->z = Start_Point->z;
        break;
    case RIGHT:
        End_Point->x = Start_Point->x;
        End_Point->y = Start_Point->y + length;
        End_Point->z = Start_Point->z;
        break;
    case FRONT:
        End_Point->x = Start_Point->x + length;
        End_Point->y = Start_Point->y;
        End_Point->z = Start_Point->z;
        break;
    case BACK:
        End_Point->x = Start_Point->x - length;
        End_Point->y = Start_Point->y;
        End_Point->z = Start_Point->z;
        break;
    }
}