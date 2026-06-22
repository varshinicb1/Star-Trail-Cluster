#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

// Initialize WiFi (non-blocking)
void wifi_init();

// Check WiFi status
bool wifi_is_connected();

// Get current time (formatted)
const char *wifi_get_time();

// Fetch sea-level pressure from weather API
float wifi_get_sea_level_pressure();

// Update NTP time
void wifi_sync_time();

#endif // WIFI_MANAGER_H
