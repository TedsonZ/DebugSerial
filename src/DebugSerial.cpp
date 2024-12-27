#include "DebugSerial.h"

static QueueHandle_t serialQueue;
static int debugEnabled = DEFAULT_DEBUG_SERIAL;
static size_t queueSize = DEFAULT_SERIAL_QUEUE_SIZE;
static size_t messageLength = DEFAULT_SERIAL_MESSAGE_MAX_LENGTH;

struct DebugMessage
{
    char *message;
};

/**
 * @brief Task responsável por imprimir na serial as mensagens enfileiradas
 *
 * Essa task é responsável por imprimir na serial as mensagens enfileiradas em
 * serialQueue. Ela permanece em loop infinito, aguardando mensagens na fila,
 * e as imprime na serial. Além disso, ela também imprime o tempo gasto para
 * imprimir a mensagem e o número de mensagens pendentes na fila.
 *
 * @param pvParameters parâmetro padrão de tasks do FreeRTOS, não é utilizado
 * aqui.
 */
static void serialTask(void *pvParameters)
{
    DebugMessage debugMessage;

    while (true)
    {
        if (xQueueReceive(serialQueue, &debugMessage, portMAX_DELAY))
        {
            unsigned long inicio = micros();
            // Verifica se a mensagem é vazia ou contém apenas espaço
            if (strlen(debugMessage.message) == 0 || strcmp(debugMessage.message, " ") == 0)
            {
                Serial.println(); // Apenas pula uma linha
            }
            else
            {
                unsigned long inicio = micros();
                Serial.print(debugMessage.message);
                unsigned long fim = micros();
                unsigned long tempoGasto = fim - inicio;
                size_t mensagensPendentes = uxQueueMessagesWaiting(serialQueue);

                Serial.printf(" [%lu µs | Pendentes: %d]\n", tempoGasto, mensagensPendentes);
            }

            delete[] debugMessage.message; // Libera memória alocada dinamicamente
        }
    }
}

/**
 * @brief Initializes the debug serial communication.
 *
 * This function sets up the debug serial by configuring the debug mode,
 * queue size, and message length based on the provided parameters. If
 * the debug mode is enabled, it creates a queue for serial messages and
 * starts a task to handle serial output.
 *
 * @param debugSerial Flag to enable or disable debug serial communication.
 * @param userQueueSize The size of the queue for storing debug messages.
 * @param userMessageLength The maximum length of individual debug messages.
 */
void initializeDebugSerial(int debugSerial, size_t userQueueSize, size_t userMessageLength)
{
    debugEnabled = debugSerial;
    queueSize = userQueueSize;
    messageLength = userMessageLength;

    if (debugEnabled)
    {
        serialQueue = xQueueCreate(queueSize, sizeof(DebugMessage));
        if (serialQueue == NULL)
        {
            Serial.println("Erro ao criar fila para Serial.");
            return;
        }
        xTaskCreate(serialTask, "SerialTask", 2048, NULL, 1, NULL);
        Serial.println("Debug Serial inicializado.");
    }
}

// Declarações adicionais para Serial2
static QueueHandle_t serial2Queue;
static int debugEnabledSerial2 = DEFAULT_DEBUG_SERIAL2;

struct DebugMessage2
{
    char *message;
};

// Task para Serial2
static void serial2Task(void *pvParameters)
{
    DebugMessage2 debugMessage;

    while (true)
    {
        if (xQueueReceive(serial2Queue, &debugMessage, portMAX_DELAY))
        {
            unsigned long inicio = micros();
            if (strlen(debugMessage.message) == 0 || strcmp(debugMessage.message, " ") == 0)
            {
                Serial2.println();
            }
            else
            {
                Serial2.print(debugMessage.message);
                unsigned long fim = micros();
                unsigned long tempoGasto = fim - inicio;
                size_t mensagensPendentes = uxQueueMessagesWaiting(serial2Queue);

                Serial2.printf(" [%lu µs | Pendentes: %d]\n", tempoGasto, mensagensPendentes);
            }

            delete[] debugMessage.message;
        }
    }
}

// Inicializar Serial2 Debug
void initializeDebugSerial2(int debugSerial, size_t queueSize, size_t messageLength)
{
    debugEnabledSerial2 = debugSerial;

    if (debugEnabledSerial2)
    {
        serial2Queue = xQueueCreate(queueSize, sizeof(DebugMessage2));
        if (serial2Queue == NULL)
        {
            Serial.println("Erro ao criar fila para Serial2.");
            return;
        }
        xTaskCreate(serial2Task, "Serial2Task", 2048, NULL, 1, NULL);
        Serial2.println("Debug Serial2 inicializado.");
    }
}

/**
 * @brief Logs a formatted debug message to the serial output.
 *
 * This function uses a variadic argument list to format a message
 * and send it to a serial queue for printing. If the debug mode is
 * disabled, the function returns immediately. Otherwise, it formats
 * the message using the provided format string and arguments, allocates
 * memory for the message, and attempts to send it to the back of the
 * serial queue. If the queue is full, the message is discarded and memory
 * is freed.
 *
 * @param format A C-style format string that specifies how to format the message.
 * @param ... Additional arguments to format the message according to the format string.
 */
