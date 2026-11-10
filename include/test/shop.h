#ifndef GUARD_TEST_SHOP_H
#define GUARD_TEST_SHOP_H
#include "global.h"
#include "constants/items.h"
enum TestShopStage { TEST_SHOP_MESSAGE, TEST_SHOP_LIST, TEST_SHOP_QUANTITY, TEST_SHOP_RECEIPT };
void TestShop_Open(const u16 *stock, u16 selected);
void TestShop_Close(void);
void TestShop_Input(u16 keys);
u32 TestShop_Price(enum Item item);
u32 TestShop_Total(void);
u32 TestShop_Stage(void);
u32 TestShop_Count(void);
#endif
