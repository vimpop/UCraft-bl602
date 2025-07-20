/*
 * Copyright (c) 2020 Bouffalolab.
 *
 * This file is part of
 *     *** Bouffalolab Software Dev Kit ***
 *      (see www.bouffalolab.com).
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of Bouffalo Lab nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <FreeRTOS.h>
#include <task.h>

#include <timers.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <bl_uart.h>
#include <bl_timer.h>

#include <hal_wifi.h>
#include <wifi_mgmr_ext.h>

#include <aos/kernel.h>
#include <aos/yloop.h>

#include <hal_sys.h>
#include <hal_board.h>
#include <hal_boot2.h>
#include <bl_dma.h>
#include <bl_sec.h>
#include <bl_irq.h>
#include <blog.h>
#include <vfs.h>
#include <looprt.h>
#include <event_device.h>
#include <bl_wifi.h>
#include <lwip/tcpip.h>

#include "UCraft.h"

#define SSID "Einstein"
#define PSK "germany101"

static StackType_t aos_loop_proc_stack[4096], ucraft_loop_stack[4096];
static StaticTask_t aos_loop_proc_task, ucraft_loop_task;
static SemaphoreHandle_t xWIFIReadySemaphore;

volatile uint32_t uxTopUsedPriority __attribute__((used)) = configMAX_PRIORITIES - 1;
static wifi_conf_t conf =
    {
        .country_code = "US",
};
static uint32_t time_main;

extern uint8_t _heap_start;
extern uint8_t _heap_size; // @suppress("Type cannot be resolved")
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size; // @suppress("Type cannot be resolved")
static HeapRegion_t xHeapRegions[] =
    {
        {&_heap_start, (unsigned int)&_heap_size}, // set on runtime
        {&_heap_wifi_start, (unsigned int)&_heap_wifi_size},
        {NULL, 0}, /* Terminates the array. */
        {NULL, 0}  /* Terminates the array. */
};
void vAssertCalled(void)
{
    volatile uint32_t ulSetTo1ToExitFunction = 0;

    taskDISABLE_INTERRUPTS();
    puts("\nAssertCalled\n");
    while (ulSetTo1ToExitFunction != 1)
    {
        __asm volatile("NOP");
    }
}
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    puts("Stack Overflow checked\r\n");
    printf("Task Name %s\r\n", pcTaskName);
    while (1)
    {
        /*empty here*/
    }
}

void vApplicationMallocFailedHook(void)
{
    printf("Memory Allocate Failed. Current left size is %d bytes\r\n",
           xPortGetFreeHeapSize());
    while (1)
    {
        /*empty here*/
    }
}

