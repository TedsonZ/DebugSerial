#include "DebugSerial.h"

static QueueHandle_t serialQueue;
static int debugEnabled = DEFAULT_DEBUG_SERIAL;
static size_t queueSize = DEFAULT_DEBUG_SERIAL_QUEUE_SIZE;
static size_t messageLength = DEFAULT_DEBUG_SERIAL_MESSAGE_MAX_LENGTH;

struct DebugMessage
{
    char *message;
};

static void serialTask(void *pvParameters)
{
    DebugMessage debugMessage;

    while (true)
    {
        if (xQueueReceive(serialQueue, &debugMessage, portMAX_DELAY))
        {
            if (debugMessage.message != nullptr)
            {
                unsigned long inicio = micros();
                if (strlen(debugMessage.message) == 0 || strcmp(debugMessage.message, " ") == 0)
                {
                    Serial.println();
                }
                else
                {
                    Serial.print(debugMessage.message);
                    unsigned long fim = micros();
                    unsigned long tempoGasto = fim - inicio;
                    size_t mensagensPendentes = uxQueueMessagesWaiting(serialQueue);

                    Serial.printf(" [%lu µs | Pendentes: %d]\n", tempoGasto, mensagensPendentes);
                }

                delete[] debugMessage.message;
            }
        }
    }
}

void initializeDebugSerial(int debugSerial, size_t userQueueSize, size_t userMessageLength)
{
    debugEnabled = debugSerial;
    queueSize = userQueueSize;
    messageLength = userMessageLength > 0 ? userMessageLength : DEFAULT_DEBUG_SERIAL_MESSAGE_MAX_LENGTH;

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

void dlog(const char *format, ...)
{
    if (!debugEnabled)
        return;

    DebugMessage debugMessage;
    debugMessage.message = new char[messageLength];

    if (debugMessage.message == nullptr)
    {
        Serial.println("Erro ao alocar memória para mensagem de log.");
        return;
    }

    char caller[32]; // Expandindo para suportar nomes de função maiores
    strncpy(caller, __FUNCTION__, sizeof(caller) - 1);
    caller[sizeof(caller) - 1] = '\0';

    unsigned long timestamp = micros();

    char formattedMessage[messageLength];
    int snprintfResult = snprintf(formattedMessage, messageLength, "[%lu] [%s] ", timestamp, caller);
    if (snprintfResult < 0 || snprintfResult >= messageLength)
    {
        Serial.println("Erro ao formatar a mensagem inicial de log.");
        delete[] debugMessage.message;
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(formattedMessage + strlen(formattedMessage), messageLength - strlen(formattedMessage), format, args);
    va_end(args);

    strncpy(debugMessage.message, formattedMessage, messageLength - 1);
    debugMessage.message[messageLength - 1] = '\0';

    if (serialQueue != NULL)
    {
        if (xQueueSendToBack(serialQueue, &debugMessage, pdMS_TO_TICKS(100)) != pdPASS)
        {
            Serial.println("Fila de debug cheia. Mensagem descartada.");
            delete[] debugMessage.message;
        }
    }
}

void dlog(int value)
{
    dlog("Valor: %d", value);
}

void dlog(unsigned int value)
{
    dlog("Valor: %u", value);
}

void dlog(long value)
{
    dlog("Valor: %ld", value);
}

void dlog(unsigned long value)
{
    dlog("Valor: %lu", value);
}

void dlog(float value)
{
    dlog("Valor: %.2f", value);
}

void dlog(double value)
{
    dlog("Valor: %.4lf", value);
}

void dlog(bool value)
{
    dlog("Valor: %s", value ? "true" : "false");
}

void dlog(char value)
{
    dlog("Valor: %c", value);
}

void dlog(float value, int decimalPlaces)
{
    char buffer[DEFAULT_DEBUG_SERIAL_MESSAGE_MAX_LENGTH];
    snprintf(buffer, sizeof(buffer), "%.*f", decimalPlaces, value);
    dlog(buffer);
}

void dlog(const String &value)
{
    dlog("Valor: %s", value.c_str());
}

static QueueHandle_t serial2Queue;
static int debugEnabledSerial2 = DEFAULT_DEBUG_SERIAL2;
static size_t queue2Size = DEFAULT_DEBUG_SERIAL2_QUEUE_SIZE;
static size_t message2Length = DEFAULT_DEBUG_SERIAL2_MESSAGE_MAX_LENGTH;

struct DebugMessage2
{
    char *message;
};

static void serial2Task(void *pvParameters)
{
    DebugMessage2 debugMessage;

    while (true)
    {
        if (xQueueReceive(serial2Queue, &debugMessage, portMAX_DELAY))
        {
            if (debugMessage.message != nullptr)
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
}

void initializeDebugSerial2(int debugSerial, size_t queueSize, size_t messageLength)
{
    debugEnabledSerial2 = debugSerial;
    queue2Size = queueSize;
    message2Length = messageLength > 0 ? messageLength : DEFAULT_DEBUG_SERIAL2_MESSAGE_MAX_LENGTH;

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

void dlog2(const char *format, ...)
{
    if (!debugEnabledSerial2)
        return;

    DebugMessage2 debugMessage;
    debugMessage.message = new char[message2Length];

    if (debugMessage.message == nullptr)
    {
        Serial2.println("Erro ao alocar memória para mensagem de log.");
        return;
    }

    char caller[32]; // Expandindo para suportar nomes de função maiores
    strncpy(caller, __FUNCTION__, sizeof(caller) - 1);
    caller[sizeof(caller) - 1] = '\0';

    unsigned long timestamp = micros();

    char formattedMessage[message2Length];
    int snprintfResult = snprintf(formattedMessage, message2Length, "[%lu] [%s] ", timestamp, caller);
    if (snprintfResult < 0 || snprintfResult >= message2Length)
    {
        Serial2.println("Erro ao formatar a mensagem inicial de log.");
        delete[] debugMessage.message;
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(formattedMessage + strlen(formattedMessage), message2Length - strlen(formattedMessage), format, args);
    va_end(args);

    strncpy(debugMessage.message, formattedMessage, message2Length - 1);
    debugMessage.message[message2Length - 1] = '\0';

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
    char buffer[DEFAULT_DEBUG_SERIAL2_MESSAGE_MAX_LENGTH];
    snprintf(buffer, sizeof(buffer), "%.*f", decimalPlaces, value);
    dlog2(buffer);
}

void dlog2(const String &value)
{
    dlog2("Valor: %s", value.c_str());
}
