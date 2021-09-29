/*
  Team: Аномально fat cocks-1
  Project: Rotator
  Broad: atmega328P/Arduino Nano
  F_CPU: 16MHz
*/

// Библиотеки
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>

// Самописки
#include "ADC_self.h"
#include "UART_self.h"
#include "LED_self.h"

// Пины управления
#define AZR PINB2
#define AZL PINB1
#define ELR PINB3
#define ELL PINB4

ADC_self adc(0, 1);
UART_self uart(103);
LED_self led(uart.getCapacity(), adc.getCapacity());

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
      // Подача нужных координат в приватные переменные класса
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
      if (m_needX < m_currentX) AzimutLeftOn();
      else if (m_needX > m_currentX) AzimutRightOn();
      else AzimutOff();
      m_currentX = adc.getX();
    }

    void rotateY(void) {
        // Поворот относительно координат
        if (m_needY < m_currentY) ElevationDownOn();
        else if (m_needY > m_currentY) ElevationUpOn();
        else ElevationOff();
        // Получение новых данных с АЦП
        m_currentY = adc.getY();
    }

    void getUartAgen(void) {
      char buf[32];
      int x {0}; int y {0};
      char* data = uart.getString(buf);
      uart.putString(data);
      uart.putChar('\n');
      parseString(data, &x, &y);
      x = scaleVal(x, 0, 450, 0, 875); //875
      y = scaleVal(y, 0, 180, 0, 530); //530
      m_needX = x; m_needY = y;
    }

    void rotate(void) {
      bool xb {false};
      bool yb {false};
      if (m_needX < 0) xb = 1;
      else xb = 0;
      if (m_needY < 0) yb = 1;
      else yb = 0;
      while ((!xb) && (!yb)) {
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
        if ((m_currentX < 0) || (m_currentX > 875)) break;
        if ((m_currentY < 0) || (m_currentY > 530)) break;
        getUartAgen();
      }
      AzimutOff();
      ElevationOff();
      uart.putString(m_done);
    }
  private:
    char m_done[16] = {"Done!\n"};
    int m_needX;
    int m_needY;
    int m_currentX;
    int m_currentY;
};

double scaleVal(double val, double l1, double h1, double l2, double h2) {
  /* Формула перевода между диапазонами
     l1 и h1 -- начальный диапазон,
     l2 и h2 -- конечный диапазон
  */
  if (val == 0) return 0;
  else return (val - l1) / (h1 - l1) * (h2 - l2) + l2;
}

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
     библиотекой stdlib */
  *x = atoi(buf0); *y = atoi(buf1);
}

void putResData(char* xc, char* yc) {
  uart.putString(xc);
  uart.putChar('\t');
  uart.putString(yc);
  uart.putChar('\n');
}

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
    y = 0; x = 0;
    char* data = uart.getString(buf);
    uart.putString(data);
    uart.putChar('\n');
    parseString(data, &x, &y);
    itoa(x, xc, 10); itoa(y, yc, 10);
    uart.putString(xc); uart.putChar('\t');
    uart.putString(yc); uart.putChar('\n');
    x = scaleVal(x, 0, 450, 0, 875); //875
    y = scaleVal(y, 0, 180, 0, 530); //530
    rot.putUART(x, y);
    rot.rotate();
    delete [] data;
  }
}
