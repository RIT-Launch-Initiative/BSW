#include "datalogging.h"
#include "gnss.h"
#include "sensing.h"

int main(int argc, char** argv) {
    // Initialize the Arduino environment
    Serial.begin(9600);


    // Initialize
    sensingInit();
    dataloggingInit();
    gnssInit();

    // Create tasks
    xTaskCreate(sensingTask, "Sensing Task", 2048, NULL, 1, NULL);
    xTaskCreate(dataloggingTask, "Datalogging Task", 2048, NULL, 1, NULL);
    xTaskCreate(gnssTask, "GNSS Task", 2048, NULL, 1, NULL);

    // Start scheduler
    vTaskStartScheduler();

    // Should never reach here
    return 0;
}