/* Configuration for the ESP32-P4 dev board
 * The GPIOs were chosen arbitrarily, but you can choose whatever you want thanks to the I/O MUX
 * command to build: python rg_tool.py --target esp32-p4 build-img --no-networking
*/

/****************************************************************************
 * Target definition for ESP32-P4 Dev-Board                                 *
 ****************************************************************************/
#define RG_TARGET_NAME             "TAB5"


/****************************************************************************
 * Status LED                                                               *
 ****************************************************************************/
// #define RG_LED_DRIVER               1   // 1 = GPIO
// #define RG_GPIO_LED                 GPIO_NUM_NC
// #define RG_GPIO_LED_INVERT          // Uncomment if the LED is active LOW


/****************************************************************************
 * I2C / GPIO Extender                                                      *
 ****************************************************************************/
// #define RG_I2C_GPIO_DRIVER          0   // 1 = AW9523, 2 = PCF9539, 3 = MCP23017, 4 = PCF8575
// #define RG_I2C_GPIO_ADDR            0x00
// #define RG_GPIO_I2C_SDA             GPIO_NUM_15
// #define RG_GPIO_I2C_SCL             GPIO_NUM_4


/****************************************************************************
 * Storage                                                                  *
 ****************************************************************************/
#define RG_STORAGE_ROOT             "/sd"
// #define RG_STORAGE_SDSPI_HOST       SPI2_HOST
// #define RG_STORAGE_SDSPI_SPEED      SDMMC_FREQ_DEFAULT
// #define RG_GPIO_SDSPI_MISO          GPIO_NUM_19
// #define RG_GPIO_SDSPI_MOSI          GPIO_NUM_23
// #define RG_GPIO_SDSPI_CLK           GPIO_NUM_18
// #define RG_GPIO_SDSPI_CS            GPIO_NUM_22
#define RG_STORAGE_SDMMC_HOST       SDMMC_HOST_SLOT_1
#define RG_STORAGE_SDMMC_SPEED      SDMMC_FREQ_HIGHSPEED
#define RG_GPIO_SDMMC_CLK           GPIO_NUM_43
#define RG_GPIO_SDMMC_CMD	        GPIO_NUM_44
#define RG_GPIO_SDMMC_D0	        GPIO_NUM_39
#define RG_GPIO_SDMMC_D1	        GPIO_NUM_40
#define RG_GPIO_SDMMC_D2	        GPIO_NUM_41
#define RG_GPIO_SDMMC_D3	        GPIO_NUM_42
// #define RG_STORAGE_FLASH_PARTITION  "vfs"


/****************************************************************************
 * Audio                                                                    *
 ****************************************************************************/
// Tab5: 音声は ES8388 codec(I2S)。codec の I2C 設定は Phase4 で BSP(esp_codec_dev)を使う。
// ここでは I2S ピンのみ Tab5 配線に合わせる(BCLK=27, WS/LRCK=29, DOUT=26)。MCLK=30 は BSP/Phase4 で扱う。
#define RG_AUDIO_USE_INT_DAC        0   // 0 = Disable
#define RG_AUDIO_USE_EXT_DAC        1   // 0 = Disable, 1 = Enable
#define RG_GPIO_SND_I2S_BCK         GPIO_NUM_27
#define RG_GPIO_SND_I2S_WS          GPIO_NUM_29
#define RG_GPIO_SND_I2S_DATA        GPIO_NUM_26
// #define RG_GPIO_SND_AMP_ENABLE   GPIO_NUM_NC  // Tab5 は AMP enable 直結GPIOなし(BSP_POWER_AMP_IO=NC)


/****************************************************************************
 * Video                                                                    *
 ****************************************************************************/
