#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_netif.h"




/**
 * @brief Configure Wi-Fi or Ethernet, connect, wait for IP
 *
 * This all-in-one helper function is used in protocols examples to
 * reduce the amount of boilerplate in the example.
 *
 * It is not intended to be used in real world applications.
 * See examples under examples/wifi/getting_started/ and examples/ethernet/
 * for more complete Wi-Fi or Ethernet initialization code.
 *
 * Read "Establishing Wi-Fi or Ethernet Connection" section in
 * examples/protocols/README.md for more information about this function.
 *
 * @return ESP_OK on successful connection
 */
esp_err_t example_connect(void);

/**
 * Counterpart to example_connect, de-initializes Wi-Fi or Ethernet
 */
esp_err_t example_disconnect(void);

/**
 * @brief Configure stdin and stdout to use blocking I/O
 *
 * This helper function is used in ASIO examples. It wraps installing the
 * UART driver and configuring VFS layer to use UART driver for console I/O.
 */
esp_err_t example_configure_stdin_stdout(void);

/**
 * @brief Returns esp-netif pointer created by example_connect()
 *
 * @note If multiple interfaces active at once, this API return NULL
 * In that case the get_example_netif_from_desc() should be used
 * to get esp-netif pointer based on interface description
 */
esp_netif_t *get_example_netif(void);

/**
 * @brief Returns esp-netif pointer created by example_connect() described by
 * the supplied desc field
 *
 * @param desc Textual interface of created network interface, for example "sta"
 * indicate default WiFi station, "eth" default Ethernet interface.
 *
 */
esp_netif_t *get_example_netif_from_desc(const char *desc);

#ifdef __cplusplus
}
#endif