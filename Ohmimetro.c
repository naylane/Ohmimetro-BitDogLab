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
#include <math.h>

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28          // GPIO para o ADC
#define WS2812_PIN 7        // GPIO para a matriz de LEDs
#define BUTTON_A 5          // GPIO para botão A
#define BUTTON_B 6          // GPIO para botão B
#define DEBOUNCE_TIME 200000        // Tempo para debounce (200 ms)
static uint32_t last_time_A = 0;    // Tempo da última interrupção do botão A
static uint32_t last_time_B = 0;    // Tempo da última interrupção do botão B

int R_conhecido = 10000;    // Resistor de 10k ohm
float R_x = 0.0;            // Resistor desconhecido
int ADC_RESOLUTION = 4095;  // Resolução do ADC (12 bits)
ssd1306_t ssd;              // Estrutura do display
bool cor = true;
bool tela = 0;
char *faixa1, *faixa2, *mult;

// Valores comerciais E24 (5% de tolerância)
float baseE24[] = {
  1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0,
  2.2, 2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.3,
  4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1
};


void gera_faixa_cores(int resistor, char** faixa1, char** faixa2, char** mult);
float encontrar_valor_comercial(float resistor);
void btn_irq_handler(uint gpio, uint32_t events);
void setup_button(uint pin);
void setup_display();


int main() {
  setup_button(BUTTON_A);
  setup_button(BUTTON_B);
  gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &btn_irq_handler);
  gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &btn_irq_handler);

  setup_display();

  adc_init();
  adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

  
  // Inicializa o PIO para controlar a matriz de LEDs (WS2812)
  PIO pio = pio0;
  uint sm = 0;
  uint offset = pio_add_program(pio, &pio_matrix_program);
  pio_matrix_program_init(pio, sm, offset, WS2812_PIN);
  clear_matrix(pio, sm);

  char str_x[5]; // Buffer para armazenar a string
  char str_y[5]; // Buffer para armazenar a string
  char str_e24[5];

  while (true) {
    adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

    
    float soma_tensao = 0.0f;
    for (int i = 0; i < 500; i++) {
      soma_tensao += adc_read();
      sleep_ms(1);
    }
    float media = soma_tensao / 500.0f;

    // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
    R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);
    float valor_comercial = encontrar_valor_comercial(R_x);

    sprintf(str_x, "%1.0f", media);
    sprintf(str_y, "%1.0f", R_x);
    sprintf(str_e24, "%1.0f", valor_comercial);

    char *faixa1, *faixa2, *mult;
    gera_faixa_cores(valor_comercial, &faixa1, &faixa2, &mult);

    // Atualiza a matriz de LEDs
    set_led(1, 1, faixa1);
    set_led(3, 2, faixa1);
    set_led(1, 3, faixa1);
    set_led(2, 1, faixa2);
    set_led(2, 2, faixa2);
    set_led(2, 3, faixa2);
    set_led(3, 1, mult);
    set_led(1, 2, mult);
    set_led(3, 3, mult);
    update_matrix(pio, sm);


    if (tela == 0) {
      ssd1306_fill(&ssd, !cor);
      ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // borda
      ssd1306_line(&ssd, 3, 25, 123, 25, cor);           // linha de cima
      ssd1306_draw_string(&ssd, "Ohmimetro", 28, 10);
      ssd1306_draw_string(&ssd, "E24:", 10, 28);
      ssd1306_draw_string(&ssd, str_e24, 48, 28);        // valor comercial
      ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // linha de baixo
      ssd1306_draw_string(&ssd, "ADC", 13, 41);
      ssd1306_draw_string(&ssd, "Resisten.", 50, 41);
      ssd1306_draw_string(&ssd, str_x, 8, 52);           // valor do ADC
      ssd1306_draw_string(&ssd, str_y, 59, 52);          // valor do resistor
      ssd1306_line(&ssd, 44, 37, 44, 60, cor);           // linha do meio
      
  } else {
      ssd1306_fill(&ssd, !cor);
      ssd1306_rect(&ssd, 4, 30, 70, 8, cor, !cor);       // Resistor
      ssd1306_line(&ssd, 4, 7, 30, 7, cor);              // linha direita
      ssd1306_line(&ssd, 100, 7, 125, 7, cor);           // linha esquerda
      ssd1306_draw_string(&ssd, faixa1, 8, 25);          // 1ª faixa
      ssd1306_draw_string(&ssd, faixa2, 8, 35);          // 2ª faixa
      ssd1306_draw_string(&ssd, mult, 8, 45);            // multiplicador
   }
    
    ssd1306_send_data(&ssd);

    sleep_ms(700);
  }
}


