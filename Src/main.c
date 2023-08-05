/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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
#include <stdlib.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
	GPIO_TypeDef *GPIO;
	uint16_t PIN;
} GPIO_T;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LOW     GPIO_PIN_RESET
#define HIGH    GPIO_PIN_SET
#define OUTPUT	GPIO_MODE_OUTPUT_PP
#define INPUT	GPIO_MODE_INPUT

#define LSBFIRST     1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim6;

/* USER CODE BEGIN PV */
GPIO_T strobe = { GPIOB, GPIO_PIN_8 }; /*D5*/
GPIO_T clock = { GPIOB, GPIO_PIN_12 }; /*D6*/
GPIO_T data = { GPIOC, GPIO_PIN_7 }; /*D7*/

GPIO_T strobe2 = { GPIOC, GPIO_PIN_0 }; /*PC0 左下のブロック上から3つ並んでる*/
GPIO_T clock2 = { GPIOC, GPIO_PIN_1 }; /*PC1*/
GPIO_T data2 = { GPIOC, GPIO_PIN_2 }; /*PC2*/
unsigned long digit_data = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
void digitalWrite(GPIO_T gpio, GPIO_PinState PinState);
GPIO_PinState digitalRead(GPIO_T gpio);
void pinMode(GPIO_T data, uint32_t mode);
void sendCommand(uint8_t value, GPIO_T strobepin, GPIO_T datapin,
		GPIO_T clockpin);
void reset(GPIO_T strobe, GPIO_T data, GPIO_T clock, GPIO_T strobe2,
		GPIO_T data2, GPIO_T clock2);
void setup(GPIO_T strobe, GPIO_T data, GPIO_T clock, GPIO_T strobe2,
		GPIO_T data2, GPIO_T clock2);
uint8_t readButtons(GPIO_T strobe, GPIO_T data, GPIO_T clockd);
void shiftOut(GPIO_T dataPin, GPIO_T clockPin, uint8_t bitOrder, uint8_t val);
uint8_t shiftIn(GPIO_T dataPin, GPIO_T clockPin, uint8_t bitOrder);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void digitalWrite(GPIO_T gpio, GPIO_PinState PinState) {
	HAL_GPIO_WritePin(gpio.GPIO, gpio.PIN, PinState);
}
GPIO_PinState digitalRead(GPIO_T gpio) {
	GPIO_PinState ret;
	ret = HAL_GPIO_ReadPin(gpio.GPIO, gpio.PIN);
	return ret;
}
void pinMode(GPIO_T data, uint32_t mode) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	if (mode == INPUT) {
		/*Configure GPIO INPUT pin */
		GPIO_InitStruct.Pin = data.PIN;
		GPIO_InitStruct.Mode = mode;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		HAL_GPIO_Init(data.GPIO, &GPIO_InitStruct);
	} else if (mode == OUTPUT) {
		/*Configure GPIO pin : PB4 */
		GPIO_InitStruct.Pin = data.PIN;
		GPIO_InitStruct.Mode = mode;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(data.GPIO, &GPIO_InitStruct);
	}
}

void sendCommand(uint8_t value, GPIO_T strobe, GPIO_T data, GPIO_T clock) {
	digitalWrite(strobe, LOW);
	shiftOut(data, clock, LSBFIRST, value);
	digitalWrite(strobe, HIGH);
}

void reset(GPIO_T strobe, GPIO_T data, GPIO_T clock, GPIO_T strobe2,
		GPIO_T data2, GPIO_T clock2) {
	uint8_t i;
	sendCommand(0x40, strobe, data, clock);
	digitalWrite(strobe, LOW);
	shiftOut(data, clock, LSBFIRST, 0xc0);
	for (i = 0; i < 16; i++) {
		shiftOut(data, clock, LSBFIRST, 0x00);
	}
	digitalWrite(strobe, HIGH);

	sendCommand(0x40, strobe2, data2, clock2);
	digitalWrite(strobe2, LOW);
	shiftOut(data2, clock2, LSBFIRST, 0xc0);
	for (i = 0; i < 16; i++) {
		shiftOut(data2, clock2, LSBFIRST, 0x00);
	}
	digitalWrite(strobe2, HIGH);

}

