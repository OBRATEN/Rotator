/*
  Team: Аномально fat cocks-1
  Project: Rotator
  Broad: atmega328P/Arduino Nano
  F_CPU: 16MHz
*/

// Светодиоды индикации
#define LEDU PIND2
#define LEDA PIND3
#define LEDP PIND4

// Пины управления
#define AZR PINB5
#define AZL PINB4
#define ELR PINB3
#define ELL PINB2

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h> // Для itoa и atoi

// Классы для считывания

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
    uint16_t m_ubrr; // Скорость порта
    bool m_capacity {false}; // Булиновая состояния
};

UART_self uart(103);

class ADC_self {
  public:
    ADC_self (uint8_t channel0, uint8_t channel1) {
      // Включаем АЦП
      ADMUX = (1 << REFS0);
      ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
      // Устанавливаем используемые каналы
      m_channel0 = channel0;
      m_channel1 = channel1;
      // Переменная состояния
      m_capacity = true;
    }

    int getX(void){
        m_channel0 &= 0x7;
        ADMUX = (ADMUX & 0xF8) | m_channel0;
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC)); // Пока не получим данные -- ожидаем
        return ADC; // Возврат данных
    }

    int getY(void){
        m_channel1 &= 0x7;
        ADMUX = (ADMUX & 0xF8) | m_channel1;
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC)); // Пока не получим данные -- ожидаем
        return ADC; // Возврат данных
    }

    bool getCapacity(void) {
      return m_capacity; // Возврат состояния
    }

  private:
    // Пины АЦП
    uint8_t m_channel0;
    uint8_t m_channel1;
    // Булиновая состояния
    bool m_capacity {false};
};

ADC_self adc(0, 1);

// Классы для работы

class Additional {
  public:
    void parseString(char *data, int* x, int* y) {
      // Буфферы для чисел
      char buf0[32];
      char buf1[32];
      bool toSecond {false}; // Булиновая перехода от buf0 к buf1
      uint8_t q {0}; // Индекс конца первого буффера
      for (unsigned short int i {0}; i < 32; i++) { // Цикл по принятому массиву
        switch (data[i]) {
          /* Стоп-символ первого буффера, обрезается buf0, переходит к buf1,
             Записывается переменная индекса конца buf0 */
          case ',': buf0[i] = 0; toSecond = true; q = i; break;
          // Стоп-символ второго буффера, брейк цикла for
          case ';': buf1[i - q] = 0; i = 32; break;
          /* Если не перешло ко второму, то добавляем символ к buf0,
             Иначе добавляем к bu1 */
          default: if (toSecond) buf1[i - q - 1] = data[i]; else buf0[i] = data[i];
        }
      }
      uart.putChar('\n'); // Перевод строки
      /* Преобразование массива символов в число
         по средствам библиотеки stdlib */
      *x = atoi(buf0); *y = atoi(buf1);
    }

    double scaleVal(double val, double l1, double h1, double l2, double h2) {
      /* Формула перевода между диапазонами
         l1 и h1 -- начальный диапазон,
         l2 и h2 -- конечный диапазон
      */
      return (val - l1) / (h1 - l1) * (h2 - l2) + l2;
    }
};

class Rotator {
  public:
    Rotator (void) {
      // Пины на вывод
      DDRB |= (1 << AZL);
      DDRB |= (1 << AZR);
      DDRB |= (1 << ELL);
      DDRB |= (1 << ELR);
      // Чтение данных АЦП
      m_currentX = adc.getX();
      m_currentY = adc.getY();
    }

    void putUART(int x, int y) {
      // Подача нужных координат в класс
      m_needX = x;
      m_needY = y;
    }

    // Включение
    void AzimutLeftOn(void) {
      PORTB |= (1 << AZL);
    }

    void AzimutRightOn(void) {
      PORTB |= (1 << AZR);
    }

    void ElevationDownOn(void) {
      PORTB |= (1 << ELL);
    }

    void ElevationUpOn(void) {
      PORTB |= (1 << ELR);
    }

    // Выключение

    void AzimutOff(void) {
      PORTB &= ~(1 << AZL);
      PORTB &= ~(1 << AZR);
    }

    void ElevationOff(void) {
      PORTB &= ~(1 << ELL);
      PORTB &= ~(1 << ELR);
    }