/**
 * @brief Determina as 3 primeiras faixas de cor (1ª, 2ª e multiplicador)
 *        para um resistor com valor comercial informado em ohms.
 * 
 * @param resistor Valor do resistor (em ohms). Assumindo que ele tenha pelo menos 2 dígitos.
 * @param faixa1 Ponteiro para armazenar a string da 1ª faixa.
 * @param faixa2 Ponteiro para armazenar a string da 2ª faixa.
 * @param mult Ponteiro para armazenar a string do multiplicador.
 */
void gera_faixa_cores(int resistor, char** faixa1, char** faixa2, char** mult) {
    char* cores[] = {"preto", "marrom", "vermelho", "laranja", "amarelo", "verde", "azul", "roxo", "cinza", "branco"};

    // conta quantos dígitos tem o valor do resistor
    int n = 0;
    int temp = resistor;
    while (temp > 0) {
        n++;
        temp /= 10;
    }

    // define o expoente do multiplicador considerando <dígitos significativos> x 10^expoente
    int expoente = n - 2; // duas casas significativas
    if (expoente < 0) {
        expoente = 0;
    }

    // calcula o divisor que isola as duas primeiras faixas
    int divisor = 1;
    for (int i = 0; i < expoente; i++) {
        divisor *= 10;
    }

    // calcula os dois dígitos significativos
    int significand = (int)((float)resistor / divisor + 0.5f);

    // se significand chegar a 100, usa apenas duas casas (100 vira 10 e incrementa o expoente)
    if (significand >= 100) {
        significand /= 10;
        expoente++;
    }

    // separar o significand em dois dígitos
    int firstDigit = significand / 10;
    int secondDigit = significand % 10;

    // atribui as cores correspondentes
    *faixa1 = cores[firstDigit];
    // A 2ª faixa corresponde ao segundo dígito.
    *faixa2 = cores[secondDigit];


    // verifica se o expoente está dentro do intervalo suportado (0 a 9)
    if (expoente < 0 || expoente > 9) {
      *mult = "Inválido";  // Caso esteja fora dos limites
    } else {
      *mult = cores[expoente];
    }
}


/**
 * @brief Encontra o valor comercial mais próximo na série E24 (510Ω a 100kΩ).
 * 
 * @param resistor Valor do resistor a ser comparado.
 * @return float Valor comercial mais próximo.
 */
float encontrar_valor_comercial(float resistor) {
    float valor_proximo = 0.0;
    float menor_diferenca = INFINITY;

    // Multiplicadores para a faixa de 510Ω a 100kΩ
    int multiplicadores[] = {1, 10, 100, 1000, 10000, 100000};
    int num_multiplicadores = sizeof(multiplicadores) / sizeof(multiplicadores[0]);

    for (int m = 0; m < num_multiplicadores; m++) {
        for (int i = 0; i < sizeof(baseE24) / sizeof(baseE24[0]); i++) {
            float valor_atual = baseE24[i] * multiplicadores[m];
            if (valor_atual >= 510 && valor_atual <= 100000) { // Restringe à faixa desejada
                float diferenca = fabs(resistor - valor_atual);
                if (diferenca < menor_diferenca) {
                    menor_diferenca = diferenca;
                    valor_proximo = valor_atual;
                }
            }
        }
    }

    return valor_proximo;
}


/**
 * @brief Manipulador de interrupção para os botões A e B.
 * 
 * @param gpio O número do GPIO que gerou a interrupção.
 * @param events Eventos associados à interrupção.
 */
void btn_irq_handler(uint gpio, uint32_t events) {
  uint32_t current_time = to_us_since_boot(get_absolute_time());

  if (gpio == BUTTON_A) {
    if (current_time - last_time_A > DEBOUNCE_TIME) {
      tela = !tela; // Alterna entre as telas
      last_time_A = current_time;
      return;
    }
  } else if (gpio == BUTTON_B) {
    if (current_time - last_time_B > DEBOUNCE_TIME) {
      reset_usb_boot(0, 0);
      last_time_B = current_time;
      return;
    }
  }
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