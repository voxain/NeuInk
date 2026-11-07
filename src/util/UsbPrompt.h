#pragma once

// Asking to enter USB file transfer mode when a computer is plugged in. A charger never triggers it (see
// HalGPIO::isUsbHostConnected). It asks once per connection: declining, or leaving USB mode, silences it until
// the cable has been out for a while, which also covers the restart that leaving USB mode makes.
namespace UsbPrompt {

// Call every loop with whether a host is connected; true once, when the prompt should be shown.
bool shouldPrompt(bool hostConnected);

// The user said no, or left USB mode: do not ask again for this connection.
void suppress();

}  // namespace UsbPrompt