    void rotateX(void) {
      if (m_needX < m_currentX) {
        AzimutLeftOn();
      } else AzimutOff();
      if (m_needX > m_currentX) {
        AzimutRightOn();
      } else AzimutOff();
      m_currentX = adc.getX();
    }

    void rotateY(void) {
        // Поворот относительно координат
        if (m_needY < m_currentY) {
          ElevationDownOn();
        } else ElevationOff();
        if (m_needY > m_currentY) {
          ElevationUpOn();
        } else ElevationOff();
        // Получение новых данных с АЦП
        m_currentY = adc.getY();
    }

    void rotate(void) {
      bool xb {false};
      bool yb {false};
      while (1) {
        if (m_needX != m_currentX && !xb) rotateX(); else {
          xb = true;
          AzimutOff();
        }
        if (m_needY != m_currentY && !yb) rotateY(); else {
          yb = true;
          ElevationOff();
        }
        char xc[32]; char yc[32];
        char ux[32]; char uy[32];
        itoa(m_currentX, xc, 10); itoa(m_currentY, yc, 10);
        itoa(m_needX, ux, 10); itoa(m_needY, uy, 10);
        uart.putString(ux); uart.putChar('\t');
        uart.putString(uy); uart.putChar('\t');
        uart.putString(xc); uart.putChar('\t');
        uart.putString(yc); uart.putChar('\n');
        if ((m_needX == m_currentX) && (m_needY == m_currentY)) break;
        if ((m_currentX < 0) || (m_currentX > 1024)) {
          AzimutOff();
          ElevationOff();
          break;
        }
        if ((m_currentY < 0) || (m_currentY > 1024)) {
          AzimutOff();
          ElevationOff();
          break;
        }
      }
      AzimutOff();
      ElevationOff();
      uart.putString(m_done);
    }
  private:
    char m_done[16] = {"Done!"};
    int m_needX;
    int m_needY;
    int m_currentX;
    int m_currentY;
};

class LED_self {
  public:
    LED_self (bool uart, bool adc) {
      DDRD |= (1 << LEDP); // Пин питация на вывод
      if (uart) {
        // Если юарт работает, то его пин светодиода на вывод
        DDRD |= (1 << LEDU);
        UARTactive_m = uart;
      }
      if (adc) {
        // Если АПЦ работает, то пин его светодиода на вывод
        DDRD |= (1 << LEDA);
        ADCactive_m = adc;
      }
    }

    void checkActivity(void) {
      PORTD |= (1 << LEDP); // Включение светодиода питания
      // Если активен ЮАРТ, то включаем его светодиод
      if (UARTactive_m) PORTD |= (1 << LEDU);
      // Если активен АЦП, то включаем его светодиод
      if (ADCactive_m) PORTD |= (1 << LEDA);
    }

    bool getUARTactivity(void) {
      return UARTactive_m; // Возврат состояния ЮАРТ
    }

    bool getADCactivity(void) {
      return ADCactive_m; // Возврат состояния АЦП
    }

  private:
    // Булиновые состояния ЮАРТа и АЦП
    bool UARTactive_m {false};
    bool ADCactive_m {false};
};

LED_self led(uart.getCapacity(), adc.getCapacity());
Additional addfs;
Rotator rot;

int main() {
  while (1) {
    led.checkActivity();
    char buf[32];
    int x {0}; int y {0};
    char xc[32]; char yc[32];
    y = adc.getY();
    x = adc.getX();
    itoa(x, xc, 10); itoa(y, yc, 10);
    uart.putString(xc); uart.putChar('\t');
    uart.putString(yc); uart.putChar('\n');
    char* data = uart.getString(buf);
    uart.putString(data);
    uart.putChar('\n');
    addfs.parseString(data, &x, &y);
    itoa(x, xc, 10); itoa(y, yc, 10);
    uart.putString(xc); uart.putChar('\t');
    uart.putString(yc); uart.putChar('\n');
    x = addfs.scaleVal(x, 0, 450, 0, 1024);
    y = addfs.scaleVal(y, 0, 180, 0, 1024);
    rot.putUART(x, y);
    itoa(x, xc, 10); itoa(y, yc, 10);
    uart.putString(xc); uart.putChar('\t');
    uart.putString(yc); uart.putChar('\n');
    rot.rotate();
  }
}
