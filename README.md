# DS3Trespasser

This program is a DLL library using Rafzi's hacklib https://bitbucket.org/rafzi/hacklib which is intended to block incoming Steam connections on Dark Souls 3. This is based on the idea behind eur0pa's DSNA program.

Additionally, there's some hardcoded addresses for player structs, since a future implementation (Read: After the DLC brings me back), may encourage me to add additional anti-hacking features.

Settings:
Away on Steam will block all non-friend connection.
Otherwise, all non-blocked and ignored players can join. This means you can blocks players to prevent them from joining your game. As of the earliest implementation, however, blocked players will remain

# USE AT YOUR OWN RISK

## To Compile
In order to compile on Windows, you'll need CMake installed and a copy of hacklib. Trespasser should be placed in the inner src directory of hacklib with the other projects. This should generate a 64-bit Visual Studio solution, which you can the compile to create the necessary DLL. If not, you'll need to manually tweak the settings.
