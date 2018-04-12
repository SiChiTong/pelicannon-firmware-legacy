# Pelicannon NXP FRDM-K66F Firmware
- Part of the [Pelicannon Project][pelicannon]
- Runs on an [NXP FRDM-K66F development board][k66f]
- Reads and synchronizes IMU data and sends it to Jetson TK1 (ninedof.c and tk1.c)
- Takes commands from the Jetson TK1 to rotate or fire the nerf gun (tk1.c and TODO.c)

[pelicannon]: https://github.com/csvance/pelicannon
[k66f]: https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/kinetis-cortex-m-mcus/k-seriesperformancem4/k6x-ethernet/freedom-development-platform-for-kinetis-k66-k65-and-k26-mcus:FRDM-K66F