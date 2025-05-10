#pragma once
#include <gpiod.h>

struct gpio_handle {
    struct gpiod_chip *chip;
    struct gpiod_line *line;
};

/**
 * Find which /dev/gpiochipX contains a given line offset.
 *
 * @param line_offset  The GPIO line offset within its chip (0-based).
 * @param chip_dev_out On success, set to a malloc()ed string "/dev/gpiochipY".
 *                     Caller must free().
 * @return             0 on success, -1 on failure (not found or I/O error).
 */
int find_gpiochip_for_offset(const int line_offset, char **chip_dev_out);

/**
 * Initialize a GPIO line for output.
 *
 * @param chip_path  e.g. "/dev/gpiochip0"
 * @param offset     line offset on that chip
 * @return           pointer to gpio_handle (free with gpio_release), or NULL on error
 */
struct gpio_handle *gpio_init_handle(const char *chip_path, unsigned int offset);

/**
 * Set the line high (value=1) or low (value=0).
 */
int gpio_set(const struct gpio_handle *h, int value);

/**
 * Release resources.
 */
void gpio_release(struct gpio_handle *h);
