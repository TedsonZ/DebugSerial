#ifndef DEBUG_SERIAL_H
#define DEBUG_SERIAL_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// Configurações padrão da biblioteca para Serial1
#define DEFAULT_DEBUG_SERIAL 1
#define DEFAULT_SERIAL_QUEUE_SIZE 10
#define DEFAULT_SERIAL_MESSAGE_MAX_LENGTH 128

// Configurações padrão da biblioteca para Serial2
#define DEFAULT_DEBUG_SERIAL2 1
#define DEFAULT_SERIAL2_QUEUE_SIZE 15
#define DEFAULT_SERIAL2_MESSAGE_MAX_LENGTH 256

// Funções de inicialização
void initializeDebugSerial(
    int debugSerial = DEFAULT_DEBUG_SERIAL,
    size_t queueSize = DEFAULT_SERIAL_QUEUE_SIZE,
    size_t messageLength = DEFAULT_SERIAL_MESSAGE_MAX_LENGTH);

void initializeDebugSerial2(
    int debugSerial2 = DEFAULT_DEBUG_SERIAL2,
    size_t queueSize = DEFAULT_SERIAL2_QUEUE_SIZE,
    size_t messageLength = DEFAULT_SERIAL2_MESSAGE_MAX_LENGTH);

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

#endif // DEBUG_SERIAL_H
