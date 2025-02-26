#include "gpio.h"

GPIO::GPIO(int num, gpio_mode_t mode, GPIO_PULL pull)
{
    this->Num = (gpio_num_t)num;

    gpio_config_t conf = 
    {
        .mode = mode,
        .pull_up_en = (pull & GPIO_PULL::Up) ? gpio_pullup_t::GPIO_PULLUP_ENABLE : gpio_pullup_t::GPIO_PULLUP_DISABLE,
        .pull_down_en = (pull & GPIO_PULL::Down) ? gpio_pulldown_t::GPIO_PULLDOWN_ENABLE : gpio_pulldown_t::GPIO_PULLDOWN_DISABLE,
    };

    gpio_config(&conf);
}

bool GPIO::Get()
{
    return gpio_get_level(this->Num);
}
    
void GPIO::Set(bool level) 
{
    gpio_set_level(this->Num, level ? 1 : 0);
}
