# Tab5 retro-go 移植プラン（ファミコン / スーファミ）

Tab5 (ESP32-P4) で NES + SNES を動かすため、retro-go を Tab5 ターゲットとして移植する。
エミュレータコア（nofrendo=NES, snes9x=SNES 等）は完成済みを流用し、**Tab5 のハード接続を新規に書く**のが作業の本体。

## Git 管理
- リモート: `origin = git@github.com:htpboost/retrogo-tab5.git`（SSH。HTTPSはworkflowスコープで弾かれるためSSH必須）、`upstream = github.com/rapha-tech/retro-go`
- 作業ブランチ: `tab5-port`

## Phase 1 確定事項（Tab5 表示 — 公式 m5stack_tab5 BSP より抽出済み）
- パネル native = **720(H)×1280(V) 縦**（横向き 1280×720 は回転表示）。色 = **RGB565 16bpp LE**（retro-go内部と一致→変換不要）。
- MIPI-DSI = **2レーン / 730 Mbps**。タイミング HSYNC10/HBP40/HFP40, VSYNC4/VBP16/VFP16。DPI clk ~78MHz。
- パネルIC = ILI9881C / ST7123 を BSP が自動検出（手元実機は ST7123 世代の疑い）。
- **アプローチ: タイミングを手書きせず公式 BSP `platforms/tab5/components/m5stack_tab5/` を retro-go に component として取り込む。**
  - 主要ファイル: `m5stack_tab5.c`(bsp_display_new実体), `esp_lcd_st7123.c`, `include/bsp/ili9881_init_data.c`, `include/bsp/{display,config,esp-bsp,touch}.h`, `priv_include/esp_lcd_st7123.h`。依存: esp_lcd_st7121, GT911タッチ, IOエクスパンダ, 電源。
  - API: `bsp_display_new_with_handles(&cfg,&handles)` → `esp_lcd_panel_handle_t`。描画 `esp_lcd_panel_draw_bitmap()`。輝度 `bsp_display_brightness_init()/set()`、`bsp_display_backlight_on()`。
- `tab5_dsi.h` 実装方針: lcd_init=bsp_display_new+brightness_init+backlight_on / lcd_send_buffer=draw_bitmap(回転・拡大込み) / lcd_set_backlight=brightness_set。emulator出力(256×240等 RGB565)を1280×720領域へ整数倍 or 中央寄せ。
- 参照ソースのローカル取得先: /tmp/tab5disp（esp_lcd_st7121.{c,h}, display.h, config.h, tree.json）。本番はBSPをupstream component registryからも取得可。

## ベース
- フォーク: `rapha-tech/retro-go` ブランチ `ESP32-P4-clean`（issue ducalex/retro-go#211）
- 置き場所: `~/M5Stack/TAB5/retro-go/`
- ビルド: ESP-IDF **v5.5**（`~/esp/esp-idf-v5.5`）。既存の v4.4.8 は P4 非対応。
- ビルドコマンド: `python rg_tool.py --target <tab5> build-img`（フォークの流儀）

## フォークの前提ハード（P4-MINI）と Tab5 の差分（＝移植対象）
| 要素 | フォーク前提(P4-MINI) | Tab5 | 移植 |
|---|---|---|---|
| 表示 | ST7789V SPI 320×240 | MIPI-DSI 1280×720 | **全面書換(最難関)** |
| 音声 | NS4168 I2S DAC | ES8388 codec(I2C+I2S) | 書換 |
| 入力 | GPIO ボタン | USB-A ホスト HID パッド | 書換 |
| ストレージ | SDMMC | microSD(SDMMC) | ほぼ流用 |

## フェーズ
- [x] **Phase 0 土台(完了)**: ESP-IDF v5.5 導入(~/esp/esp-idf-v5.5, install.sh esp32p4) / フォーク clone 済み(~/M5Stack/TAB5/retro-go) / 素の esp32-p4 launcher を**フルビルド成功**(launcher.bin 生成)。※フォークの esp32-p4 config.h は `RG_GPIO_LCD_MISO` がコメントアウトされていてビルド不可→`GPIO_NUM_NC` で有効化して解決。
  - ビルド: `cd ~/M5Stack/TAB5/retro-go && source ~/esp/esp-idf-v5.5/export.sh && python rg_tool.py --target esp32-p4 build launcher --no-networking`
  - 表示ドライバ構造: `rg_display.c` が `RG_SCREEN_DRIVER` で `drivers/display/{ili9341,sdl2,dummy}.h` を切替。各ドライバは `lcd_init/lcd_deinit/lcd_sync/lcd_set_rotation/lcd_set_backlight/lcd_set_window/lcd_get_buffer/lcd_send_buffer` を提供。→ Tab5 は `drivers/display/tab5_dsi.h` を新規追加し `RG_SCREEN_DRIVER==2` 分岐を足す。
