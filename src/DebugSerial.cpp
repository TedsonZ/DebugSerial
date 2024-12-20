#include "DebugSerial.h"

static QueueHandle_t serialQueue;
static int debugEnabled = DEFAULT_DEBUG_SERIAL;
static size_t queueSize = DEFAULT_SERIAL_QUEUE_SIZE;
static size_t messageLength = DEFAULT_SERIAL_MESSAGE_MAX_LENGTH;

struct DebugMessage {
    char *message;
};

static void serialTask(void *pvParameters) {
    DebugMessage debugMessage;

    while (true) {
        if (xQueueReceive(serialQueue, &debugMessage, portMAX_DELAY)) {
            unsigned long inicio = micros();
            Serial.print(debugMessage.message);
            unsigned long fim = micros();
            unsigned long tempoGasto = fim - inicio;
            size_t mensagensPendentes = uxQueueMessagesWaiting(serialQueue);

            Serial.printf(" [%lu µs | Pendentes: %d]\n", tempoGasto, mensagensPendentes);

            delete[] debugMessage.message; // Libera memória alocada dinamicamente
        }
    }
}

void initializeDebugSerial(int debugSerial, size_t userQueueSize, size_t userMessageLength) {
    debugEnabled = debugSerial;
    queueSize = userQueueSize;
    messageLength = userMessageLength;

    if (debugEnabled) {
        serialQueue = xQueueCreate(queueSize, sizeof(DebugMessage));
        if (serialQueue == NULL) {
            Serial.println("Erro ao criar fila para Serial.");
            return;
        }
        xTaskCreate(serialTask, "SerialTask", 2048, NULL, 1, NULL);
        Serial.println("Debug Serial inicializado.");
    }
}

void dlog(const char *format, ...) {
    if (!debugEnabled) return;

    DebugMessage debugMessage;
    debugMessage.message = new char[messageLength]; // Aloca memória dinamicamente para a mensagem

    va_list args;
    va_start(args, format);
    vsnprintf(debugMessage.message, messageLength, format, args);
    va_end(args);

    if (serialQueue != NULL) {
        if (xQueueSendToBack(serialQueue, &debugMessage, pdMS_TO_TICKS(100)) != pdPASS) {
            Serial.println("Fila de debug cheia. Mensagem descartada.");
            delete[] debugMessage.message; // Libera memória se a mensagem não for enviada
        }
    }
}

// Sobrecargas para diferentes tipos de dados
void dlog(int value) {
    dlog("Valor: %d", value);
}

void dlog(unsigned int value) {
    dlog("Valor: %u", value);
}

void dlog(long value) {
    dlog("Valor: %ld", value);
}

void dlog(unsigned long value) {
    dlog("Valor: %lu", value);
}

void dlog(float value) {
    dlog("Valor: %.2f", value);
}

void dlog(double value) {
    dlog("Valor: %.4lf", value);
}

void dlog(bool value) {
    dlog("Valor: %s", value ? "true" : "false");
}

void dlog(char value) {
    dlog("Valor: %c", value);
}
/*
// Remova a implementação redundante
void dlog(const char *value) {
    dlog("Valor: %s", value);
}
*/

void dlog(float value, int decimalPlaces) {
    // Cria um buffer para armazenar o valor formatado
    char buffer[DEFAULT_SERIAL_MESSAGE_MAX_LENGTH];
    
    // Formata o valor de ponto flutuante com o número de casas decimais desejado
    snprintf(buffer, sizeof(buffer), "%.*f", decimalPlaces, value);

    // Envia o valor formatado para a sobrecarga existente
    dlog(buffer);
}


void dlog(const String &value) {
    dlog("Valor: %s", value.c_str());
}
