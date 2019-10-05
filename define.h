#define VERSION  "V1.1"

// enable functions
#undef  ENABLE_SWR_CLASS                // SWR calculator class enable option
#define ENABLE_MENU                     // OLED Menu Option
#define ENABLE_HAMSKEY_I2C              // Hamskey I2C function enable/disable option

// OLED
#undef  OLED_SSD1306                    // SPI OLED driver
#define OLED_SH1106                     // I2C OLED driver

// OLED Graphic Meter Port
#define fwd_pwr_input  A2               // forward power meter analog input
#define rev_pwr_input  A3               // reverse power meter analog input
#define s_meter_input  A6               // S-Meter analog input (from AGC line)
#define vu_meter_input A10              // vu-Meter analog input (from audio signal)

// Hamskey I2C type Signal Meter Port
#define SWR_METER_ADC     A2            // for v0.8 SWR FWD
#define POWER_METER_ADC   A3            // for v0.8 SWR REV
#define SIGNAL_METER_ADC  A7            // signal meter
#undef  I2C_VER_0_7                     // use signal analyzer
#define I2C_VER_0_8                     // use SWR and RF power sensor

