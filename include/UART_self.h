// UART_self.h

#include <avr/io.h>

class UART_self {
  public:
    UART_self (uint8_t ubrr) {
      // Задаём скорость порта (103 = 9600)
      UBRR0L = (uint8_t)(ubrr & 0xFF);
      UBRR0H = (uint8_t)(ubrr >> 8);
      // Включаем ресивер и трасмитор
      UCSR0B = (1 << RXEN0) | (1 << TXEN0);
      // Задаём формат данных (8 дата, 1 стоп)
      UCSR0C = (1 << USBS0) | (1 << 1) | (1 << 2);
      // Переменная работоспособности
      m_capacity = true;
    }

    void putChar(char data) {
      while (!(UCSR0A & (1 << UDRE0))); // Ожидание накопления буффера
      UDR0 = data; // Передача байта в буффер
    }

    void putString(char* data) {
      uint8_t i = 0;
      // Ожидание стоп-символа
      while ((data[i] != '\r') && (data[i] != '\n') && (data[i] != '\0')) {
        // Передача символа из массива
        putChar(data[i++]);
      }
    }

    char getChar(void) {
      while (!(UCSR0A & (1 << RXC0))); // Ожидание получения байта
      return UDR0; // Возврат байта
    }

    char* getString(char *buf) {
      uint8_t i {0};
      unsigned char c {'0'};
      // Цикл идёт до стоп-символа
      do {
        c = getChar(); // Получаем символ
        buf[i++] = c; // Добавляем символ в массив
      } while ((c != '\r') && (c != '\0') && (c != ';'));
      buf[i] = 0; // Обрезаем массив
      return buf; // Возвращаем массив
    }

    bool getCapacity(void) {
      return m_capacity; // Возвращаем состояние юарта
    }

  private:
    bool m_capacity {false}; // Булиновая состояния
};