void vApplicationIdleHook(void)
{
#if 0
    __asm volatile(
            "   wfi     "
    );
    /*empty*/
#else
    (void)uxTopUsedPriority;
#endif
}
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
    /* If the buffers to be provided to the Idle task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
    /* If the buffers to be provided to the Timer task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
static void event_cb_wifi_event(input_event_t *event, void *private_data)
{
    switch (event->code)
    {
    case CODE_WIFI_ON_INIT_DONE:
    {
        printf("[APP] [EVT] INIT DONE %lldms\r\n", aos_now_ms());
        wifi_mgmr_start_background(&conf);
    }
    break;
    case CODE_WIFI_ON_MGMR_DONE:
    {
        printf("\n[APP] [EVT] MGMR DONE %lld, now %lums\r\n\n", aos_now_ms(), bl_timer_now_us() / 1000);
        wifi_interface_t wifi_interface;
        wifi_interface = wifi_mgmr_sta_enable();
        printf("[APP][WIFI] Wifi Interface: %p\n", wifi_interface);
        wifi_mgmr_sta_connect(wifi_interface, SSID, PSK, NULL, NULL, 0, 0);
    }
    break;
    case CODE_WIFI_ON_MGMR_DENOISE:
    {
        printf("[APP] [EVT] Microwave Denoise is ON %lld\r\n", aos_now_ms());
    }
    break;
    case CODE_WIFI_ON_SCAN_DONE:
    {
        printf("[APP] [EVT] SCAN Done %lld\r\n", aos_now_ms());
    }
    break;
    case CODE_WIFI_ON_SCAN_DONE_ONJOIN:
    {
        printf("[APP] [EVT] SCAN On Join %lld\r\n", aos_now_ms());
    }
    break;
    case CODE_WIFI_ON_DISCONNECT:
    {
        printf("[APP] [EVT] disconnect %lld, Reason: %s\r\n",
               aos_now_ms(),
               wifi_mgmr_status_code_str(event->value));
    }
    break;
    case CODE_WIFI_ON_CONNECTING:
    {
        printf("[APP] [EVT] Connecting %lld\r\n", aos_now_ms());
    }
    break;
    case CODE_WIFI_CMD_RECONNECT:
    {
        printf("[APP] [EVT] Reconnect %lld\r\n", aos_now_ms());
    }
    break;
    case CODE_WIFI_ON_CONNECTED:
    {
        printf("[APP] [EVT] connected %lld\r\n", aos_now_ms());
    }
    break;
    case CODE_WIFI_ON_PRE_GOT_IP:
    {
        printf("[APP] [EVT] connected %lld\r\n", aos_now_ms());
    }
    break;
    case CODE_WIFI_ON_GOT_IP:
    {
        printf("[APP] [EVT] GOT IP %lld\r\n", aos_now_ms());
        printf("[SYS] Memory left is %d Bytes\r\n", xPortGetFreeHeapSize());
        xSemaphoreGive(xWIFIReadySemaphore);
    }
    break;
    case CODE_WIFI_ON_EMERGENCY_MAC:
    {
        printf("[APP] [EVT] EMERGENCY MAC %lld\r\n", aos_now_ms());
        hal_reboot(); // one way of handling emergency is reboot. Maybe we should also consider solutions
    }
    break;
    case CODE_WIFI_ON_PROV_SSID:
    {
        printf("[APP] [EVT] [PROV] [SSID] %lld: %s\r\n",
               aos_now_ms(),
               event->value ? (const char *)event->value : "UNKNOWN");
    }
    break;
    case CODE_WIFI_ON_PROV_BSSID:
    {
        printf("[APP] [EVT] [PROV] [BSSID] %lld: %s\r\n",
               aos_now_ms(),
               event->value ? (const char *)event->value : "UNKNOWN");
    }
    break;
    case CODE_WIFI_ON_PROV_PASSWD:
    {
        printf("[APP] [EVT] [PROV] [PASSWD] %lld: %s\r\n", aos_now_ms(),
               event->value ? (const char *)event->value : "UNKNOWN");
    }
    break;
    case CODE_WIFI_ON_PROV_CONNECT:
    {
        printf("[APP] [EVT] [PROV] [CONNECT] %lld\r\n", aos_now_ms());
        // printf("connecting to %s:%s...\r\n", ssid, password);
        // wifi_sta_connect(ssid, password);
    }
    break;
    case CODE_WIFI_ON_PROV_DISCONNECT:
    {
        printf("[APP] [EVT] [PROV] [DISCONNECT] %lld\r\n", aos_now_ms());
    }
    break;
    case CODE_WIFI_ON_AP_STA_ADD:
    {
        printf("[APP] [EVT] [AP] [ADD] %lld, sta idx is %lu\r\n", aos_now_ms(), (uint32_t)event->value);
    }
    break;
    case CODE_WIFI_ON_AP_STA_DEL:
    {
        printf("[APP] [EVT] [AP] [DEL] %lld, sta idx is %lu\r\n", aos_now_ms(), (uint32_t)event->value);
    }
    break;
    default:
    {
        printf("[APP] [EVT] Unknown code %u, %lld\r\n", event->code, aos_now_ms());
        /*nothing*/
    }
    }
}
static void aos_loop_proc(void *pvParameters)
{
    vfs_init();
    vfs_device_init();

    aos_loop_init();
    aos_register_event_filter(EV_WIFI, event_cb_wifi_event, NULL);
    printf("Start Wi-Fi fw @%lums\n", bl_timer_now_us() / 1000);
    hal_wifi_start_firmware_task();
    /*Trigger to start Wi-Fi*/
    printf("Start Wi-Fi fw is Done @%lums\n", bl_timer_now_us() / 1000);
    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
    aos_loop_run();
    puts("------------------------------------------\r\n");
    puts("+++++++++Critical Exit From Loop aos_loop_proc++++++++++\r\n");
    puts("******************************************\r\n");
    vTaskDelete(NULL);
}
static void ucraft_loop(void *pvParameters)
{
    xSemaphoreTake(xWIFIReadySemaphore, portMAX_DELAY);
    puts("Entering UCraft_loop\n");
    uint8_t cleanup_flag = 0;
    UCraftStart(&cleanup_flag);
    puts("------------------------------------------\r\n");
    puts("+++++++++Critical Exit From Loop ucraft_loop++++++++++\r\n");
    puts("******************************************\r\n");
    vTaskDelete(NULL);
}
void bfl_main(void)
{

    /*
     * Init UART using pins 16+7 (TX+RX)
     * and baudrate of 2M
     */
    bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
    puts("Starting bl602 now....\r\n");

    vPortDefineHeapRegions(xHeapRegions);
    printf("Heap %u@%p, %u@%p\r\n",
           (unsigned int)&_heap_size, &_heap_start,
           (unsigned int)&_heap_wifi_size, &_heap_wifi_start);
    printf("Boot2 consumed %lums\r\n", time_main / 1000);

    blog_init();
    bl_irq_init();
    bl_sec_init();
    bl_sec_test();
    bl_dma_init();
    hal_boot2_init();

    /* board config is set after system is init*/
    hal_board_cfg(0);
    xWIFIReadySemaphore = xSemaphoreCreateBinary();
    puts("[OS] Starting aos_loop_proc task...\r\n");
    xTaskCreateStatic(aos_loop_proc, (char *)"event_loop", sizeof(aos_loop_proc_stack) / sizeof(StackType_t), NULL, 15, aos_loop_proc_stack, &aos_loop_proc_task);
    puts("[OS] Starting ucraft_loop task...\r\n");
    xTaskCreateStatic(ucraft_loop, (char *)"ucraft_loop", sizeof(ucraft_loop_stack) / sizeof(StackType_t), NULL, 15, ucraft_loop_stack, &ucraft_loop_task);
    puts("[OS] Starting TCP/IP Stack...\r\n");
    tcpip_init(NULL, NULL);
    puts("[OS] Starting OS Scheduler...\r\n");
    vTaskStartScheduler();
}
