#ifndef MUSIC_H
#define MUSIC_H

void music_init();
void music_show();
void music_hide();
void music_update();
void music_next_track();
void music_prev_track();
void music_set_volume(int vol);
void music_toggle_play(); // Toggle play/pause icon

#endif // MUSIC_H