// Tab5: MIPI-DSI パネル。実初期化は drivers/display/tab5_dsi.h が公式BSP(bsp_display_new_auto)で行う。
// パネル native は 720x1280 縦・RGB565・2レーン/730Mbps。
// Phase1 は native の縦 720x1280 のまま描画(回転なし)。横向き1280x720化は後続フェーズ。
#define RG_SCREEN_DRIVER            2   // 2 = Tab5 MIPI-DSI (m5stack_tab5 BSP)
#define RG_SCREEN_WIDTH             720
#define RG_SCREEN_HEIGHT            1280
#define RG_SCREEN_BACKLIGHT         1
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}  // Left, Top, Right, Bottom
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}  // Left, Top, Right, Bottom
#define RG_SCREEN_PARTIAL_UPDATES   0   // DSIはフルフレームバッファ運用、部分更新なし
#define RG_SCREEN_INIT()                                                                                     \
ILI9341_CMD(0xCF, 0x00, 0xc3, 0x30);                                                                         \
ILI9341_CMD(0xED, 0x64, 0x03, 0x12, 0x81);                                                                   \
ILI9341_CMD(0xE8, 0x85, 0x00, 0x78);                                                                         \
ILI9341_CMD(0xCB, 0x39, 0x2c, 0x00, 0x34, 0x02);                                                             \
ILI9341_CMD(0xF7, 0x20);                                                                                     \
ILI9341_CMD(0xEA, 0x00, 0x00);                                                                               \
ILI9341_CMD(0xC0, 0x1B);                 /* Power control   //VRH[5:0] */                                    \
ILI9341_CMD(0xC1, 0x12);                 /* Power control   //SAP[2:0];BT[3:0] */                            \
ILI9341_CMD(0xC5, 0x32, 0x3C);           /* VCM control */                                                   \
ILI9341_CMD(0xC7, 0x91);                 /* VCM control2 */                                                  \
ILI9341_CMD(0x36, 0x08); /* Memory Access Control */                                         \
ILI9341_CMD(0xB1, 0x00, 0x10);           /* Frame Rate Control (1B=70, 1F=61, 10=119) */                     \
ILI9341_CMD(0xB6, 0x0A, 0xA2);           /* Display Function Control */                                      \
ILI9341_CMD(0xF6, 0x01, 0x30);                                                                               \
ILI9341_CMD(0xF2, 0x00); /* 3Gamma Function Disable */                                                       \
ILI9341_CMD(0x26, 0x01); /* Gamma curve selected */                                                          \
ILI9341_CMD(0xE0, 0xD0, 0x00, 0x02, 0x07, 0x0a, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0e, 0x12, 0x14, 0x17);       \
ILI9341_CMD(0xE1, 0xD0, 0x00, 0x02, 0x07, 0x0a, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1b, 0x1e);       

#define RG_GPIO_LCD_MISO            GPIO_NUM_NC
#define RG_GPIO_LCD_MOSI            GPIO_NUM_22
#define RG_GPIO_LCD_CLK             GPIO_NUM_23
#define RG_GPIO_LCD_CS              GPIO_NUM_24
#define RG_GPIO_LCD_DC              GPIO_NUM_25
#define RG_GPIO_LCD_RST             GPIO_NUM_26
// #define RG_GPIO_LCD_BCKL         GPIO_NUM_NC
// #define RG_GPIO_LCD_BCKL_INVERT  // Uncomment if the LED is active LOW


/****************************************************************************
 * Input                                                                    *
 ****************************************************************************/
// Tab5 は物理ゲームパッド非搭載。入力は Phase2 で USB-A ホストの HID ゲームパッドを
// rg_input に新規パスとして追加する(BSP 依存に usb_host_hid あり)。
// Phase1(表示確認)では入力マップ無し = 全キー未押下。ランチャーUI描画の確認が目的。
// (GPIO 9-21 は Tab5 の周辺機能と競合しうるため、旧 esp32-p4 の GPIO マップは使わない)
// #define RG_GAMEPAD_GPIO_MAP { ... }  // Phase2 で USB HID に置換


/****************************************************************************
 * Battery                                                                  *
 ****************************************************************************/
// #define RG_BATTERY_DRIVER            1   // 1 = ADC, 2 = MRGC
// #define RG_BATTERY_ADC_UNIT          ADC_UNIT_1
// #define RG_BATTERY_ADC_CHANNEL       ADC_CHANNEL_0
// #define RG_BATTERY_CALC_PERCENT(raw) (((raw) * 2.f - 3500.f) / (4200.f - 3500.f) * 100.f)
// #define RG_BATTERY_CALC_VOLTAGE(raw) ((raw) * 2.f * 0.001f)


/****************************************************************************
 * Updater                                                                  *
 ****************************************************************************/
// #define RG_UPDATER_ENABLE               1
// #define RG_UPDATER_APPLICATION          RG_APP_FACTORY
// #define RG_UPDATER_DOWNLOAD_LOCATION    RG_STORAGE_ROOT "/odroid/firmware"



/****************************************************************************
 * Miscellaneous                                                            *
 ****************************************************************************/
#define RG_RECOVERY_BTN                 RG_KEY_MENU // Keep this button pressed to open the recovery menu

#define RG_CUSTOM_PLATFORM_INIT() \
    /* Arbitrary code executed very early during retro-go init */

// See components/retro-go/config.h for more things you can define here!