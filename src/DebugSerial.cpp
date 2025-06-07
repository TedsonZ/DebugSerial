#include "DebugSerial.h"

static QueueHandle_t serialQueue;
static int debugEnabled = DEFAULT_DEBUG_SERIAL;
static size_t queueSize = DEFAULT_DEBUG_SERIAL_QUEUE_SIZE;
static size_t messageLength = DEFAULT_DEBUG_SERIAL_MESSAGE_MAX_LENGTH;

static QueueHandle_t serial1Queue;
static int debugEnabledSerial1 = DEFAULT_DEBUG_SERIAL1;
static size_t queue1Size = DEFAULT_DEBUG_SERIAL1_QUEUE_SIZE;
static size_t message1Length = DEFAULT_DEBUG_SERIAL1_MESSAGE_MAX_LENGTH;

static QueueHandle_t serial2Queue;
static int debugEnabledSerial2 = DEFAULT_DEBUG_SERIAL2;
static size_t queue2Size = DEFAULT_DEBUG_SERIAL2_QUEUE_SIZE;
static size_t message2Length = DEFAULT_DEBUG_SERIAL2_MESSAGE_MAX_LENGTH;

struct DebugMessage
{
    char *message;         // Usado para strings
    bool isBinary = false; // Indica se a mensagem é binária
    const void *data;      // Ponteiro para dados binários
    size_t size;           // Tamanho dos dados binários
};

// Fila para armazenar os dados recebidos da Serial2
QueueHandle_t serial0ReceptionQueue;
QueueHandle_t serial1ReceptionQueue;
QueueHandle_t serial2ReceptionQueue;

