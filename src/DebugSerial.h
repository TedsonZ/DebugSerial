#ifndef DEBUG_SERIAL_H
#define DEBUG_SERIAL_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// Configurações padrão da biblioteca para Serial1
#define DEFAULT_DEBUG_SERIAL 1
#define DEFAULT_DEBUG_SERIAL_QUEUE_SIZE 10
#define DEFAULT_DEBUG_SERIAL_MESSAGE_MAX_LENGTH 128

// Configurações padrão da biblioteca para Serial2
#define DEFAULT_DEBUG_SERIAL2 1
#define DEFAULT_DEBUG_SERIAL2_QUEUE_SIZE 15
#define DEFAULT_DEBUG_SERIAL2_MESSAGE_MAX_LENGTH 256

#define TAMANHO_DO_STRUCT 122 // Tamanho do struct statusDeste 28/01/2025

// Funções de inicialização
void initializeDebugSerial(
    int debugSerial = DEFAULT_DEBUG_SERIAL,
    size_t queueSize = DEFAULT_DEBUG_SERIAL_QUEUE_SIZE,
    size_t messageLength = DEFAULT_DEBUG_SERIAL_MESSAGE_MAX_LENGTH);

void initializeDebugSerial2(
    int debugSerial2 = DEFAULT_DEBUG_SERIAL2,
    size_t queueSize = DEFAULT_DEBUG_SERIAL2_QUEUE_SIZE,
    size_t messageLength = DEFAULT_DEBUG_SERIAL2_MESSAGE_MAX_LENGTH);

// Funções de log para Serial1
void dlog(const char *format, ...);
void dlog(int value);
void dlog(unsigned int value);
void dlog(long value);
void dlog(unsigned long value);
void dlog(float value);
void dlog(double value);
void dlog(bool value);
void dlog(char value);
void dlog(float value, int decimalPlaces);
void dlog(const String &value);

// Funções de log para Serial2
void dlog2(const char *format, ...);
void dlog2(int value);
void dlog2(unsigned int value);
void dlog2(long value);
void dlog2(unsigned long value);
void dlog2(float value);
void dlog2(double value);
void dlog2(bool value);
void dlog2(char value);
void dlog2(float value, int decimalPlaces);
void dlog2(const String &value);
void dlog2Binary(const void *data, size_t dataSize);

template <typename T>
void dlog2Struct(const T &data)
{
    if (!DEFAULT_DEBUG_SERIAL2)
        return;

    // Serializar a estrutura em um buffer de bytes
    const size_t dataSize = sizeof(T);
    if (dataSize > DEFAULT_DEBUG_SERIAL2_MESSAGE_MAX_LENGTH)
    {
        // Serial2.println("Erro: Tamanho da estrutura excede o limite de mensagem.");
        return;
    }

    uint8_t buffer[dataSize + 2]; // Inclui espaço para header e terminador
    buffer[0] = 0x7E;             // Header (marcador de início)
    memcpy(&buffer[1], &data, dataSize);
    buffer[dataSize + 1] = 0x7F; // Terminador (marcador de fim)

    // Enviar os bytes pela Serial2
    Serial2.write(buffer, dataSize + 2);
}
// Inicialização da tarefa de recepção da Serial2
void initializeSerial2ReceptionTask(size_t queueSize, size_t bufferSize);

// Função para obter dados da fila de recepção
bool getSerial2Struct(void *data, size_t dataSize, TickType_t timeout);

// Função da tarefa de recepção (internamente usada pela inicialização)
void serial2ReceptionTask(void *pvParameters);

#endif // DEBUG_SERIAL_H