- [ ] **Phase 1 表示(最難関・新規コード)**: 下記参照。Tab5 MIPI-DSI バックエンドを retro-go に新規実装 → ランチャーUI表示が目標
- [ ] **Phase 2 入力**: ESP32-P4 USB host (HID) ゲームパッド → retro-go 入力にマップ(config.h の RG_GAMEPAD_GPIO_MAP を USB入力に置換)
- [ ] **Phase 3 ROM/SD**: microSD(SDMMC) から ROM 一覧/ロード。Tab5 の SDMMC ピンに RG_GPIO_SDMMC_* を合わせる
- [ ] **Phase 4 音声**: ES8388 を I2C 初期化 + I2S 供給(RG_AUDIO_USE_EXT_DAC=1, BCK/WS/DATA を Tab5 配線に)
- [ ] **Phase 5 SNES**: SNES モジュール有効化、400MHz・frameskip・音声プロファイルでチューニング

## ターゲット定義の仕組み（実コード確認済み）
- ターゲット = `components/retro-go/targets/<name>/` の `config.h` / `sdkconfig` / `env.py`。`esp32-p4` を `tab5` にコピーして書換。
- ビルド: `python rg_tool.py --target tab5 build-img`。
- 表示は `config.h` の `RG_SCREEN_DRIVER` で選択。`rg_display.c` は `==0 (ILI9341/ST7789 SPI)` と `==99 (custom)` のみ。**MIPI-DSI は無い**。

## Phase 1 詳細（最重要リスク）
- **retro-go に MIPI-DSI 表示ドライバが存在しない。** 既存 P4 フォーク(rapha-tech/snes9x_esp32)も小型 **SPI ST7789** パネル前提で、MIPI-DSI は未解決。
- やること: `rg_display.c` に新ドライバ分岐(例 `RG_SCREEN_DRIVER==2 MIPI-DSI`、または `==99` custom フック)を追加し、**esp_lcd の DPI パネル(esp_lcd_dpi)** で Tab5 パネルを初期化。emulator の出力バッファ(256×240 等)を DPI フレームバッファへ拡大コピー(整数倍 or 中央寄せ)。
- パネル init は **公式 M5Tab5-UserDemo / espp m5stack-tab5 / esp-bsp** から移植。Tab5 個体差: ~2025/10/14前= **ILI9881C + GT911**、以降= **ST7123統合**。**この実機は起動ログ上 ST7123 世代の疑い**。
- **⚠️ ESP-IDF バージョン地雷**: Tab5 の MIPI-DSI は **v5.4.2 で安定、v5.5.X で黒画面/横縞**の報告(espressif/esp-idf#18083)。Phase 0(SPIターゲットのビルド疎通)は v5.5 でOKだが、**Phase 1 で DSI が出ない場合は v5.4.2 へ切替**(または該当 DSI 修正をbackport)を検討する。

## Tab5 ハード諸元（移植で参照）
- SoC: ESP32-P4 (RISC-V dual 360/400MHz)、Flash 16MB / PSRAM 32MB Octal
- 表示: 5" IPS 1280×720 MIPI-DSI（パネルIC/レーン数は要確認 → 公式 M5Tab5-UserDemo / esp-bsp から取得）
- タッチ: ST7123/ST7121（エミュでは未使用でも可）
- 音声: ES8388 codec + ES7210 mic、IMU BMI270、RTC RX8130CE
- 実機ポート: /dev/cu.usbmodem2101、USB MAC 30:ed:a0:e2:7f:57

## 参考
- 公式 NES: https://github.com/m5stack/M5Stack-nesemu （nofrendo, ESP-IDF, Tab5動作動画あり）
- SNES on P4: https://github.com/fcipaq/snes9x_esp32 （retro-go ベース, EV-board/Pico Held 2向け）
- retro-go P4: https://github.com/ducalex/retro-go/issues/211 , fork rapha-tech/retro-go#ESP32-P4-clean
- Tab5 公式demo(ESP-IDF, パネル初期化の参照元): https://github.com/m5stack/M5Tab5-UserDemo
