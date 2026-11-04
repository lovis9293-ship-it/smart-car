#include "car.h"
#include "freertos/task.h"


// Chuyen muc toc do thanh PWM
static int map_speed(int level)
{
    if (level < 1) level = 1;
    if (level > 9) level = 9;

    return 600 + level * 44;
}

// Xe tien
static void car_forward(Car_typedef *car, uint32_t speed)
{
    motor_backward(&car->left, speed);
    motor_forward(&car->right, speed);
}

// Xe lui
static void car_backward(Car_typedef *car, uint32_t speed)
{
    motor_forward(&car->left, speed);
    motor_backward(&car->right, speed);
}

// Re trai
static void car_left(Car_typedef *car, uint32_t speed)
{
    motor_backward(&car->left, speed / 4);
    motor_forward(&car->right, speed);
}

// Re phai
static void car_right(Car_typedef *car, uint32_t speed)
{
    motor_backward(&car->left, speed);
    motor_forward(&car->right, speed / 4);
}

// Dung xe
static void car_stop(Car_typedef *car)
{
    motor_stop(&car->left);
    motor_stop(&car->right);
}

// Chi goi khi dang giu cmd_mutex.
static bool car_blocked(Car_typedef *car)
{
    return !car->unlocked || car->obstacle;
}

// Task dieu khien xe
static void car_task(void *arg)
{
    Car_typedef *car = (Car_typedef *)arg;

    while (1)
    {
        xSemaphoreTake(car->cmd_mutex, portMAX_DELAY);

        if (car_blocked(car)) car->command = 'x';
        char cmd = car->command;
        int pwm = map_speed(car->speed_level);

        // Giu mutex qua luc ap dung lenh: tranh lenh cu chay sau khi khoa.
        // Khong delay/log/cho cam bien trong mutex nay.
        switch (cmd)
        {
            case 'w':
                car_forward(car, pwm);
                break;

            case 's':
                car_backward(car, pwm);
                break;

            case 'a':
                car_left(car, pwm);
                break;

            case 'd':
                car_right(car, pwm);
                break;

            case 'x':
            default:
                car_stop(car);
                break;
        }

        xSemaphoreGive(car->cmd_mutex);
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));
    }
}

// Cap nhat lenh nhan duoc
void car_set_command(Car_typedef *car, char cmd)
{
    xSemaphoreTake(car->cmd_mutex, portMAX_DELAY);

    if (cmd >= '1' && cmd <= '9')
    {
        car->speed_level = cmd - '0';
    }
    else if (cmd == 'w' || cmd == 's' ||
             cmd == 'a' || cmd == 'd' ||
             cmd == 'x')
    {
        car->command = car_blocked(car) ? 'x' : cmd;
    }

    xSemaphoreGive(car->cmd_mutex);
}

// RFID va SafetyTask chi cap nhat trang thai, khong cham motor.
void car_set_unlocked(Car_typedef *car, bool unlocked)
{
    xSemaphoreTake(car->cmd_mutex, portMAX_DELAY);

    car->unlocked = unlocked;
    car->command = 'x';

    // Moi lan doi trang thai deu cho ket qua do moi.
    car->obstacle = true;
    car->unlock_generation++;

    if (unlocked)
    {
        // Cho phep task SR05 bat dau do.
        xEventGroupSetBits(
            car->state_event,
            CAR_UNLOCKED_BIT
        );
    }
    else
    {
        // Task SR05 se cho khi quay lai dau vong lap.
        xEventGroupClearBits(
            car->state_event,
            CAR_UNLOCKED_BIT
        );
    }

    xSemaphoreGive(car->cmd_mutex);

    // Danh thuc task xe de ap dung trang thai dung.
    xTaskNotifyGive(car->task_handle);
}

// Khoi tao xe
void car_init(Car_typedef *car)
{
    car->command = 'x';
    car->speed_level = 5;
    car->unlocked = false;

    // Chan xe cho den khi co ket qua do hop le.
    car->obstacle = true;
    car->unlock_generation = 0;
    car->state_event = xEventGroupCreate();
    configASSERT(car->state_event != NULL);

    car->task_handle = NULL;

    motor_init(&car->left,
               5, 22, 21,
               LEDC_CHANNEL_0,
               LEDC_TIMER_0);

    motor_init(&car->right,
               23, 19, 18,
               LEDC_CHANNEL_1,
               LEDC_TIMER_1);

    // motor_init dat duty = 0; tu day chi car_task dieu khien motor.
    car->cmd_mutex = xSemaphoreCreateMutex();
    configASSERT(car->cmd_mutex != NULL);

    BaseType_t ret = xTaskCreate(
        car_task,
        "car_task",
        2048,
        car,
        5,
        &car->task_handle
    );

    configASSERT(ret == pdPASS);
}