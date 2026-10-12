#include "bme280.h"

#define CS_LOW()   HAL_GPIO_WritePin(BME_CS_GPIO_Port, BME_CS_Pin, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(BME_CS_GPIO_Port, BME_CS_Pin, GPIO_PIN_SET)

#define REG_ID        0xD0
#define REG_RESET     0xE0
#define REG_CTRL_HUM  0xF2
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG    0xF5
#define REG_DATA      0xF7

static SPI_HandleTypeDef *spi;
static int32_t t_fine;

static struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    uint8_t  H1; int16_t H2; uint8_t H3; int16_t H4, H5; int8_t H6;
} cal;


static HAL_StatusTypeDef read_regs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t cmd = reg | 0x80;
    CS_LOW();
    HAL_StatusTypeDef st = HAL_SPI_Transmit(spi, &cmd, 1, 20);
    if (st == HAL_OK) st = HAL_SPI_Receive(spi, buf, len, 20);
    CS_HIGH();
    return st;
}

static HAL_StatusTypeDef write_reg(uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg & 0x7F, val };
    CS_LOW();
    HAL_StatusTypeDef st = HAL_SPI_Transmit(spi, tx, 2, 20);
    CS_HIGH();
    return st;
}

/* ---------- калібрування ---------- */
static HAL_StatusTypeDef read_calibration(void)
{
    uint8_t a[24], h1, e[7];
    HAL_StatusTypeDef st;

    if ((st = read_regs(0x88, a, 24)) != HAL_OK) return st;
    if ((st = read_regs(0xA1, &h1, 1)) != HAL_OK) return st;
    if ((st = read_regs(0xE1, e, 7))  != HAL_OK) return st;

    // little-endian: молодший байт першим
    cal.T1 = (uint16_t)(a[1]  << 8 | a[0]);
    cal.T2 = (int16_t) (a[3]  << 8 | a[2]);
    cal.T3 = (int16_t) (a[5]  << 8 | a[4]);
    cal.P1 = (uint16_t)(a[7]  << 8 | a[6]);
    cal.P2 = (int16_t) (a[9]  << 8 | a[8]);
    cal.P3 = (int16_t) (a[11] << 8 | a[10]);
    cal.P4 = (int16_t) (a[13] << 8 | a[12]);
    cal.P5 = (int16_t) (a[15] << 8 | a[14]);
    cal.P6 = (int16_t) (a[17] << 8 | a[16]);
    cal.P7 = (int16_t) (a[19] << 8 | a[18]);
    cal.P8 = (int16_t) (a[21] << 8 | a[20]);
    cal.P9 = (int16_t) (a[23] << 8 | a[22]);

    cal.H1 = h1;
    cal.H2 = (int16_t)(e[1] << 8 | e[0]);
    cal.H3 = e[2];

    cal.H4 = (int16_t)(((int8_t)e[3] << 4) | (e[4] & 0x0F));
    cal.H5 = (int16_t)(((int8_t)e[5] << 4) | (e[4] >> 4));
    cal.H6 = (int8_t)e[6];

    return HAL_OK;
}


static int32_t comp_temp(int32_t adc_T)
{
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)cal.T1 << 1))) * (int32_t)cal.T2) >> 11;
    int32_t v2 = (((((adc_T >> 4) - (int32_t)cal.T1) *
                    ((adc_T >> 4) - (int32_t)cal.T1)) >> 12) * (int32_t)cal.T3) >> 14;
    t_fine = v1 + v2;
    return (t_fine * 5 + 128) >> 8;
}

static uint32_t comp_press(int32_t adc_P)
{
    int64_t v1 = (int64_t)t_fine - 128000;
    int64_t v2 = v1 * v1 * (int64_t)cal.P6;
    v2 += (v1 * (int64_t)cal.P5) << 17;
    v2 += ((int64_t)cal.P4) << 35;
    v1 = ((v1 * v1 * (int64_t)cal.P3) >> 8) + ((v1 * (int64_t)cal.P2) << 12);
    v1 = (((((int64_t)1) << 47) + v1) * (int64_t)cal.P1) >> 33;
    if (v1 == 0) return 0;

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - v2) * 3125) / v1;
    v1 = ((int64_t)cal.P9 * (p >> 13) * (p >> 13)) >> 25;
    v2 = ((int64_t)cal.P8 * p) >> 19;
    p = ((p + v1 + v2) >> 8) + ((int64_t)cal.P7 << 4);
    return (uint32_t)(p / 256);
}

static uint32_t comp_hum(int32_t adc_H)
{
    int32_t v = t_fine - 76800;
    v = (((((adc_H << 14) - ((int32_t)cal.H4 << 20) - ((int32_t)cal.H5 * v)) + 16384) >> 15) *
         (((((((v * (int32_t)cal.H6) >> 10) *
              (((v * (int32_t)cal.H3) >> 11) + 32768)) >> 10) + 2097152) *
           (int32_t)cal.H2 + 8192) >> 14));
    v -= (((((v >> 15) * (v >> 15)) >> 7) * (int32_t)cal.H1) >> 4);
    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;
    return (uint32_t)(v >> 12);
}


HAL_StatusTypeDef bme280_init(SPI_HandleTypeDef *hspi)
{
    spi = hspi;
    CS_HIGH();
    HAL_Delay(5);

    uint8_t id = 0;
    HAL_StatusTypeDef st = read_regs(REG_ID, &id, 1);
    if (st != HAL_OK) return st;
    if (id != 0x60) return HAL_ERROR;

    write_reg(REG_RESET, 0xB6);
    HAL_Delay(5);

    if ((st = read_calibration()) != HAL_OK) return st;

    if ((st = write_reg(REG_CTRL_HUM, 0x01)) != HAL_OK) return st;
    if ((st = write_reg(REG_CONFIG,   0x00)) != HAL_OK) return st;
    return write_reg(REG_CTRL_MEAS, (1 << 5) | (1 << 2) | 0x03);
}

HAL_StatusTypeDef bme280_read(Bme280Data *d)
{
    uint8_t b[8];
    HAL_StatusTypeDef st = read_regs(REG_DATA, b, 8);
    if (st != HAL_OK) return st;

    int32_t adc_P = ((int32_t)b[0] << 12) | ((int32_t)b[1] << 4) | (b[2] >> 4);
    int32_t adc_T = ((int32_t)b[3] << 12) | ((int32_t)b[4] << 4) | (b[5] >> 4);
    int32_t adc_H = ((int32_t)b[6] << 8)  |  b[7];

    d->temp_c100 = comp_temp(adc_T);
    d->press_pa  = comp_press(adc_P);
    d->hum_q10   = comp_hum(adc_H);
    return HAL_OK;
}
