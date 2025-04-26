#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/WS2812.h"
#include "WS2812.pio.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28          // GPIO para o voltímetro
#define WS2812_PIN 7        // GPIO para a matriz de LEDs
#define BUTTON_A 5          // GPIO para botão A
#define BUTTON_B 6          // GPIO para botão B

int R_conhecido = 10000;    // Resistor de 10k ohm
float R_x = 0.0;            // Resistor desconhecido
float ADC_VREF = 3.31;      // Tensão de referência do ADC
int ADC_RESOLUTION = 4095;  // Resolução do ADC (12 bits)
ssd1306_t ssd;              // Estrutura do display

// Valores comerciais E24 (5% de tolerância)
float baseE24[] = {
  1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0,
  2.2, 2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.3,
  4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1
};

void btn_irq_handler(uint gpio, uint32_t events);
void setup_button(uint pin);
void setup_display();


int main() {
  setup_button(BUTTON_A);
  setup_button(BUTTON_B);
  gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &btn_irq_handler);

  setup_display();

  adc_init();
  adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

  
  // Inicializa o PIO para controlar a matriz de LEDs (WS2812)
  PIO pio = pio0;
  uint sm = 0;
  uint offset = pio_add_program(pio, &pio_matrix_program);
  pio_matrix_program_init(pio, sm, offset, WS2812_PIN);


  char str_x[5]; // Buffer para armazenar a string
  char str_y[5]; // Buffer para armazenar a string

  bool cor = true;
  while (true) {
    adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

    float soma_tensao = 0.0f;
    for (int i = 0; i < 500; i++) {
      soma_tensao += adc_read();
      sleep_ms(1);
    }
    float media = soma_tensao / 500.0f;
    //float media = 500;

    // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
    R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);

    sprintf(str_x, "%1.0f", media); // Converte o inteiro em string
    sprintf(str_y, "%1.0f", R_x);   // Converte o float em string
    
    printf("ADC: %1.0f\n", media);
    printf("R_x: %1.0f\n", R_x);
    printf("R_conhecido: %d\n", R_conhecido);

    // cor = !cor;
    //  Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor);                          // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
    ssd1306_line(&ssd, 3, 25, 123, 25, cor);           // Desenha uma linha
    ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // Desenha uma linha
    ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6); // Desenha uma string
    ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);  // Desenha uma string
    ssd1306_draw_string(&ssd, "  Ohmimetro", 10, 28);  // Desenha uma string
    ssd1306_draw_string(&ssd, "ADC", 13, 41);          // Desenha uma string
    ssd1306_draw_string(&ssd, "Resisten.", 50, 41);    // Desenha uma string
    ssd1306_line(&ssd, 44, 37, 44, 60, cor);           // Desenha uma linha vertical
    ssd1306_draw_string(&ssd, str_x, 8, 52);           // Desenha uma string
    ssd1306_draw_string(&ssd, str_y, 59, 52);          // Desenha uma string
    ssd1306_send_data(&ssd);                           // Atualiza o display
    sleep_ms(700);
  }
}


// Trecho para modo BOOTSEL com botão B
void btn_irq_handler(uint gpio, uint32_t events) {
  reset_usb_boot(0, 0);
}


/**
 * @brief Configura push button como saída com pull-up.
 * 
 * @param pin o pino do push button.
 */
void setup_button(uint pin) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_IN);
  gpio_pull_up(pin);
}


/**
 * @brief Configura Display ssd1306 via I2C, iniciando com todos os pixels desligados.
*/
void setup_display() {
  // Inicialização do I2C a 400Khz
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
  
  // Configura o display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); 
  ssd1306_config(&ssd);  
  ssd1306_fill(&ssd, false);                                       
  ssd1306_send_data(&ssd);
}