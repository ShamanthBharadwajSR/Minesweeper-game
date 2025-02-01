#ifndef GAME_H
#define GAME_H

void mns_GameInit();
void mns_GameShutdown();

void mns_GameUpdate(float delta);
void mns_GameRender();

void mns_GameOnClick(int x, int y, int left);

#endif