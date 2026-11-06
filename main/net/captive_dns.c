#include "captive_dns.h"

#include <string.h>
#include <unistd.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

static const char *TAG = "captive_dns";

static TaskHandle_t s_task;
static volatile bool s_run;
static uint32_t s_ip_be; /* network-order IPv4 */

/* Minimal DNS A-record response: copy query header/question, add answer. */
static int build_dns_reply(uint8_t *buf, int qlen, uint32_t ip_be)
{
    if (qlen < 12 || qlen > 512) {
        return -1;
    }
    /* Flags: response, recursion available, no error */
    buf[2] = 0x81;
    buf[3] = 0x80;
    /* Answer count = 1 */
    buf[6] = 0x00;
    buf[7] = 0x01;
    int pos = qlen;
    /* Pointer to name at offset 12 */
    buf[pos++] = 0xC0;
    buf[pos++] = 0x0C;
    /* Type A */
    buf[pos++] = 0x00;
    buf[pos++] = 0x01;
    /* Class IN */
    buf[pos++] = 0x00;
    buf[pos++] = 0x01;
    /* TTL 30s */
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x1E;
    /* RDLENGTH 4 */
    buf[pos++] = 0x00;
    buf[pos++] = 0x04;
    memcpy(&buf[pos], &ip_be, 4);
    pos += 4;
    return pos;
}

static void captive_dns_task(void *arg)
{
    (void)arg;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed");
        s_run = false;
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(53),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "bind :53 failed");
        close(sock);
        s_run = false;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Captive DNS listening on :53");
    uint8_t buf[512];
    while (s_run) {
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        int n = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
        if (n < 0) {
            if (!s_run) {
                break;
            }
            continue;
        }
        int rlen = build_dns_reply(buf, n, s_ip_be);
        if (rlen > 0) {
            sendto(sock, buf, rlen, 0, (struct sockaddr *)&from, fromlen);
        }
    }
    close(sock);
    s_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t captive_dns_start(const char *redirect_ip)
{
    if (s_run) {
        return ESP_OK;
    }
    if (!redirect_ip || !redirect_ip[0]) {
        return ESP_ERR_INVALID_ARG;
    }
    struct in_addr ina;
    if (!inet_aton(redirect_ip, &ina)) {
        return ESP_ERR_INVALID_ARG;
    }
    s_ip_be = ina.s_addr;
    s_run = true;
    if (xTaskCreate(captive_dns_task, "captive_dns", 3072, NULL, 5, &s_task) != pdPASS) {
        s_run = false;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void captive_dns_stop(void)
{
    s_run = false;
    /* Task exits on next recv timeout path; force by closing isn't shared.
     * Best-effort: leave task to die on next packet or reboot after save. */
}