void setup(GPIO_T strobe, GPIO_T data, GPIO_T clock, GPIO_T strobe2,
		GPIO_T data2, GPIO_T clock2) {
	pinMode(strobe, OUTPUT);
	pinMode(clock, OUTPUT);
	pinMode(data, OUTPUT);
	sendCommand(0x8f, strobe, data, clock);  // activate
	pinMode(strobe2, OUTPUT);
	pinMode(clock2, OUTPUT);
	pinMode(data2, OUTPUT);
	sendCommand(0x8f, strobe2, data2, clock2);  // activate
	reset(strobe, data, clock, strobe2, data2, clock2);
}

uint8_t readButtons(GPIO_T strobe, GPIO_T data, GPIO_T clock) {
	uint8_t i;

	uint8_t buttons = 0;
	digitalWrite(strobe, LOW);
	shiftOut(data, clock, LSBFIRST, 0x42);

	pinMode(data, INPUT);

	for (i = 0; i < 4; i++) {
		uint8_t v = shiftIn(data, clock, LSBFIRST) << i;
		buttons |= v;
	}

	pinMode(data, OUTPUT);
	digitalWrite(strobe, HIGH);
	return buttons;
}

void shiftOut(GPIO_T dataPin, GPIO_T clockPin, uint8_t bitOrder, uint8_t val) {
	uint8_t i;

	for (i = 0; i < 8; i++) {
		if (bitOrder == LSBFIRST) {
			digitalWrite(dataPin, val & 1);
			val >>= 1;
		} else {
			digitalWrite(dataPin, (val & 128) != 0);
			val <<= 1;
		}

		digitalWrite(clockPin, HIGH);
		digitalWrite(clockPin, LOW);
	}
}

uint8_t shiftIn(GPIO_T dataPin, GPIO_T clockPin, uint8_t bitOrder) {
	uint8_t value = 0;
	uint8_t i;

	for (i = 0; i < 8; ++i) {
		digitalWrite(clockPin, HIGH);
		if (bitOrder == LSBFIRST)
			value |= digitalRead(dataPin) << i;
		else
			value |= digitalRead(dataPin) << (7 - i);
		digitalWrite(clockPin, LOW);
	}
	return value;
}
// 7セグメントディスプレイのパターン
static const uint8_t SEGMENT_PATTERNS[16] = { 0b00111111,  // 0
		0b00000110,  // 1
		0b01011011,  // 2
		0b01001111,  // 3
		0b01100110,  // 4
		0b01101101,  // 5
		0b01111101,  // 6
		0b00000111,  // 7
		0b01111111,  // 8
		0b01101111,  // 9
		0b01110111,  // A 0x77
		0b01111100,  // B 0x7c
		0b01100011,  // Upper part of ロ
		0b01011100,  // Lower part of ロ
		0b01111001,  //E 0x79
		0b00111000   //L 0x38
		};
static const uint8_t BABEL[] = { 0x7f, 0x77, 0x7f, 0x79, 0x38, 0, 0x7f, 0x77,
		0x7f, 0x79, 0x38, 0, 0x7f, 0x77, 0x7f, 0x79, 0x38, 0, 0x7f };

