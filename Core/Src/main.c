/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9341.h"
#include "stdint.h"
#include "bitmaps.h"
#include "pgmspace.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
extern unsigned char fondo_mario_nivel_1[];
extern unsigned char fondo_mario_nivel_2[];
extern unsigned char menu[];
// Variables relacionadas con control de Mario
uint8_t buffer_control_1[2];
//-----------------------------------------------------------------------------
//---------------------Variables para ambos jugadores--------------------------
float gravedad = 0.01f;
//-----------------------------------------------------------------------------
//--------------- Variables relacionadas con Verticlidad de Mario--------------
uint16_t posicion_y_mario = 100;
uint8_t salta_mario = 0;
float velocidad_y_mario = 0.0f;
uint8_t mario_suelo;
//-----------------------------------------------------------------------------
//--------------- Variables relacionadas con Verticlidad de luigi--------------
uint8_t salta_luigi = 0;
uint16_t posicion_y_luigi = 100;
float velocidad_y_luigi = 0.0f;
uint8_t luigi_suelo;
//-----------------------------------------------------------------------------
//-----------------Variables relacionadas con horizontalidad de Mario----------
uint8_t animacion_mario_corriendo;
uint8_t derecha_mario = 2;
uint16_t posicion_x_mario = 20;
//-----------------------------------------------------------------------------
//-----------------Variables relacionadas con horizontalidad de luigi----------
uint16_t posicion_x_luigi = 40;
uint8_t derecha_luigi = 2;
uint8_t animacion_luigi_corriendo;
//-----------------------------------------------------------------------------
//--------------Variables relacionadas con la vida interna de mario------------
uint8_t estado_mario = 0;
uint8_t estado_luigi = 1;
//-----------------------------------------------------------------------------
//-------------Variables ralcionadas con el nivel y la etapa-------------------
uint8_t nivel;
uint8_t estado_nivel;
uint8_t bandera_estado_nivel;
#define MAPA_ANCHO  21  // 320 / 15
#define MAPA_ALTO   16  // 240 / 15
uint8_t mapa_colision[MAPA_ALTO][MAPA_ANCHO];
//-----------------------------------------------------------------------------
//--------------------Variables relacionadas con el entorno--------------------
uint8_t Estado_bloques_animacion;
uint8_t bandera_recargar_bloques;
uint8_t Bandera_colision;
uint8_t BloqueX_colision;
uint8_t BloqueY_colision;
uint8_t bloque_desactivado[5] = { 0 };
uint8_t bloque_animando[5] = { 0 }; // 1 = mostrar sprite especial, luego apagar
uint8_t flag_bloque_evento_disparado[5] = { 0 };
uint8_t contador_animacion_bloque[3];
//-----------------------------------------------------------------------------
//--------------------Variables relacionadas con los enemigos------------------
#define MAX_GOOMBAS 10
typedef struct {
	uint16_t x;
	uint16_t y;
	int8_t direccion;     // -1 o 1
	uint8_t activo;       // 1 si está en juego
} Goomba;

Goomba goombas[MAX_GOOMBAS];

uint8_t num_goombas_activos = 0;   // cuántos están activos según la etapa
uint8_t animacion_gumba = 0; // frame global de animación (si todos animan igual)
uint8_t bandera_mover_gumba = 0;   // se activa desde interrupción del timer
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM7_Init(void);
/* USER CODE BEGIN PFP */

static void Funciones_mario_chiquito(void);
static void Funciones_mario_grande(void);
static void Funciones_luigi_chiquito(void);
static void Funciones_luigi_grande(void);
void Nivel(uint8_t nivel);
static void Primer_pantalla_segundo_nivel(void);
static void animacion_bloques_amarillos(void);
void cargar_colisiones_etapa_0(void);
uint8_t detectar_colision_mapa(uint16_t x, uint16_t y, char direccion,
		uint8_t *col_y, uint8_t *col_x);
uint8_t mario_tiene_suelo(void);
uint8_t luigi_tiene_suelo(void);
void dibujar_mario_chiquito_en_tierra(void);
uint8_t mario_cae_sobre_bloque(uint8_t col_y, uint8_t col_x);
uint8_t hay_colision_entre_jugadores(uint16_t x1, uint16_t y1, uint8_t w1,
		uint8_t h1, uint16_t x2, uint16_t y2, uint8_t w2, uint8_t h2);
