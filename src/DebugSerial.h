#ifndef DEBUG_SERIAL_H
#define DEBUG_SERIAL_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// Configurações padrão da biblioteca
#define DEFAULT_DEBUG_SERIAL 1
#define DEFAULT_SERIAL_QUEUE_SIZE 10
#define DEFAULT_SERIAL_MESSAGE_MAX_LENGTH 128

void initializeDebugSerial(
    int debugSerial = DEFAULT_DEBUG_SERIAL,
    size_t queueSize = DEFAULT_SERIAL_QUEUE_SIZE,
    size_t messageLength = DEFAULT_SERIAL_MESSAGE_MAX_LENGTH
);

void dlog(const char *format, ...);

// Sobrecargas para diferentes tipos de dados
void dlog(int value);
void dlog(unsigned int value);
void dlog(long value);
void dlog(unsigned long value);
void dlog(float value);
void dlog(double value);
void dlog(bool value);
void dlog(char value);
void dlog(const char *value);
void dlog(const String &value);

#endif // DEBUG_SERIAL_H
