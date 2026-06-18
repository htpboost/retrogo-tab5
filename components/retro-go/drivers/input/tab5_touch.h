/*
 * Tab5 (ESP32-P4) 暫定タッチ入力ドライバ for retro-go  [RG_GAMEPAD_TOUCH_TAB5]
 *
 * GT911 タッチ(ILI9881C世代)を読み、画面の領域を retro-go のキーにマップする。
 * USB ゲームパッドが届くまでのランチャー操作用。パネル native は 720(W) x 1280(H) 縦。
 *
 * ゾーン(縦画面 720x1280):
 *   上端(y<256)      → UP    (一覧を上へ)
 *   下端(y>1024)     → DOWN  (一覧を下へ)
 *   左端(x<150)中央帯 → SELECT(前のタブへ)
 *   右端(x>570)中央帯 → START (次のタブへ)
 *   中央              → A     (決定/ゲーム起動)
 * ※ ゲーム中も同じゾーン。本格的な操作は USB パッドで。
 */
#pragma once
#include "esp_lcd_touch.h"
#include "bsp/esp-bsp.h"
#include "bsp/touch.h"

static esp_lcd_touch_handle_t s_tab5_touch = NULL;
static bool s_tab5_touch_tried = false;

static uint32_t tab5_touch_read_keys(void)
{
    if (!s_tab5_touch_tried)
    {
        s_tab5_touch_tried = true;
        // I2C は表示初期化で既に init 済み。GT911/ST7123 を自動選択して生成。
        if (bsp_touch_new(NULL, &s_tab5_touch) != ESP_OK)
            s_tab5_touch = NULL;
    }
    if (!s_tab5_touch)
        return 0;

    uint16_t tx[1] = {0}, ty[1] = {0}, ts[1] = {0};
    uint8_t cnt = 0;
    esp_lcd_touch_read_data(s_tab5_touch);
    if (!esp_lcd_touch_get_coordinates(s_tab5_touch, tx, ty, ts, &cnt, 1) || cnt == 0)
        return 0;

    const int W = 720, H = 1280;
    int x = tx[0], y = ty[0];

    if (y < H / 5)        return RG_KEY_UP;     // 上端
    if (y > H * 4 / 5)    return RG_KEY_DOWN;   // 下端
    if (x < 150)          return RG_KEY_SELECT; // 左端 → 前タブ
    if (x > W - 150)      return RG_KEY_START;  // 右端 → 次タブ
    return RG_KEY_A;                            // 中央 → 決定/起動
}
