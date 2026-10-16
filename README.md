# STM32F411 Black Pill: ADC + UART через DMA
 
- Значення з АЦП зчитуються безперервно через **DMA** (circular-режим), процесор у цьому не бере участі.
- Усереднене значення форматується в рядок і відправляється в консоль через **UART також по DMA**.
Приклад виводу в терміналі:
 
```
Hello from Black Pill
ADC: 2048  (1650 mV)
ADC: 2051  (1652 mV)
```
 
## Обладнання
 
| Компонент | Призначення |
|---|---|
| Black Pill (STM32F411CEU6) | мікроконтролер |
| ST-Link V2 | прошивка та відладка (SWD) |
| USB-TTL адаптер | вивід UART на комп'ютер |
 
## Налаштування CubeMX
 
### Тактування
- **RCC → HSE**: Crystal/Ceramic Resonator (кварц 25 МГц)
- **SYS → Debug**: Serial Wire
- Поточна конфігурація: PLLM = 25, PLLN = 336, PLLP = 4, що дає **SYSCLK = 84 МГц**
### ADC1 (канал IN1, пін PA1)
 
| Параметр | Значення |
|---|---|
| Resolution | 12 bits |
| Scan Conversion Mode | Disabled |
| Continuous Conversion Mode | **Enabled** |
| DMA Continuous Requests | **Enabled** |
| Sampling Time | 84 Cycles (рекомендовано для потенціометра) |
 
**DMA Settings (ADC1):** DMA2 Stream 0, Mode **Circular**, Data Width **Half Word** (Peripheral і Memory), Increment Address тільки на **Memory**.
 
### USART1
 
| Параметр | Значення |
|---|---|
| Mode | Asynchronous |
| Baud Rate | 115200 |
| Word Length / Parity / Stop Bits | 8 bit / None / 1 |
| Пін TX | PA9 |
 
**DMA Settings (USART1_TX):** DMA2 Stream 7, Mode **Normal**, Data Width **Byte**, Increment Address тільки на **Memory**.
 
### NVIC
Мають бути увімкнені:
- DMA2 stream0 global interrupt
- DMA2 stream7 global interrupt
- **USART1 global interrupt**