// Função de tarefa para processar dados da Serial1
void serial1ReceptionTask(void *pvParameters)
{
    static bool inSync = false;
    static uint8_t buffer[130]; // Tamanho máximo baseado no struct
    static size_t bytesReceived = 0;

    while (true)
    {
        while (Serial1.available() > 0)
        {
            uint8_t byte = Serial1.read();

            if (byte == 0x7E) // Marcador de início
            {
                inSync = true;
                bytesReceived = 0;
                dlog("Serial1: Marcador de início detectado.");
                continue;
            }

            if (inSync)
            {
                if (byte == 0x7F) // Marcador de fim
                {
                    if (bytesReceived >= 122) // Supondo struct com 122 bytes
                    {
                        // Enviar cópia dos dados para a fila
                        if (xQueueSendToBack(serial1ReceptionQueue, buffer, pdMS_TO_TICKS(100)) != pdPASS)
                        {
                            dlog("Fila Serial1 ; DESCARTADA ; Tamanho: %zu bytes", bytesReceived);
                        }
                        else
                        {
                            dlog("Fila Serial1 ; ARMAZENADA ; Tamanho: %zu bytes", bytesReceived);
                        }
                        memset(buffer, 0, sizeof(buffer));
                        bytesReceived = 0;
                    }
                    else
                    {
                        dlog("Serial1: Dados insuficientes antes do marcador de fim.");
                    }
                    inSync = false;
                    continue;
                }

                if (bytesReceived < sizeof(buffer))
                {
                    buffer[bytesReceived++] = byte;
                }
                else
                {
                    dlog("Serial1: Erro - Buffer cheio antes do fim.");
                    inSync = false;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


// Função de tarefa para processar dados da Serial2
void serial2ReceptionTask(void *pvParameters)
{
    static bool inSync = false;
    static uint8_t buffer[130]; // Tamanho máximo de dados recebidos ja que statusDeste tem 122 bytes
    static size_t bytesReceived = 0;

    while (true)
    {
        while (Serial2.available() > 0)
        {
            uint8_t byte = Serial2.read();

            if (byte == 0x7E) // Marcador de início
            {
                inSync = true;
                bytesReceived = 0;
                dlog("Marcador de início detectado.");
                continue;
            }

            if (inSync)
            {
                if (byte == 0x7F) // Marcador de fim
                {
                    if (bytesReceived >= 122) // Tamanho lido do struct statusDeste 28/01/2025
                    {
                        // Enviar os dados para a fila
                        if (xQueueSendToBack(serial2ReceptionQueue, buffer, pdMS_TO_TICKS(100)) != pdPASS)
                        {
                            dlog("Fila Serial2 ; DESCARTADA ; Tamanho: %zu bytes", bytesReceived);
                        }
                        else
                        {
                            dlog("Fila Serial2 ; ARMAZENADA ; Tamanho: %zu bytes", bytesReceived);
                        }
                        memset(buffer, 0, sizeof(buffer));
                        bytesReceived = 0;
                    }
                    else
                    {
                        dlog("Nenhum dado recebido antes do marcador de fim.");
                    }
                    inSync = false;
                    continue;
                }

                // Gravar o byte no buffer se não for o terminador
                if (bytesReceived < sizeof(buffer))
                {
                    buffer[bytesReceived++] = byte;
                }
                else
                {
                    dlog("Erro: Buffer cheio antes do marcador de fim.");
                    inSync = false;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
bool getSerialData(char *buffer, size_t bufferSize)
{
    return xQueueReceive(serial0ReceptionQueue, buffer, 0) == pdPASS;
}

void serialReceptionTask(void *pvParameters)
{
    static char receivedData[128];
    static size_t index = 0;
    while (true)
    {
        while (Serial.available() > 0)
        {
            char c = Serial.read();
            if (c == '\n' || index >= sizeof(receivedData) - 1) // Fim da linha ou buffer cheio
            {
                receivedData[index] = '\0';
                xQueueSend(serial0ReceptionQueue, receivedData, portMAX_DELAY);
                index = 0;
            }
            else
            {
                receivedData[index++] = c;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void initializeSerialReceptionTask()
{
    serial0ReceptionQueue = xQueueCreate(1, sizeof(char) * 32);
    xTaskCreate(serialReceptionTask, "SerialReceptionTask", 6096, NULL, 1, NULL);
}

// Inicializa a tarefa de recepção da Serial2
void initializeSerial2ReceptionTask(size_t queueSize, size_t bufferSize, TaskHandle_t* outHandle)
{
    serial2ReceptionQueue = xQueueCreate(queueSize, bufferSize);
    if (serial2ReceptionQueue == NULL)
    {
        dlog("Erro ao criar fila para recepção da Serial2.");
        return;
    }

    TaskHandle_t tempHandle = NULL;
    xTaskCreate(serial2ReceptionTask, "Serial2ReceptionTask", 6096, NULL, 1, &tempHandle);

    if (outHandle != nullptr)
    {
        *outHandle = tempHandle;
    }

    dlog("Tarefa de recepção da Serial2 inicializada.");
}

// Obtém dados estruturados da fila de recepção da Serial1
bool getSerial1Struct(void *data, size_t dataSize, TickType_t timeout)
{
    if (serial1ReceptionQueue == NULL)
        return false;

    uint8_t buffer[dataSize];
    if (xQueueReceive(serial1ReceptionQueue, buffer, timeout) == pdPASS)
    {
        memcpy(data, buffer, dataSize);
        return true;
    }

    return false;
}

// Obtém dados estruturados da fila de recepção da Serial2
bool getSerial2Struct(void *data, size_t dataSize, TickType_t timeout)
{
    if (serial2ReceptionQueue == NULL)
        return false;

    uint8_t buffer[dataSize];
    if (xQueueReceive(serial2ReceptionQueue, buffer, timeout) == pdPASS)
    {
        memcpy(data, buffer, dataSize);
        return true;
    }

    return false;
}

static void serialTask(void *pvParameters)
{
    DebugMessage debugMessage;

    while (true)
    {
        if (xQueueReceive(serialQueue, &debugMessage, portMAX_DELAY))
        {
            if (debugMessage.message != nullptr)
            {
                if (strlen(debugMessage.message) == 0 || strcmp(debugMessage.message, "") == 0)
                {
                    Serial.println();
                }
                else
                {
                    Serial.print(debugMessage.message);
                    Serial.println();
                }

                delete[] debugMessage.message;
            }
        }
    }
}

static void serial1Task(void *pvParameters)
{
    DebugMessage debugMessage;

    while (true)
    {
        if (xQueueReceive(serial1Queue, &debugMessage, portMAX_DELAY))
        {
            if (debugMessage.message != nullptr)
            {
                if (debugMessage.isBinary)
                {
                    Serial1.write(static_cast<const uint8_t *>(debugMessage.data), debugMessage.size);
                }
                else
                {
                    unsigned long inicio = micros();
                    if (strlen(debugMessage.message) == 0 || strcmp(debugMessage.message, " ") == 0)
                    {
                        Serial1.println();
                    }
                    else
                    {
                        Serial1.print(debugMessage.message);
                    }
                    unsigned long fim = micros();
                    dlog(" [%lu µs | Pendentes: %d]\n", fim - inicio, uxQueueMessagesWaiting(serial1Queue));
                }

                delete[] debugMessage.message;
            }
        }
    }
}

//
// Tarefa responsável por processar as mensagens de log da Serial2
//
// A tarefa verifica se há mensagens na fila serial2Queue e, se sim,
// as processa. Se a mensagem for binária, ela é escrita na Serial2
// usando o método write. Caso contrário, a mensagem é impressa na
// Serial2 como texto. Em ambos os casos, o tempo de processamento é
// medido e impresso, juntamente com o número de mensagens pendentes
// na fila serial2Queue.
//
static void serial2Task(void *pvParameters)
{
    while (true)
    {
        if (uxQueueMessagesWaiting(serial2Queue) > 0)
        {
            DebugMessage debugMessage;

            if (xQueueReceive(serial2Queue, &debugMessage, portMAX_DELAY))
            {
                if (debugMessage.message != nullptr)
                {
                    // Verifique se a mensagem é binária
                    if (debugMessage.isBinary)
                    {
                        Serial2.write(static_cast<const uint8_t *>(debugMessage.data), debugMessage.size);
                    }
                    else
                    {
                        unsigned long inicio = micros();
                        if (strlen(debugMessage.message) == 0 || strcmp(debugMessage.message, " ") == 0)
                        {
                            Serial2.println();
                        }
                        else
                        {
                            Serial2.print(debugMessage.message);
                        }
                        unsigned long fim = micros();
                        dlog(" [%lu µs | Pendentes: %d]\n", fim - inicio, uxQueueMessagesWaiting(serial2Queue));
                    }

                    delete[] debugMessage.message;
                }
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
        xTaskCreate(serialTask, "SerialTask", 5048, NULL, 1, NULL);
        dlog("Debug Serial inicializado.");
    }
}

void initializeDebugSerial1(int debugSerial, size_t queueSize, size_t messageLength)
{
    debugEnabledSerial1 = debugSerial;
    queue1Size = queueSize;
    message1Length = messageLength > 0 ? messageLength : DEFAULT_DEBUG_SERIAL1_MESSAGE_MAX_LENGTH;

    if (debugEnabledSerial1)
    {
        serial1Queue = xQueueCreate(queueSize, sizeof(DebugMessage));
        if (serial1Queue == NULL)
        {
            dlog("Erro ao criar fila para Serial1.");
            return;
        }
        xTaskCreate(serial1Task, "Serial1Task", 5048, NULL, 1, NULL);
        dlog("Debug Serial1 inicializado.");
    }
}

void initializeDebugSerial2(int debugSerial, size_t queueSize, size_t messageLength)
{
    debugEnabledSerial2 = debugSerial;
    queue2Size = queueSize;
    message2Length = messageLength > 0 ? messageLength : DEFAULT_DEBUG_SERIAL2_MESSAGE_MAX_LENGTH;

    if (debugEnabledSerial2)
    {
        serial2Queue = xQueueCreate(queueSize, sizeof(DebugMessage));
        if (serial2Queue == NULL)
        {
            dlog("Erro ao criar fila para Serial2.");
            return;
        }
        xTaskCreate(serial2Task, "Serial2Task", 5048, NULL, 1, NULL);
        dlog("Debug Serial2 inicializado.");
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
        // Serial.println("Erro ao alocar memória para mensagem de log.");
        return;
    }

    const char *taskName = pcTaskGetName(NULL);
    if (taskName == nullptr)
    {
        taskName = "Unknown";
    }

    char caller[16];
    strncpy(caller, taskName, 15); // Copia até 15 caracteres de taskName
    caller[15] = '\0';             // Garante que o último caractere seja o terminador de string

    // Se taskName tiver menos que 15 caracteres, preenche o restante com espaços
    if (strlen(taskName) < 15)
    {
        for (int i = strlen(taskName); i < 15; i++)
        {
            caller[i] = ' '; // Preenche com espaço
        }
    }

    unsigned long timestamp = micros();

    char formattedMessage[messageLength];
    formattedMessage[0] = '\0';

    va_list args;
    va_start(args, format);
    vsnprintf(formattedMessage, messageLength, format, args);
    va_end(args);

    if (strlen(formattedMessage) == 0)
    {
        snprintf(debugMessage.message, messageLength, "\n");
    }
    else
    {
        char temp[messageLength];
        snprintf(temp, messageLength, "%lu;%s;%s", timestamp, caller, formattedMessage);
        strncpy(debugMessage.message, temp, messageLength - 1);
    }

    debugMessage.message[messageLength - 1] = '\0';

    if (serialQueue != NULL)
    {
        if (xQueueSendToBack(serialQueue, &debugMessage, pdMS_TO_TICKS(100)) != pdPASS)
        {
            Serial.println("Fila de debug cheia. Mensagem descartada.");
            delete[] debugMessage.message;
        }
    }
    else
    {
        delete[] debugMessage.message;
        Serial.println("serialQueue não está inicializado.");
    }
}

void dlog1(const char *format, ...)
{
    if (!debugEnabledSerial1)
        return;

    DebugMessage debugMessage;
    debugMessage.message = new char[message1Length];

    if (debugMessage.message == nullptr)
    {
        Serial1.println("Erro ao alocar memória para mensagem de log.");
        return;
    }

    const char *taskName = pcTaskGetName(NULL);
    if (taskName == nullptr)
    {
        taskName = "Unknown";
    }

    char caller[16];
    strncpy(caller, taskName, 15);
    caller[15] = '\0';

    unsigned long timestamp = micros();

    char formattedMessage[message1Length];
    formattedMessage[0] = '\0';

    va_list args;
    va_start(args, format);
    vsnprintf(formattedMessage, message1Length, format, args);
    va_end(args);

    if (strlen(formattedMessage) == 0)
    {
        snprintf(debugMessage.message, message1Length, "\n");
    }
    else
    {
        char temp[message1Length];
        snprintf(temp, message1Length, "%lu;%s;%s", timestamp, caller, formattedMessage);
        strncpy(debugMessage.message, temp, message1Length - 1);
    }

    debugMessage.message[message1Length - 1] = '\0';

    if (serial1Queue != NULL)
    {
        if (xQueueSendToBack(serial1Queue, &debugMessage, pdMS_TO_TICKS(100)) != pdPASS)
        {
            Serial1.println("Fila de debug Serial1 cheia. Mensagem descartada.");
            delete[] debugMessage.message;
        }
    }
    else
    {
        Serial1.println("serial1Queue não está inicializada.");
        delete[] debugMessage.message; // Garantir que a memória é liberada.
    }
}

// Sobrecargas para dlog
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

// Sobrecargas para dlog1
void dlog1(int value)
{
    dlog1("Valor: %d", value);
}

void dlog1(unsigned int value)
{
    dlog1("Valor: %u", value);
}

void dlog1(long value)
{
    dlog1("Valor: %ld", value);
}

void dlog1(unsigned long value)
{
    dlog1("Valor: %lu", value);
}

void dlog1(float value)
{
    dlog1("Valor: %.2f", value);
}
void dlog1(double value)
{
    dlog1("Valor: %.4lf", value);
}
void dlog1(bool value)
{
    dlog1("Valor: %s", value ? "true" : "false");
}

void dlog1(char value)
{
    dlog1("Valor: %c", value);
}
void dlog1(float value, int decimalPlaces)
{
    char buffer[DEFAULT_DEBUG_SERIAL1_MESSAGE_MAX_LENGTH];
    snprintf(buffer, sizeof(buffer), "%.*f", decimalPlaces, value);
    dlog1(buffer);
}
void dlog1(const String &value)
{
    dlog1("Valor: %s", value.c_str());
}
void dlog1Binary(const void *data, size_t dataSize)
{
    if (!debugEnabled || data == nullptr || dataSize == 0)
        return;

    struct BinaryDebugMessage
    {
        const void *data;
        size_t size;
    };

    BinaryDebugMessage debugMessage = {data, dataSize};

    if (serialQueue != NULL)
    {
        if (xQueueSendToBack(serialQueue, &debugMessage, pdMS_TO_TICKS(100)) != pdPASS)
        {
            Serial.println("Fila de debug Serial1 cheia. Mensagem binária descartada.");
        }
    }
}

// Sobrecargas para dlog2

void dlog2(const char *format, ...)
{
    if (!debugEnabledSerial2)
        return;

    DebugMessage debugMessage;
    debugMessage.message = new char[message2Length];

    if (debugMessage.message == nullptr)
    {
        Serial2.println("Erro ao alocar memória para mensagem de log.");
        return;
    }

    const char *taskName = pcTaskGetName(NULL);
    if (taskName == nullptr)
    {
        taskName = "Unknown";
    }

    char caller[16];
    strncpy(caller, taskName, 15);
    caller[15] = '\0';

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
    else
    {
        Serial2.println("Fila de debug Serial2 não inicializada. Mensagem descartada.");
        delete[] debugMessage.message; // Garantir que a memória é liberada.
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

void dlog2Binary(const void *data, size_t dataSize)
{
    if (!debugEnabledSerial2 || data == nullptr || dataSize == 0)
        return;

    struct BinaryDebugMessage
    {
        const void *data;
        size_t size;
    };

    BinaryDebugMessage debugMessage = {data, dataSize};

    if (serial2Queue != NULL)
    {
        if (xQueueSendToBack(serial2Queue, &debugMessage, pdMS_TO_TICKS(100)) != pdPASS)
        {
            Serial2.println("Fila de debug Serial2 cheia. Mensagem binária descartada.");
        }
    }
}
