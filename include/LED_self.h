/* LED_self.h */

#include <avr/io.h>

#define LEDA PINB0
#define LEDP PIND6
#define LEDU PIND7

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
        DDRB |= (1 << LEDA);
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
