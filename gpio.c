#include "gpio.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <gpiod.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int find_gpiochip_for_offset(const int line_offset, char** chip_dev_out)
{
    DIR* d = opendir("/sys/class/gpio");
    if (!d)
    {
        perror("opendir(/sys/class/gpio)");
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL)
    {
        const char* name = entry->d_name;

        /* only gpiochipN directories */
        if (strncmp(name, "gpiochip", 8) != 0)
            continue;

        /* ensure at least one digit follows */
        if (!isdigit((unsigned char)name[8]))
            continue;

        /* read ngpio */
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "/sys/class/gpio/%s/ngpio", name);
        FILE* f = fopen(path, "r");
        if (!f)
            continue;

        int ngpio;
        if (fscanf(f, "%d", &ngpio) != 1)
        {
            fclose(f);
            continue;
        }
        fclose(f);

        /* is our line_offset in range? */
        if (line_offset < ngpio)
        {
            /* now find the chip ID under device/ */
            snprintf(path, sizeof(path), "/sys/class/gpio/%s/device", name);
            DIR* devd = opendir(path);
            if (!devd)
            {
                closedir(d);
                errno = ENOENT;
                return -1;
            }

            struct dirent* devent;
            int found = 0;
            while ((devent = readdir(devd)) && !found)
            {
                if (strncmp(devent->d_name, "gpiochip", 8) == 0)
                {
                    const char* ns = devent->d_name + 8;
                    char* endptr;
                    long chip_id = strtol(ns, &endptr, 10);
                    if (*endptr == '\0')
                    {
                        /* got it—build "/dev/gpiochipY" */
                        *chip_dev_out = malloc(16);
                        if (!*chip_dev_out)
                        {
                            closedir(devd);
                            closedir(d);
                            errno = ENOMEM;
                            return -1;
                        }
                        snprintf(*chip_dev_out, 16,
                                 "/dev/gpiochip%ld", chip_id);
                        found = 1;
                    }
                }
            }

            closedir(devd);
            closedir(d);
            if (found)
                return 0;
            errno = ENOENT;
            return -1;
        }
    }

    closedir(d);
    errno = ENOENT;
    return -1;
}

struct gpio_handle* gpio_init_handle(const char* chip_path, const unsigned int offset)
{
    struct gpio_handle* h = malloc(sizeof(*h));
    if (!h)
    {
        perror("malloc");
        return NULL;
    }

    h->chip = gpiod_chip_open(chip_path);
    if (!h->chip)
    {
        perror("gpiod_chip_open");
        free(h);
        return NULL;
    }

    h->line = gpiod_chip_get_line(h->chip, offset);
    if (!h->line)
    {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(h->chip);
        free(h);
        return NULL;
    }

    char consumer[32];
    snprintf(consumer, sizeof(consumer), "lpc21isp-%d", offset);

    if (gpiod_line_request_output(h->line, consumer, 0) < 0)
    {
        perror("gpiod_line_request_output");
        gpiod_chip_close(h->chip);
        free(h);
        return NULL;
    }

    return h;
}

/**
 * Set the line high (value=1) or low (value=0).
 */
int gpio_set(const struct gpio_handle* h, const int value)
{
    int ret = gpiod_line_set_value(h->line, value);
    if (ret < 0)
        perror("gpiod_line_set_value");
    return ret;
}

/**
 * Release resources.
 */
void gpio_release(struct gpio_handle* h)
{
    gpiod_line_release(h->line);
    gpiod_chip_close(h->chip);
    free(h);
}
