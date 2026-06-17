/*
 * Tab5 (ESP32-P4) MIPI-DSI display driver for retro-go  [RG_SCREEN_DRIVER == 2]
 *
 * 公式 m5stack_tab5 BSP の bsp_display_new_auto() で DSI バス + DPI パネルを立ち上げ、
 * esp_lcd_panel_draw_bitmap() でパネルのフレームバッファへ書き込む。
 * retro-go の「lcd_set_window(矩形) → lcd_get_buffer/lcd_send_buffer で行順次ストリーム」
 * モデルを draw_bitmap の矩形転送へ変換する。
 *
 * DPI の draw_bitmap は非同期(DMAコピー)。完了前に次を呼ぶと
 * "previous draw operation is not finished" となり描画が落ちるため、
 * on_color_trans_done コールバック + セマフォで「1転送ずつ完了を待つ」直列化を行う。
 * draw_bitmap は内部でキャッシュのライトバックも行うので、PSRAM FB でも正しく見える。
 *
 * Phase1: パネル native の 720(W) x 1280(H) 縦のまま描画(回転・拡大なし)。
 *         横向き(1280x720)化は後続フェーズでソフト回転として実装。
 * 注意: 色が化ける場合は retro-go と BSP の RGB565 バイト順差(endian)を疑う。
 */
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_mipi_dsi.h>
#include "bsp/esp-bsp.h"

#if defined(RG_SCREEN_ROTATE) && RG_SCREEN_ROTATE != 0
#warning "tab5_dsi: RG_SCREEN_ROTATE は未対応(回転は別途実装予定)。0 推奨。"
#endif

// BSP(m5stack_tab5.c)に追加した非LVGLの自動検出ラッパー
extern esp_err_t bsp_display_new_auto(const bsp_display_config_t *config, bsp_lcd_handles_t *ret_handles);

static esp_lcd_panel_handle_t s_panel = NULL;
static SemaphoreHandle_t s_trans_done = NULL;

// retro-go 窓ストリーム状態(現在の描画矩形と、書き込み済み行数)
static int s_win_x, s_win_y, s_win_w, s_win_h, s_win_line;

// 描画バッファプール
#define TAB5_BUFFER_COUNT (4)
static QueueHandle_t s_buffers;

static IRAM_ATTR bool tab5_on_trans_done(esp_lcd_panel_handle_t panel,
                                         esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    BaseType_t hp_task_woken = pdFALSE;
    xSemaphoreGiveFromISR(s_trans_done, &hp_task_woken);
    return hp_task_woken == pdTRUE;
}

static inline uint16_t *lcd_get_buffer(size_t length)
{
    uint16_t *buf;
    if (xQueueReceive(s_buffers, &buf, pdMS_TO_TICKS(2500)) != pdTRUE)
        RG_PANIC("display: no buffer");
    return buf;
}

static void lcd_set_window(int left, int top, int width, int height)
{
    if (left < 0 || top < 0 ||
        left + width > display.screen.real_width || top + height > display.screen.real_height)
        RG_LOGW("Bad lcd window (x=%d y=%d w=%d h=%d)\n", left, top, width, height);
    s_win_x = left;
    s_win_y = top;
    s_win_w = width;
    s_win_h = height;
    s_win_line = 0;
}

static inline void lcd_send_buffer(uint16_t *buffer, size_t length)
{
    if (length == 0) // 送信無し: バッファ返却のみ
    {
        xQueueSend(s_buffers, &buffer, portMAX_DELAY);
        return;
    }
    // retro-go GUI は RGB565 をビッグエンディアン(SPIパネル向けにバイト入替)で合成するが、
    // MIPI-DSI DPI フレームバッファはネイティブ(LE)。送出前にバイトスワップして色を合わせる。
    for (size_t i = 0; i < length; i++)
        buffer[i] = (uint16_t)((buffer[i] >> 8) | (buffer[i] << 8));

    // retro-go は常に「幅の整数倍」=丸ごとの行を送ってくる
    int lines = (int)length / s_win_w;
    int y0 = s_win_y + s_win_line;
    // draw_bitmap は [x0,y0)-(x1,y1) の半開区間。非同期DMA転送を開始し、完了を待つ。
    esp_lcd_panel_draw_bitmap(s_panel, s_win_x, y0, s_win_x + s_win_w, y0 + lines, buffer);
    xSemaphoreTake(s_trans_done, pdMS_TO_TICKS(200)); // 直前のDMAコピー完了を待ってから返却/次転送
    s_win_line += lines;
    xQueueSend(s_buffers, &buffer, portMAX_DELAY);
}

static void lcd_sync(void)
{
    // 単一フレームバッファDPI: draw_bitmap が即FBへ反映するため何もしない
}

static void lcd_set_backlight(float percent)
{
    int level = (int)RG_MIN(RG_MAX(percent, 0.f), 100.f);
    bsp_display_brightness_set(level);
    RG_LOGI("backlight set to %d%%\n", level);
}

static void lcd_init(void)
{
    s_buffers = xQueueCreate(TAB5_BUFFER_COUNT, sizeof(uint16_t *));
    for (int i = 0; i < TAB5_BUFFER_COUNT; i++)
    {
        void *buf = rg_alloc(LCD_BUFFER_LENGTH * sizeof(uint16_t), MEM_DMA);
        xQueueSend(s_buffers, &buf, portMAX_DELAY);
    }

    s_trans_done = xSemaphoreCreateBinary();

    // BSP が表示IC自動検出 → DSIバス/DPIパネル生成 → disp_on + brightness_init まで実施
    bsp_lcd_handles_t handles = {0};
    if (bsp_display_new_auto(NULL, &handles) != ESP_OK)
        RG_PANIC("bsp_display_new_auto failed");
    s_panel = handles.panel;

    // DPI 転送完了コールバックを登録(draw_bitmap の直列化に使用)
    esp_lcd_dpi_panel_event_callbacks_t cbs = {
        .on_color_trans_done = tab5_on_trans_done,
    };
    esp_lcd_dpi_panel_register_event_callbacks(s_panel, &cbs, NULL);

    bsp_display_backlight_on();
}

static void lcd_deinit(void)
{
    if (s_panel)
        esp_lcd_panel_del(s_panel);
    s_panel = NULL;
}

const rg_display_driver_t rg_display_driver_tab5 = {
    .name = "tab5_dsi",
};