uint8_t jugador_sobre_otro(uint16_t x1, uint16_t y1, uint8_t w1, uint8_t h1,
		uint16_t x2, uint16_t y2, uint8_t w2, uint8_t h2);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void escribir_terminal(char *string) {
	uint8_t largo = strlen(string);
	HAL_UART_Transmit(&huart2, string, largo, 200);
}
static void Funciones_mario_chiquito(void) {
	mario_suelo = mario_tiene_suelo();

	// Movimiento horizontal
	if (derecha_mario == 1) {  // Mover a la derecha

		uint8_t cx1, cy1, cx2, cy2;
		uint8_t col1 = detectar_colision_mapa(posicion_x_mario + 20,
				posicion_y_mario + 5, 'H', &cy1, &cx1);
		uint8_t col2 = detectar_colision_mapa(posicion_x_mario + 20,
				posicion_y_mario + 15, 'H', &cy2, &cx2);

		uint8_t colision_con_luigi = hay_colision_entre_jugadores(
				posicion_x_mario + 1, posicion_y_mario, 20, 19,
				posicion_x_luigi, posicion_y_luigi, 20, 19);

		if (col1 != 1 && col2 != 1 && colision_con_luigi == 0) {
			posicion_x_mario++;
		}

		if (!salta_mario) {
			animacion_mario_corriendo = (posicion_x_mario / 5) % 4;
			uint8_t flip = (derecha_mario == 0) ? 1 : 0;
			LCD_Sprite(posicion_x_mario, posicion_y_mario + -5, 20, 19,
					mario_corriendo_nivel_2, 4, animacion_mario_corriendo, flip,
					0);
		}
		derecha_mario = 2;
	}

	if (derecha_mario == 0) {  // Mover a la izquierda

		uint8_t cx1, cy1, cx2, cy2;
		uint8_t col1 = detectar_colision_mapa(posicion_x_mario - 1,
				posicion_y_mario + 5, 'H', &cy1, &cx1);
		uint8_t col2 = detectar_colision_mapa(posicion_x_mario - 1,
				posicion_y_mario + 15, 'H', &cy2, &cx2);
		uint8_t colision_con_luigi = hay_colision_entre_jugadores(
				posicion_x_mario + 1, posicion_y_mario, 20, 19,
				posicion_x_luigi, posicion_y_luigi, 20, 19);
		if (col1 != 1 && col2 != 1 && colision_con_luigi == 0) {
			posicion_x_mario--;
		}

		if (!salta_mario) {
			animacion_mario_corriendo = (posicion_x_mario / 5) % 4;
			uint8_t flip = (derecha_mario == 0) ? 1 : 0;
			LCD_Sprite(posicion_x_mario, posicion_y_mario + -5, 20, 19,
					mario_corriendo_nivel_2, 4, animacion_mario_corriendo, flip,
					0);
		}
		derecha_mario = 2;
	}
	if (jugador_sobre_otro(posicion_x_mario, posicion_y_mario, 20, 19,  // Mario chico
	                       posicion_x_luigi, posicion_y_luigi, 20, 34)) { // Luigi grande
		velocidad_y_mario = 0;
		salta_mario = 0;
		posicion_y_mario = posicion_y_luigi - 19;

		LCD_Sprite(posicion_x_mario, posicion_y_mario - 10, 16, 10, negro, 1, 0, 0, 0);
		LCD_Sprite(posicion_x_mario, posicion_y_mario - 5, 20, 19,
		           mario_corriendo_nivel_2, 4, animacion_mario_corriendo,
		           (derecha_mario == 0 ? 1 : 0), 0);
	}
	// Movimiento vertical (salto o caída)
	if (salta_mario) {
		posicion_y_mario += velocidad_y_mario;
		velocidad_y_mario += gravedad;

		// Verificación de colisión desde arriba (cayendo)
		if (velocidad_y_mario > 0) {
			Bandera_colision = detectar_colision_mapa(posicion_x_mario + 10,
					posicion_y_mario + 20, 'V', &BloqueY_colision,
					&BloqueX_colision);
			if (Bandera_colision == 2) {
				// 🟩 Detener caída y ajustar posición
				velocidad_y_mario = 0;
				salta_mario = 0;
				posicion_y_mario = BloqueY_colision * 15 - 17;

				// 🧽 Primero borrás donde estaba la cabeza
				LCD_Sprite(posicion_x_mario, posicion_y_mario - 10, 16, 10,
						negro, 1, 0, 0, 0);

				// ✅ Luego dibujás a Mario con el ajuste visual
				animacion_mario_corriendo = (posicion_x_mario / 5) % 4;
				uint8_t flip = (derecha_mario == 0) ? 1 : 0;
				LCD_Sprite(posicion_x_mario, posicion_y_mario + -5, 20, 19,
						mario_corriendo_nivel_2, 4, animacion_mario_corriendo,
						flip, 0);
			}
		}

		// Verificación de colisión desde abajo (golpeando bloques)
		if (velocidad_y_mario < 0) {
			Bandera_colision = detectar_colision_mapa(posicion_x_mario + 15,
					posicion_y_mario - 7, 'V', &BloqueY_colision,
					&BloqueX_colision);
			if (Bandera_colision == 2) {
				velocidad_y_mario = 0;
				posicion_y_mario += 2;
				salta_mario = 1;

				uint8_t idx = BloqueX_colision - 10;
				if (idx < 5 && bloque_desactivado[idx] == 0) {
					contador_animacion_bloque[idx] = 0;
					bloque_animando[idx] = 1;
				}
			}
		}

		// Mostrar sprite de salto
		if (velocidad_y_mario != 0) {
			LCD_Sprite(posicion_x_mario, posicion_y_mario - 5, 20, 19,
					mario_saltando_nivel_2, 1, 0, 0, 0);
		}
	}

	// Si no hay suelo debajo, empieza a caer
	if (!salta_mario && !mario_suelo) {
		salta_mario = 1;
		velocidad_y_mario = 0.1f;
	}
}
static void Funciones_mario_grande(void) {
	mario_suelo = mario_tiene_suelo();

	// Movimiento horizontal + animación
	if (derecha_mario == 1) {
		uint8_t cx1, cy1, cx2, cy2;
		uint8_t col1 = detectar_colision_mapa(posicion_x_mario + 20,
				posicion_y_mario - 10, 'H', &cy1, &cx1);
		uint8_t col2 = detectar_colision_mapa(posicion_x_mario + 20,
				posicion_y_mario + 5, 'H', &cy2, &cx2);
		uint8_t colision_con_luigi = hay_colision_entre_jugadores(
				posicion_x_mario + 1, posicion_y_mario, 20, 19,
				posicion_x_luigi, posicion_y_luigi, 20, 19);
		if (col1 != 1 && col2 != 1 && !colision_con_luigi) {
			posicion_x_mario++;
		}

		if (!salta_mario) {
			animacion_mario_corriendo = (posicion_x_mario / 5) % 4;
			LCD_Sprite(posicion_x_mario, posicion_y_mario - 5, 20, 34,
					mario_corriendo_grande_nivel_2, 4,
					animacion_mario_corriendo, 0, 0);
		}
		derecha_mario = 2;
	}

	if (derecha_mario == 0) {
		uint8_t cx1, cy1, cx2, cy2;
		uint8_t col1 = detectar_colision_mapa(posicion_x_mario - 1,
				posicion_y_mario - 10, 'H', &cy1, &cx1);
		uint8_t col2 = detectar_colision_mapa(posicion_x_mario - 1,
				posicion_y_mario + 5, 'H', &cy2, &cx2);
		uint8_t colision_con_luigi = hay_colision_entre_jugadores(
				posicion_x_mario + 1, posicion_y_mario, 20, 19,
				posicion_x_luigi, posicion_y_luigi, 20, 19);

		if (col1 != 1 && col2 != 1 && colision_con_luigi == 0) {
			posicion_x_mario--;
		}

		if (!salta_mario) {
			animacion_mario_corriendo = (posicion_x_mario / 5) % 4;
			LCD_Sprite(posicion_x_mario, posicion_y_mario - 5, 20, 34,
					mario_corriendo_grande_nivel_2, 4,
					animacion_mario_corriendo, 1, 0);
		}
		derecha_mario = 2;
	}
	if (jugador_sobre_otro(posicion_x_mario, posicion_y_mario, 20, 19,  // Mario chico
	                       posicion_x_luigi, posicion_y_luigi, 20, 34)) { // Luigi grande
		velocidad_y_mario = 0;
		salta_mario = 0;
		posicion_y_mario = posicion_y_luigi - 19;

		LCD_Sprite(posicion_x_mario, posicion_y_mario - 10, 16, 10, negro, 1, 0, 0, 0);
		LCD_Sprite(posicion_x_mario, posicion_y_mario - 5, 20, 19,
		           mario_corriendo_nivel_2, 4, animacion_mario_corriendo,
		           (derecha_mario == 0 ? 1 : 0), 0);
	}
	// Movimiento vertical (salto o caída)
	if (salta_mario) {
		posicion_y_mario += velocidad_y_mario;
		velocidad_y_mario += gravedad;

		// Caída sobre bloque
		if (velocidad_y_mario > 0) {
			Bandera_colision = detectar_colision_mapa(posicion_x_mario + 10,
					posicion_y_mario + 32, 'V', &BloqueY_colision,
					&BloqueX_colision);
			if (Bandera_colision == 2) {
				velocidad_y_mario = 0;
				salta_mario = 0;
				posicion_y_mario = BloqueY_colision * 15 - 32;

				// 🧽 Borrar cabeza vieja
				LCD_Sprite(posicion_x_mario, posicion_y_mario - 10, 16, 10,
						negro, 1, 0, 0, 0);

				animacion_mario_corriendo = (posicion_x_mario / 5) % 4;
				uint8_t flip = (derecha_mario == 0) ? 1 : 0;

				LCD_Sprite(posicion_x_mario, posicion_y_mario - 5, 20, 34,
						mario_corriendo_grande_nivel_2, 4,
						animacion_mario_corriendo, flip, 0);
			}
		}

		// Límite inferior
		if (posicion_y_mario >= 195) {
			posicion_y_mario = 195;
			velocidad_y_mario = 0;
			salta_mario = 0;
		}

		// Colisión desde abajo (golpeando bloques)
		if (velocidad_y_mario < 0) {
			Bandera_colision = detectar_colision_mapa(posicion_x_mario + 15,
					posicion_y_mario, 'V', &BloqueY_colision,
					&BloqueX_colision);
			if (Bandera_colision == 2) {
				velocidad_y_mario = 0;
				posicion_y_mario += 2;
				salta_mario = 1;
				velocidad_y_mario = 1.0f;

				uint8_t idx = BloqueX_colision - 10;
				if (idx < 5 && bloque_desactivado[idx] == 0) {
					bloque_animando[idx] = 1;
					contador_animacion_bloque[idx] = 0;
				}
			}
		}

		if (posicion_y_mario <= 195 && velocidad_y_mario != 0) {
			LCD_Sprite(posicion_x_mario, posicion_y_mario - 5, 20, 34,
					mario_saltando_grande_nivel_2, 1, 0, 0, 0);
		}
	}

	// Caída automática si no hay suelo
	if (!salta_mario && !mario_suelo) {
		salta_mario = 1;
		velocidad_y_mario = 0.1f;
	}
}
static void Funciones_luigi_chiquito(void) {
	luigi_suelo = luigi_tiene_suelo();

	if (derecha_luigi == 1) {
		uint8_t cx1, cy1, cx2, cy2;
		uint8_t col1 = detectar_colision_mapa(posicion_x_luigi + 20,
				posicion_y_luigi + 5, 'H', &cy1, &cx1);
		uint8_t col2 = detectar_colision_mapa(posicion_x_luigi + 20,
				posicion_y_luigi + 15, 'H', &cy2, &cx2);
		uint8_t colision_con_mario = hay_colision_entre_jugadores(
				posicion_x_luigi + 1, posicion_y_luigi, 20, 19,
				posicion_x_mario, posicion_y_mario, 20, 19);

		if (col1 != 1 && col2 != 1 && !colision_con_mario) {
			posicion_x_luigi++;
		}

		if (!salta_luigi) {
			animacion_luigi_corriendo = (posicion_x_luigi / 5) % 4;
			uint8_t flip = (derecha_luigi == 0) ? 1 : 0;
			LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 5, 20, 19,
					luigi_corriendo_nivel_2, 4, animacion_luigi_corriendo, flip,
					0);
		}
		derecha_luigi = 2;
	}

	if (derecha_luigi == 0) {

		uint8_t cx1, cy1, cx2, cy2;
		uint8_t col1 = detectar_colision_mapa(posicion_x_luigi - 1,
				posicion_y_luigi + 5, 'H', &cy1, &cx1);
		uint8_t col2 = detectar_colision_mapa(posicion_x_luigi - 1,
				posicion_y_luigi + 15, 'H', &cy2, &cx2);
		uint8_t colision_con_mario = hay_colision_entre_jugadores(
				posicion_x_luigi + 1, posicion_y_luigi, 20, 19,
				posicion_x_mario, posicion_y_mario, 20, 19);

		if (col1 != 1 && col2 != 1 && !colision_con_mario) {
			posicion_x_luigi--;
		}

		if (!salta_luigi) {
			animacion_luigi_corriendo = (posicion_x_luigi / 5) % 4;
			uint8_t flip = (derecha_luigi == 0) ? 1 : 0;
			LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 5, 20, 19,
					luigi_corriendo_nivel_2, 4, animacion_luigi_corriendo, flip,
					0);
		}
		derecha_luigi = 2;
	}
	if (jugador_sobre_otro(posicion_x_luigi, posicion_y_luigi, 20, 34,  // Luigi grande
	                       posicion_x_mario, posicion_y_mario, 20, 19)) { // Mario chico
		velocidad_y_luigi = 0;
		salta_luigi = 0;
		posicion_y_luigi = posicion_y_mario - 34;

		LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 10, 16, 10, negro, 1, 0, 0, 0);
		LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 5, 20, 34,
		           luigi_corriendo_grande_nivel_2, 4, animacion_luigi_corriendo,
		           (derecha_luigi == 0 ? 1 : 0), 0);
	}
	if (salta_luigi) {
		posicion_y_luigi += velocidad_y_luigi;
		velocidad_y_luigi += gravedad;

		if (velocidad_y_luigi > 0) {
			Bandera_colision = detectar_colision_mapa(posicion_x_luigi + 10,
					posicion_y_luigi + 20, 'V', &BloqueY_colision,
					&BloqueX_colision);
			if (Bandera_colision == 2) {
				velocidad_y_luigi = 0;
				salta_luigi = 0;
				posicion_y_luigi = BloqueY_colision * 15 - 17;

				LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 10, 16, 10,
						negro, 1, 0, 0, 0);

				animacion_luigi_corriendo = (posicion_x_luigi / 5) % 4;
				uint8_t flip = (derecha_luigi == 0) ? 1 : 0;
				LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 5, 20, 19,
						luigi_corriendo_nivel_2, 4, animacion_luigi_corriendo,
						flip, 0);
			}
		}

		if (velocidad_y_luigi < 0) {
			Bandera_colision = detectar_colision_mapa(posicion_x_luigi + 15,
					posicion_y_luigi - 7, 'V', &BloqueY_colision,
					&BloqueX_colision);
			if (Bandera_colision == 2) {
				velocidad_y_luigi = 0;
				posicion_y_luigi += 2;
				salta_luigi = 1;

				uint8_t idx = BloqueX_colision - 10;
				if (idx < 5 && bloque_desactivado[idx] == 0) {
					contador_animacion_bloque[idx] = 0;
					bloque_animando[idx] = 1;
				}
			}
		}

		if (velocidad_y_luigi != 0) {
			LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 5, 20, 19,
					luigi_saltando_nivel_2, 1, 0, 0, 0);
		}
	}

	if (!salta_luigi && !luigi_suelo) {
		salta_luigi = 1;
		velocidad_y_luigi = 0.1f;
	}
}
static void Funciones_luigi_grande(void) {
	luigi_suelo = luigi_tiene_suelo();

	if (derecha_luigi == 1) {
		uint8_t cx1, cy1, cx2, cy2;
		uint8_t col1 = detectar_colision_mapa(posicion_x_luigi + 20,
				posicion_y_luigi - 10, 'H', &cy1, &cx1);
		uint8_t col2 = detectar_colision_mapa(posicion_x_luigi + 20,
				posicion_y_luigi + 5, 'H', &cy2, &cx2);
		uint8_t colision_con_mario = hay_colision_entre_jugadores(
				posicion_x_luigi + 1, posicion_y_luigi, 20, 19,
				posicion_x_mario, posicion_y_mario, 20, 19);

		if (col1 != 1 && col2 != 1 && !colision_con_mario) {
			posicion_x_luigi++;
		}

		if (!salta_luigi) {
			animacion_luigi_corriendo = (posicion_x_luigi / 5) % 4;
			LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 5, 20, 34,
					luigi_corriendo_grande_nivel_2, 4,
					animacion_luigi_corriendo, 0, 0);
		}
		derecha_luigi = 2;
	}

	if (derecha_luigi == 0) {
		uint8_t cx1, cy1, cx2, cy2;
		uint8_t col1 = detectar_colision_mapa(posicion_x_luigi - 1,
				posicion_y_luigi - 10, 'H', &cy1, &cx1);
		uint8_t col2 = detectar_colision_mapa(posicion_x_luigi - 1,
				posicion_y_luigi + 5, 'H', &cy2, &cx2);
		uint8_t colision_con_mario = hay_colision_entre_jugadores(
				posicion_x_luigi + 1, posicion_y_luigi, 20, 19,
				posicion_x_mario, posicion_y_mario, 20, 19);

		if (col1 != 1 && col2 != 1 && !colision_con_mario) {
			posicion_x_luigi--;
		}

		if (!salta_luigi) {
			animacion_luigi_corriendo = (posicion_x_luigi / 5) % 4;
			LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 5, 20, 34,
					luigi_corriendo_grande_nivel_2, 4,
					animacion_luigi_corriendo, 1, 0);
		}
		derecha_luigi = 2;
	}
	if (jugador_sobre_otro(posicion_x_luigi, posicion_y_luigi, 20, 34,  // Luigi grande
	                       posicion_x_mario, posicion_y_mario, 20, 19)) { // Mario chico
		velocidad_y_luigi = 0;
		salta_luigi = 0;
		posicion_y_luigi = posicion_y_mario - 34;

		LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 10, 16, 10, negro, 1, 0, 0, 0);
		LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 5, 20, 34,
		           luigi_corriendo_grande_nivel_2, 4, animacion_luigi_corriendo,
		           (derecha_luigi == 0 ? 1 : 0), 0);
	}
	if (salta_luigi) {
		posicion_y_luigi += velocidad_y_luigi;
		velocidad_y_luigi += gravedad;

		if (velocidad_y_luigi > 0) {
			Bandera_colision = detectar_colision_mapa(posicion_x_luigi + 10,
					posicion_y_luigi + 32, 'V', &BloqueY_colision,
					&BloqueX_colision);
			if (Bandera_colision == 2) {
				velocidad_y_luigi = 0;
				salta_luigi = 0;
				posicion_y_luigi = BloqueY_colision * 15 - 32;

				LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 10, 16, 10,
						negro, 1, 0, 0, 0);

				animacion_luigi_corriendo = (posicion_x_luigi / 5) % 4;
				uint8_t flip = (derecha_luigi == 0) ? 1 : 0;

				LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 5, 20, 34,
						luigi_corriendo_grande_nivel_2, 4,
						animacion_luigi_corriendo, flip, 0);
			}
		}

		if (posicion_y_luigi >= 195) {
			posicion_y_luigi = 195;
			velocidad_y_luigi = 0;
			salta_luigi = 0;
		}

		if (velocidad_y_luigi < 0) {
			Bandera_colision = detectar_colision_mapa(posicion_x_luigi + 15,
					posicion_y_luigi, 'V', &BloqueY_colision,
					&BloqueX_colision);
			if (Bandera_colision == 2) {
				velocidad_y_luigi = 0;
				posicion_y_luigi += 2;
				salta_luigi = 1;
				velocidad_y_luigi = 1.0f;

				uint8_t idx = BloqueX_colision - 10;
				if (idx < 5 && bloque_desactivado[idx] == 0) {
					bloque_animando[idx] = 1;
					contador_animacion_bloque[idx] = 0;
				}
			}
		}

		if (posicion_y_luigi <= 195 && velocidad_y_luigi != 0) {
			LCD_Sprite(posicion_x_luigi, posicion_y_luigi - 5, 20, 33,
					luigi_saltando_grande_nivel_2, 1, 0, 0, 0);
		}
	}

	if (!salta_luigi && !luigi_suelo) {
		salta_luigi = 1;
		velocidad_y_luigi = 0.1f;
	}
}