void dlog(const char *format, ...)
{
    if (!debugEnabled)
        return;

    DebugMessage debugMessage;
    debugMessage.message = new char[messageLength]; // Aloca memória dinamicamente para a mensagem

    va_list args;
    va_start(args, format);
    vsnprintf(debugMessage.message, messageLength, format, args);
    va_end(args);

    if (serialQueue != NULL)
    {
        if (xQueueSendToBack(serialQueue, &debugMessage, pdMS_TO_TICKS(100)) != pdPASS)
        {
            Serial.println("Fila de debug cheia. Mensagem descartada.");
            delete[] debugMessage.message; // Libera memória se a mensagem não for enviada
        }
    }
}

/**
 * @brief Logs an integer value to the serial output.
 *
 * @param value The value to log.
 */
void dlog(int value)
{
    dlog("Valor: %d", value);
}

/**
 * @brief Logs an unsigned integer value to the serial output.
 *
 * @param value The value to log.
 */
void dlog(unsigned int value)
{
    /**
     * @brief Logs a long integer value to the serial output.
     *
     * @param value The value to log.
     */
    dlog("Valor: %u", value);
}

/**
 * @brief Logs a long integer value to the serial output.
 *
 * @param value The value to log.
 */
void dlog(long value)
{
    /**
     * @brief Logs an unsigned long integer value to the serial output.
     *
     * @param value The value to log.
     */
    dlog("Valor: %ld", value);
}

/**
 * @brief Logs an unsigned long integer value to the serial output.
 *
 * @param value The value to log.
 */
void dlog(unsigned long value)
{
    dlog("Valor: %lu", value);
}

/**
 * @brief Logs a floating-point value to the serial output.
 *
 * @param value The value to log.
 */
void dlog(float value)
{
    dlog("Valor: %.2f", value);
}

/**
 * @brief Logs a double value to the serial output.
 *
 * @param value The value to log.
 */

void dlog(double value)
{
    dlog("Valor: %.4lf", value);
}

void dlog(bool value)
{
    dlog("Valor: %s", value ? "true" : "false");
}

/**
 * @brief Logs a character value to the serial output.
 *
 * @param value The character to log.
 */
void dlog(char value)
{
    dlog("Valor: %c", value);
}

/**
 * @brief Logs a floating-point value to the serial output with a specified number of decimal places.
 *
 * @param value The value to log.
 * @param decimalPlaces The number of decimal places to format the value with.
 */
void dlog(float value, int decimalPlaces)
{
    // Cria um buffer para armazenar o valor formatado
    char buffer[DEFAULT_SERIAL_MESSAGE_MAX_LENGTH];

    // Formata o valor de ponto flutuante com o número de casas decimais desejado
    snprintf(buffer, sizeof(buffer), "%.*f", decimalPlaces, value);

    // Envia o valor formatado para a sobrecarga existente
    dlog(buffer);
}

/**
 * @brief Logs a String value to the serial output.
 *
 * @param value The String to log.
 */
void dlog(const String &value)
{
    dlog("Valor: %s", value.c_str());
}

void dlog2(const char *format, ...)
{
    if (!debugEnabledSerial2)
        return;

    DebugMessage2 debugMessage;
    debugMessage.message = new char[DEFAULT_SERIAL_MESSAGE_MAX_LENGTH];

    va_list args;
    va_start(args, format);
    vsnprintf(debugMessage.message, DEFAULT_SERIAL_MESSAGE_MAX_LENGTH, format, args);
    va_end(args);

    if (serial2Queue != NULL)
    {
        if (xQueueSendToBack(serial2Queue, &debugMessage, pdMS_TO_TICKS(100)) != pdPASS)
        {
            Serial2.println("Fila de debug Serial2 cheia. Mensagem descartada.");
            delete[] debugMessage.message;
        }
    }
}

void dlog2(int value)
{
    dlog2("Valor: %d", value);
}

void dlog2(unsigned int value)
{
    dlog2("Valor: %u", value);
}

void dlog2(long value)
{
    dlog2("Valor: %ld", value);
}

void dlog2(unsigned long value)
{
    dlog2("Valor: %lu", value);
}

void dlog2(float value)
{
    dlog2("Valor: %.2f", value);
}

void dlog2(double value)
{
    dlog2("Valor: %.4lf", value);
}

void dlog2(bool value)
{
    dlog2("Valor: %s", value ? "true" : "false");
}

void dlog2(char value)
{
    dlog2("Valor: %c", value);
}

void dlog2(float value, int decimalPlaces)
{
    char buffer[DEFAULT_SERIAL_MESSAGE_MAX_LENGTH];
    snprintf(buffer, sizeof(buffer), "%.*f", decimalPlaces, value);
    dlog2(buffer);
}

void dlog2(const String &value)
{
    dlog2("Valor: %s", value.c_str());
}
