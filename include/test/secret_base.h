#ifndef GUARD_TEST_SECRET_BASE_H
#define GUARD_TEST_SECRET_BASE_H
#include "global.h"
const u8 *TestUtility_Interaction(s16 x, s16 y, u8 elevation, u8 direction);
bool32 TestDecor_Place(u8 decor, s16 x, s16 y);
void TestDecor_Remove(u8 slot);
void TestDecor_CopyTile(u16 tile, u8 *dest);
u8 TestStorage_MenuOption(u8 mode);
void TestStorage_OpenMenu(void);
u32 TestStorage_Input(u16 keys);
bool32 TestStorage_Launch(u8 option);
bool32 TestStorage_Return(void);
bool32 TestStorage_RejectFullBox(u8 box);
bool32 TestStorage_IsLastPartyMon(u8 slot);
void TestStorage_Transfer(u8 fromBox, u8 fromSlot, u8 toBox, u8 toSlot);
#endif