uint8_t convert_to_7seg(int number) {
	if (number < 0 || number > 9) {
		return 0;
	}
	return SEGMENT_PATTERNS[number];
}
void set7SEG(uint8_t *displaynum, GPIO_T strobe, GPIO_T data, GPIO_T clock) {
	int i = 0;
	sendCommand(0x44, strobe, data, clock); // set fixed mode
	for (i = 0; i < 8; i++) {
		digitalWrite(strobe, LOW);
		shiftOut(data, clock, LSBFIRST, (0xc0) + 2 * i);
		shiftOut(data, clock, LSBFIRST, displaynum[i]);
		digitalWrite(strobe, HIGH);
	}
}
void setButtons(uint8_t value, uint8_t position, uint8_t *fixed_numbers) {
	if (fixed_numbers[position] == 0) {
		fixed_numbers[position] = value;
	}
}
uint8_t endchecknum(uint8_t *fixed_numbers) {
	for (int i = 0; i < 8; i++) {
		if (fixed_numbers[i] == 0) {
			return 0;
		}
	}
	return 1;
}
void startcheck(GPIO_T strobe, GPIO_T data, GPIO_T clock, GPIO_T strobe2,
		GPIO_T data2, GPIO_T clock2, uint8_t *wait) {
	uint8_t buttons = 0;
	buttons = readButtons(strobe, data, clock);
	for (uint8_t position = 0; position < 8; position++) {
		uint8_t mask = 0x1 << position;
		if (buttons & mask) {
			*wait = 0;
		}
	}
	buttons = readButtons(strobe2, data2, clock2);
	for (uint8_t position = 0; position < 8; position++) {
		uint8_t mask = 0x1 << position;
		if (buttons & mask) {
			*wait = 0;
		}
	}
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
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
	MX_TIM6_Init();
	/* USER CODE BEGIN 2 */
	setup(strobe, data, clock, strobe2, data2, clock2);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	uint8_t random_number;
	uint8_t displaynum[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t fixed_numbers[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t buttons = 0;
	uint8_t gamemode = 0;
	uint8_t gamemodedisplay[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t gooffdisplay[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t display2flg;
	uint8_t oddplaynum[8];
	uint8_t addplaynum[8];
	uint8_t acontroller[8];
	uint8_t bcontroller[8];
	uint8_t lodisplay1[8] = { 0 };
	uint8_t lodisplay2[8] = { 0 };
	uint8_t babelbuff[8] = { 0 };
	uint8_t wait = 1;

	for (int i = 0; i < 8; i++) {
		if (i % 2 == 0) {
			oddplaynum[i] = SEGMENT_PATTERNS[8];
			addplaynum[i] = 0;
		} else {
			oddplaynum[i] = 0;
			addplaynum[i] = SEGMENT_PATTERNS[8];

		}
	}
	for (int i = 0; i < 8; i++) {
		acontroller[i] = SEGMENT_PATTERNS[10];
		bcontroller[i] = SEGMENT_PATTERNS[11];
	}

	while (1) {
		//demo mode
		while (wait) {
			//game initialize
			gamemode = 0;
			//8 8 8 8 display
			for (int i = 0; i < 5; i++) {
				set7SEG(oddplaynum, strobe, data, clock);
				set7SEG(addplaynum, strobe2, data2, clock2);
				startcheck(strobe, data, clock, strobe2, data2, clock2, &wait);
				HAL_Delay(500);
				set7SEG(addplaynum, strobe, data, clock);
				set7SEG(oddplaynum, strobe2, data2, clock2);
				startcheck(strobe, data, clock, strobe2, data2, clock2, &wait);
				HAL_Delay(500);
			}
			//lo lo lo lo display
			for (int i = 0; i < 8; i += 2) {
				lodisplay1[i] = SEGMENT_PATTERNS[12];
				lodisplay1[i + 1] = SEGMENT_PATTERNS[13];
			}
			for (int i = 0; i < 8; i += 2) {
				lodisplay2[i] = SEGMENT_PATTERNS[13];
				lodisplay2[i + 1] = SEGMENT_PATTERNS[12];
			}
			for (int i = 0; i < 5; i++) {
				set7SEG(lodisplay1, strobe, data, clock);
				set7SEG(lodisplay2, strobe2, data2, clock2);
				HAL_Delay(500);
				set7SEG(lodisplay2, strobe, data, clock);
				set7SEG(lodisplay1, strobe2, data2, clock2);
				HAL_Delay(500);
			}
			//A A A A b b b b display
			for (int i = 0; i < 4; i++) {
				set7SEG(acontroller, strobe, data, clock);
				set7SEG(bcontroller, strobe2, data2, clock2);
				startcheck(strobe, data, clock, strobe2, data2, clock2, &wait);
				HAL_Delay(500);
				set7SEG(gooffdisplay, strobe, data, clock);
				set7SEG(gooffdisplay, strobe2, data2, clock2);
				startcheck(strobe, data, clock, strobe2, data2, clock2, &wait);
				HAL_Delay(500);
			}
		}
		HAL_Delay(1000);
		wait = 1;
		//main routine mode
		while (gamemode <= 5) {
			/* USER CODE END WHILE */

			/* USER CODE BEGIN 3 */

			//read pin & position information
			buttons = readButtons(strobe, data, clock);
			for (uint8_t position = 0; position < 8; position++) {
				uint8_t mask = 0x1 << position;
				if (buttons & mask) {
					setButtons(1, position, fixed_numbers);
				}
			}
			// random number create
			for (int i = 0; i < 8; i++) {
				if (fixed_numbers[i] == 0) {
					random_number = rand() % 7 + 1;
					displaynum[i] = SEGMENT_PATTERNS[random_number];
				}
			}
			// current game mode display
			for (int i = 0; i < 8; i++) {
				gamemodedisplay[i] = SEGMENT_PATTERNS[gamemode];
			}
			set7SEG(displaynum, strobe, data, clock);
			if (display2flg == 0) {
				set7SEG(gamemodedisplay, strobe2, data2, clock2);
				display2flg = 1;
			} else {
				display2flg = 0;
				set7SEG(gooffdisplay, strobe2, data2, clock2);
			}
			//play difficulty
			HAL_Delay(250 - (gamemode * 50));
			if (endchecknum(fixed_numbers)) {
				// display initialize
				uint8_t displaynumend[8] = { 0 };
				// max number define
				int max_count = 0;
				uint8_t most_repeated_num = 0;
				for (int i = 0; i < 8; i++) {
					int count = 1;
					for (int j = i + 1; j < 8; j++) {
						if (displaynum[i] == displaynum[j]) {
							count++;
						}
					}
					if (count > max_count) {
						max_count = count;
						most_repeated_num = displaynum[i];
					}
				}
				for (int i = 0; i < 8; i++) {
					if (displaynum[i] == most_repeated_num) {
						displaynumend[i] = 0;
					} else {
						displaynumend[i] = displaynum[i];
					}
				}
				//current LV & result number display
				for (int i = 0; i < 5; i++) {
					set7SEG(gamemodedisplay, strobe2, data2, clock2);
					set7SEG(displaynum, strobe, data, clock);
					HAL_Delay(500);
					set7SEG(displaynumend, strobe, data, clock);
					set7SEG(gooffdisplay, strobe2, data2, clock2);
					HAL_Delay(500);
				}
				//fix number initialize
				for (int i = 0; i < 8; i++) {
					fixed_numbers[i] = 0;
				}
				//game level UP judge
				if (max_count > 2) {
					gamemode++;
				}
			}
			if (gamemode > 5) {
				// end BABEL mode
				uint8_t babelcount = 0;
				while (babelcount < 4) {
					for (int i = 0; i < 12; i++) {
						for (int j = 0; j < 8; j++) {
							babelbuff[j] = BABEL[j + i];
						}
						set7SEG(babelbuff, strobe, data, clock);
						set7SEG(babelbuff, strobe2, data2, clock2);
						HAL_Delay(300);
					}
					babelcount++;
				}
			}
		}
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief TIM6 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM6_Init(void) {

	/* USER CODE BEGIN TIM6_Init 0 */

	/* USER CODE END TIM6_Init 0 */

	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM6_Init 1 */

	/* USER CODE END TIM6_Init 1 */
	htim6.Instance = TIM6;
	htim6.Init.Prescaler = 16000 - 1;
	htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim6.Init.Period = 100 - 1;
	htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM6_Init 2 */

	/* USER CODE END TIM6_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2,
			GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12 | GPIO_PIN_8, GPIO_PIN_RESET);

	/*Configure GPIO pins : PC0 PC1 PC2 */
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : PB12 PB8 */
	GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_8;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : PC7 */
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
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
