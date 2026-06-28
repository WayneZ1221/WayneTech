/**
  ******************************************************************************
  * @file    bsp_key.c
  * @author  fire (refactored debounce)
  * @version V2.0
  * @date    2015-xx-xx
  * @brief   Key scan with state-machine debounce (non-blocking)
  ******************************************************************************
  * @attention
  *
  * Platform: Wildfire STM32 F407 development board
  * Forum   : http://www.firebbs.cn
  * Taobao  : https://fire-stm32.taobao.com
  *
  * Debounce strategy:
  *  - State machine per key (IDLE -> DEBOUNCE -> CONFIRMED_PRESS -> ... -> IDLE)
  *  - Caller must poll Key_Scan() periodically (e.g. every 5-10 ms from a task)
  *  - A key press is only confirmed after DEBOUNCE_CNT_MAX consecutive stable
  *    readings of the pressed level. The release edge is also debounced.
  *  - Fully non-blocking: returns immediately every call.
  *
  ******************************************************************************
  */

#include "bsp_key.h"

/* --------------------------------------------------------------------------
 * Debounce configuration
 * -------------------------------------------------------------------------- */
#define DEBOUNCE_CNT_MAX   6U    /* consecutive stable samples to confirm
                                    e.g. 6 * 5ms poll period = ~30ms debounce */

#define KEY_SLOT_MAX       4U    /* max distinct keys tracked simultaneously */
#define KEY_SLOT_INVALID   0xFFU /* sentinel: empty slot */

/* Key state machine states
 *
 *   IDLE ──(level==ON)──> DEBOUNCE ──(cnt>=MAX)──> PRESSED
 *     ^                      |                        |
 *     |                   (level!=ON)                 (level==OFF)
 *     |                      v                        v
 *     +──(cnt>=MAX)── WAIT_RELEASE <──(cnt>=MAX)── DEBOUNCE_RELEASE
 *
 * Key_Scan returns KEY_ON only once on the IDLE→PRESSED transition.
 */
typedef enum {
    KEY_ST_IDLE              = 0,
    KEY_ST_DEBOUNCE_PRESS    = 1,
    KEY_ST_PRESSED           = 2,
    KEY_ST_DEBOUNCE_RELEASE  = 3,
    KEY_ST_WAIT_RELEASE      = 4
} KeyState_t;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    KeyState_t    state;
    uint8_t       count;         /* consecutive stable-level counter */
} KeySlot_t;

/* Per-key state array — initialized to zero (IDLE, KEY_SLOT_INVALID) */
static KeySlot_t key_slots[KEY_SLOT_MAX];

/* ---- internal helper: find or allocate a slot --------------------------- */
static KeySlot_t* key_find_slot(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint8_t i;
    uint8_t first_empty = KEY_SLOT_INVALID;

    for (i = 0; i < KEY_SLOT_MAX; i++) {
        if (key_slots[i].port == GPIOx && key_slots[i].pin == GPIO_Pin) {
            return &key_slots[i];          /* already registered */
        }
        if (first_empty == KEY_SLOT_INVALID && key_slots[i].port == 0) {
            first_empty = i;               /* remember first free slot */
        }
    }

    /* Not found — allocate a new slot */
    if (first_empty != KEY_SLOT_INVALID) {
        key_slots[first_empty].port  = GPIOx;
        key_slots[first_empty].pin   = GPIO_Pin;
        key_slots[first_empty].state = KEY_ST_IDLE;
        key_slots[first_empty].count = 0;
        return &key_slots[first_empty];
    }

    return 0; /* no slot available */
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/**
  * @brief  Initialize key GPIOs (input, no pull)
  * @param  None
  * @retval None
  */
void Key_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIO clocks */
    RCC_AHB1PeriphClockCmd(KEY1_GPIO_CLK | KEY2_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;

    /* KEY1 */
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN;
    GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStructure);

    /* KEY2 */
    GPIO_InitStructure.GPIO_Pin = KEY2_PIN;
    GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStructure);
}

/**
  * @brief  Non-blocking key scan with state-machine debounce.
  *         Must be called periodically (recommended: every 5..10 ms).
  *         Each call returns immediately.
  * @param  GPIOx   : GPIO port (GPIOA..GPIOK)
  * @param  GPIO_Pin: pin mask (GPIO_Pin_0..GPIO_Pin_15)
  * @retval KEY_ON  : a confirmed key-press event was just detected
  *         KEY_OFF : no (new) press event
  *
  * Timing diagram for a single press-and-release cycle:
  *
  *   physical signal:  _|-|_|-|_________|--|_|-|_|-|__________
  *                       bounce          |   bounce
  *                                    press          release
  *
  *   state machine:   IDLE -> DEBOUNCE -> PRESSED -> DEBOUNCE_RELEASE -> IDLE
  *   return value:              KEY_ON^                                (silent)
  */
uint8_t Key_Scan(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    KeySlot_t *slot = key_find_slot(GPIOx, GPIO_Pin);
    if (slot == 0) return KEY_OFF;

    uint8_t raw_level = (GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) == KEY_ON)
                        ? 1U : 0U;
    uint8_t result = KEY_OFF;

    switch (slot->state) {

    /* ----- IDLE: waiting for a key press -------------------------------- */
    case KEY_ST_IDLE:
        if (raw_level == 1U) {
            slot->state = KEY_ST_DEBOUNCE_PRESS;
            slot->count = 1;
        }
        break;

    /* ----- DEBOUNCE_PRESS: verify the press is stable ------------------- */
    case KEY_ST_DEBOUNCE_PRESS:
        if (raw_level == 1U) {
            slot->count++;
            if (slot->count >= DEBOUNCE_CNT_MAX) {
                slot->state = KEY_ST_PRESSED;
                slot->count = 0;
                result = KEY_ON;   /* confirmed press — fire once */
            }
        } else {
            /* Bounce or glitch — reset back to idle */
            slot->state = KEY_ST_IDLE;
            slot->count = 0;
        }
        break;

    /* ----- PRESSED: key held, waiting for release ----------------------- */
    case KEY_ST_PRESSED:
        if (raw_level == 0U) {
            slot->state = KEY_ST_DEBOUNCE_RELEASE;
            slot->count = 1;
        }
        /* held — stay in PRESSED, no event */
        break;

    /* ----- DEBOUNCE_RELEASE: verify the release is stable --------------- */
    case KEY_ST_DEBOUNCE_RELEASE:
        if (raw_level == 0U) {
            slot->count++;
            if (slot->count >= DEBOUNCE_CNT_MAX) {
                slot->state = KEY_ST_IDLE;
                slot->count = 0;
                /* Key fully released; ready for next press */
            }
        } else {
            /* Bounced back high — still pressed */
            slot->state = KEY_ST_PRESSED;
            slot->count = 0;
        }
        break;

    /* ----- WAIT_RELEASE (reserved for long-press/repeat scenarios) ------ */
    case KEY_ST_WAIT_RELEASE:
        if (raw_level == 0U) {
            slot->state = KEY_ST_DEBOUNCE_RELEASE;
            slot->count = 1;
        }
        break;

    default:
        slot->state = KEY_ST_IDLE;
        slot->count = 0;
        break;
    }

    return result;
}

/*********************************************END OF FILE**********************/
