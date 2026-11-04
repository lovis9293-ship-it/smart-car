#ifndef __RC522_H
#define __RC522_H
#include <esp_log.h>
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"

typedef struct 
{
    int RC522_SPI_BUS_GPIO_MISO;
    int RC522_SPI_BUS_GPIO_MOSI;
    int RC522_SPI_BUS_GPIO_SCLK;
    int RC522_SPI_SCANNER_GPIO_SDA;
    int RC522_SCANNER_GPIO_RST;

    rc522_driver_handle_t driver;
    rc522_handle_t scanner;
    
}RC522_Typedef;
void rc522_init(RC522_Typedef *rc,int RC522_SPI_BUS_GPIO_MISO,int RC522_SPI_BUS_GPIO_MOSI,int RC522_SPI_BUS_GPIO_SCLK,int RC522_SPI_SCANNER_GPIO_SDA,int RC522_SCANNER_GPIO_RST);

#endif    
