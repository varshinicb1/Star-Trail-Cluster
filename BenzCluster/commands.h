#ifndef COMMANDS_H
#define COMMANDS_H

// Shared device-command handler used by BOTH the BLE write characteristic and
// the HTTP API, so the app gets identical behaviour over either transport.
//
// Supported commands (key or key=value):
//   attitude_style=N   select attitude indicator style 0-3 (persisted)
//   factory_zero       capture mount-tilt orientation zero (persisted)
//   factory_magcal=S   run figure-8 magnetometer cal for S seconds (persisted)
//   declination=F      set magnetic declination in degrees (persisted)
//   debug_sensors      reply with raw+fused sensor values
//
// Returns true if the command was recognised. `reply` (may be empty) is a
// short human-readable result for HTTP body / BLE notify.
bool cluster_handle_command(const char *cmd, char *reply, int replyLen);

// Load persisted attitude style at boot (call after attitude_init()).
void cluster_load_attitude_style();

#endif // COMMANDS_H