void Nivel(uint8_t nivel) {
	if (nivel == 1) {
		if (estado_nivel == 0) {
			LCD_Clear(0x00);
			Primer_pantalla_segundo_nivel();
		}

	}
}
static void Primer_pantalla_segundo_nivel(void) {
	//LCD_Bitmap(0, 0, 320, 240, fondo_mario_nivel_2);
	LCD_Sprite(posicion_x_mario, posicion_y_mario, 20, 19,
			mario_corriendo_nivel_2, 4, animacion_mario_corriendo, 0, 0);
	for (uint8_t i = 0; i < 14; i++) { // 240 / 15 = 16 bloques
		uint16_t y_bloque = i * 15;
		LCD_Sprite(0, y_bloque + 10, 15, 15, bloque_adorno, 1, 0, 0, 0); // suponiendo que bloque es tu sprite de bloque
	}
	for (uint8_t i = 0; i < 16; i++) { // 240 / 15 = 16 bloques
		uint16_t x_bloque = i * 15 + 80;
		LCD_Sprite(x_bloque, 10, 15, 15, bloque_adorno, 1, 0, 0, 0); // suponiendo que bloque es tu sprite de bloque
	}
	for (uint8_t i = 0; i < 5; i++) { // 240 / 15 = 16 bloques
		uint16_t x_bloque = i * 15 + 150;
		LCD_Sprite(x_bloque, 150, 16, 16, Bloque_amarillo_misterioso_nivel_2, 3,
				0, 0, 0); // suponiendo que bloque es tu sprite de bloque
	}
	for (uint8_t i = 0; i < 20; i++) {
		uint16_t x_bloque = i * 16;
		LCD_Sprite(x_bloque, 208, 16, 16, bloque_piso, 1, 1, 0, 0);
	}
	for (uint8_t i = 0; i < 20; i++) {
		uint16_t x_bloque = i * 16;
		LCD_Sprite(x_bloque, 224, 16, 16, bloque_piso, 1, 1, 0, 0);
	}
	cargar_colisiones_etapa_0();

}
static void animacion_bloques_amarillos(void) {
	if (estado_nivel == 0) {
		for (uint8_t i = 0; i < 5; i++) {
			uint16_t x_bloque = i * 15 + 150;

			if (bloque_desactivado[i] == 0) {
				if (bloque_animando[i]) {
					// Mostrar el frame actual sin sumarlo aquí
					LCD_Sprite(x_bloque, 154 - 10, 15, 22,
							Bloque_amarillo_misterioso_rompiendose_nivel_2, 3,
							contador_animacion_bloque[i], 0, 0);
				} else {
					// Animación normal
					LCD_Sprite(x_bloque, 150, 16, 16,
							Bloque_amarillo_misterioso_nivel_2, 3,
							Estado_bloques_animacion, 0, 0);
				}
			} else {
				// Mostrar sprite apagado
				//LCD_Sprite(x, y, width, height, bitmap, columns, index, flip, offset)
				LCD_Sprite(x_bloque, 154 - 10, 16, 22,
						Bloque_amarillo_misterioso_rompiendose_nivel_2, 3, 2, 0,
						0);
			}
		}
	}
}
void cargar_colisiones_etapa_0(void) {
	memset(mapa_colision, 0, sizeof(mapa_colision));

	// Columna izquierda
	for (uint8_t i = 0; i < 14; i++)
		mapa_colision[i][0] = 1;

	// Bloques horizontales arriba
	for (uint8_t i = 0; i < 16; i++)
		mapa_colision[0][(i * 15 + 80) / 15] = 1;

	// Bloques misteriosos
	for (uint8_t i = 0; i < 5; i++)
		mapa_colision[150 / 15][(i * 15 + 150) / 15] = 1;
	// Piso colisiones
	for (uint8_t i = 0; i < 22; i++) {
		mapa_colision[14][i] = 1;
	}
}
uint8_t detectar_colision_mapa(uint16_t x, uint16_t y, char direccion,
		uint8_t *col_y, uint8_t *col_x) {
	{
		uint8_t bx = x / 15;
		uint8_t by = y / 15;

		if (bx >= MAPA_ANCHO || by >= MAPA_ALTO)
			return 0;

		if (mapa_colision[by][bx] == 1) {
			*col_x = bx;
			*col_y = by;

			if (direccion == 'V') {
				// Ya no eliminamos el bloque
				return 2;  // colisión vertical
			}
			return 1;  // colisión lateral
		}
		return 0;
	}
}
uint8_t mario_tiene_suelo(void) {
	uint8_t col_x, col_y;
	uint8_t offset_suelo = (estado_mario == 0) ? 20 : 32; // más abajo si Mario es grande

	uint8_t resultado = detectar_colision_mapa(posicion_x_mario + 10,
			posicion_y_mario + offset_suelo, 'V', &col_y, &col_x);
	return resultado == 2;
}
uint8_t luigi_tiene_suelo(void) {
	uint8_t col_x, col_y;
	uint8_t offset_suelo = (estado_luigi == 0) ? 20 : 32; // 20 si es chiquito, 32 si es grande

	uint8_t resultado = detectar_colision_mapa(posicion_x_luigi + 10,
			posicion_y_luigi + offset_suelo, 'V', &col_y, &col_x);
	return resultado == 2;
}
uint8_t hay_colision_entre_jugadores(uint16_t x1, uint16_t y1, uint8_t w1,
		uint8_t h1, uint16_t x2, uint16_t y2, uint8_t w2, uint8_t h2) {
	return !(x1 + w1 <= x2 ||  // lado derecho de Mario a la izquierda de Luigi
			x1 >= x2 + w2 ||   // lado izquierdo de Mario a la derecha de Luigi
			y1 + h1 <= y2 ||   // parte baja de Mario arriba de Luigi
			y1 >= y2 + h2);    // parte alta de Mario debajo de Luigi
}
uint8_t jugador_sobre_otro(uint16_t x1, uint16_t y1, uint8_t w1, uint8_t h1,
                           uint16_t x2, uint16_t y2, uint8_t w2, uint8_t h2) {
	// Verifica si jugador 1 está "encima" del jugador 2
	if (x1 + w1 >= x2 && x1 <= x2 + w2) {
		if ((y1 + h1) >= y2 - 1 && (y1 + h1) <= y2 + 6) {
			return 1;
		}
	}
	return 0;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */
	LCD_Init();
	HAL_UART_Receive_IT(&huart2, buffer_control_1, 1);
	LCD_Clear(0x00);
	HAL_TIM_Base_Start_IT(&htim6);
	HAL_TIM_Base_Start_IT(&htim7);
	nivel = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		//----------------------------------------------------------------------------------------------------------
		//-------------------------------------Mario chiquieto movimiento-------------------------------------------
		if (nivel == 0) {
			LCD_Bitmap(0, 0, 320, 240, menu);
		}
		if (nivel == 1) {
			if (bandera_estado_nivel==0) {
				Nivel(nivel);
				bandera_estado_nivel=1;
			}
			if (estado_mario == 0) {
				Funciones_mario_chiquito();
			}
			//----------------------------------------------------------------------------------------------------------
			//-------------------------------------Mario grande movimiento----------------------------------------------
			if (estado_mario == 1) {
				Funciones_mario_grande();
			}
			if (estado_luigi == 0) {
				Funciones_luigi_chiquito();
			}
			//----------------------------------------------------------------------------------------------------------
			//-------------------------------------Mario grande movimiento----------------------------------------------
			if (estado_luigi == 1) {
				Funciones_luigi_grande();
			}
			if (bandera_recargar_bloques == 1) {
				animacion_bloques_amarillos();
			}
		}

	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 719;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 19970 ;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 719;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 19970 ;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin|LCD_D1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin|SD_SS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LCD_RST_Pin LCD_D1_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RD_Pin LCD_WR_Pin LCD_RS_Pin LCD_D7_Pin
                           LCD_D0_Pin LCD_D2_Pin */
  GPIO_InitStruct.Pin = LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_D6_Pin LCD_D3_Pin LCD_D5_Pin
                           LCD_D4_Pin SD_SS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin|SD_SS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	HAL_UART_Receive_IT(&huart2, buffer_control_1, 1);
	nivel = 1;
	//escribir_terminal("\nrecibo valor\n");
	if (buffer_control_1[0] == 'R') {
		derecha_mario = 1;
	}

	if (buffer_control_1[0] == 'L') {
		derecha_mario = 0;
	}
	if (buffer_control_1[0] == 'J' && salta_mario != 1) {
		salta_mario = 1;
		velocidad_y_mario = -1.0f;
	}
	if (buffer_control_1[0] == 'D') {
		derecha_luigi = 1;
	}

	if (buffer_control_1[0] == 'A') {
		derecha_luigi = 0;
	}
	if (buffer_control_1[0] == 'W' && salta_luigi != 1) {
		salta_luigi = 1;
		velocidad_y_luigi = -1.0f;
	}
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM6) {
		bandera_recargar_bloques = 1;
		Estado_bloques_animacion = (Estado_bloques_animacion + 1) % 3;

		for (uint8_t i = 0; i < 5; i++) {
			if (bloque_animando[i]) {
				contador_animacion_bloque[i]++;
				if (contador_animacion_bloque[i] >= 3) {
					bloque_animando[i] = 0;
					bloque_desactivado[i] = 1;
					contador_animacion_bloque[i] = 0;

					if (flag_bloque_evento_disparado[i] == 0) {
						flag_bloque_evento_disparado[i] = 1;

					}
				}
			}
		}
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
