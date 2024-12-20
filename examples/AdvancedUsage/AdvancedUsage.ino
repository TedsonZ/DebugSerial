#include <DebugSerial.h>

// Task que usa a biblioteca para logar mensagens de forma concorrente
void task1(void *pvParameters) {
    int count = 0;
    while (true) {
        dlog("Task 1 - Contador: %d", count++);
        vTaskDelay(pdMS_TO_TICKS(10)); // 500 ms de atraso
    }
}

void task2(void *pvParameters) {
    while (true) {
        dlog("Task 2 - Tempo atual em millis: %lu", millis());
        vTaskDelay(pdMS_TO_TICKS(12)); // 700 ms de atraso
    }
}

void setup() {
    // Inicializa a Serial
    Serial.begin(115200);

    // Inicializa a biblioteca com configurações personalizadas
    // Ativa o debug, define a fila para 20 mensagens e tamanho máximo de 256 caracteres por mensagem
    initializeDebugSerial(1, 20, 256);

    // Cria duas tasks que usam a biblioteca para logar mensagens
    xTaskCreate(task1, "Task1", 2048, NULL, 1, NULL);
    xTaskCreate(task2, "Task2", 2048, NULL, 1, NULL);

    dlog("Sistema inicializado com sucesso!");
}

void loop() {
    // Loga mensagens do loop principal
    static int loopCounter = 0;
    dlog("Loop principal - Contagem: %d", loopCounter++);
    vTaskDelay(pdMS_TO_TICKS(1000)); // Simula processamento no loop principal com atraso de 1 segundo
}
