#ifndef COMPASS_H
#define COMPASS_H

void compass_init();
void compass_show();
void compass_hide();
void compass_update(float heading);
void compass_update_speed(float speedKmh);

#endif
