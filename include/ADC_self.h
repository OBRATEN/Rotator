/* ADC_self.h */

#include <avr/io.h>

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
        ADMUX = (ADMUX & 0xF8) | m_channel1;
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC)); // Пока не получим данные -- ожидаем
        return ADC; // Возврат данных
    }

    int getY(void){
        m_channel1 &= 0x7;
        ADMUX = (ADMUX & 0xF8) | m_channel0;
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
