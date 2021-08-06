### How to compile
Follow this article https://zigdevwiki.github.io/Begin/IAR_install/

## Version 1
![](/images/DIYRuZ_Flower_10.png)
A lot of time has passed since the appearance of Mi Flora, but Jager never dared to purchase it. First of all, the fact that it was Bluetooth stopped, and this automatically created problems on a scale at home and the second reason was the cost, taking into account the fact that Jager needed 20-30 pieces. The decision, do it by myself. Taking into account the development of his home Zigbee network, the proven E18-MS1-PCB module was taken as a basis. The measurement of soil moisture is carried out using a capacitive method, the sensor electrodes are isolated from the soil and therefore do not corrode. The circuit for measuring soil moisture is typical for this kind of device and, in principle, repeats the Chinese sensor, with the only difference that Jager increased the frequency of the generator to 2 MHz.
![](/images/DIYRuZ_Flower_4.png)

Also on the board, there is optional wiring of the light sensor, temperature sensor DS18B20 (works up to voltage 2.7 volts ), temperature, humidity and atmospheric pressure sensor BME280 as a separate chip or as a ready-made module.

The board was designed in EasyEDA  
![](/images/DIYRuZ_Flower_7.png)
![](/images/DIYRuZ_Flower_6.png)

The sensor can be powered either from two AAA batteries (note that there are externally identical holders but with different pole orientations) or from one CR2032 type element.
![](/images/DIYRuZ_Flower_11.png)

It is possible to use the E18-MS1-PCB module
![](/images/DIYRuZ_Flower_8.png)
or the universal module from Jager
![](/images/DIYRuZ_Flower_9.png)

As you can see, the boards in the photos are slightly different from the design image. The fact is that this is the first version of the boards, the second was supplemented with the ability to use a ready-made BME280 module since not everyone can cope with sealing the "bare" chip. In addition, a more convenient connector for CC2530  
![](/images/DIYRuZ_Flower_5.png)  
firmware has been brought out. For reference, the BME280 size next to the 0805 resistors   
![](/images/DIYRuZ_Flower_19.png)

To measure the temperature of the soil (important in greenhouses) or water, you can solder the DS18B20 in a metal sleeve  
![](/images/DIYRuZ_Flower_13.png)

Since the board has several sensors, it can be used as, for example, an outdoor temperature/humidity sensor by cutting off the soil sensor.  
![](/images/DIYRuZ_Flower_17.png)  

Replacing alkaline batteries with Ni-MH batteries, for example, has a chance to survive the most severe frosts.

The device consumes 1.1μA in sleep mode and 26mA at the time of data transfer. During the tests, the device worked for 5 days with a measurement and transmission interval of 1 minute, which did not affect the AAA battery voltage in any way. Taking into account the fact that the device remains operational when the voltage drops to 1.8 volts and the measurement interval is increased to 30 minutes, the batteries will last for several years.

For this sensor @anonymass wrote an open-source firmware and converter for zigbee2mqtt. The firmware contains compensation for the values ​​of the level of humidity and illumination associated with a drop in the supply voltage. This sensor is also supported in the SLS Gateway It looks like this
![](/images/DIYRuZ_Flower_14.png)

![](/images/DIYRuZ_Flower_15.png)  
In general, the sensor turned out to be very sensitive, when the window is open if it is raining outside, the sensor feels like dry ground absorbs moisture. The graph shows that you need to select a resistor in the light sensor circuit.  
![](/images/DIYRuZ_Flower_16.png)  
This data is quite enough to signal the need for watering, as well as, for example, turn on the drip irrigation system in the greenhouse. In Jager's opinion, there is no need to solder on each BME280 sensor, as it is redundant.

## Version 2

Not much time has passed since the publication of the article about the Zigbee soil moisture sensor on the CC2530 chip, as the second version arrived. In this version, the total number of parts is reduced and the assembly is accordingly simplified. Through the efforts of @anonymass, it turned out that the cc2530 can generate a signal on its GPIOs with a frequency of up to 3 MHz, which is quite enough to exclude an external generator on the TLC555 chip from the circuit.

The updated schema now looks like this  
![](/images/DIYRuZ_Flower_V2-1.png)

Accordingly, the board has become even more simplified.  
![](/images/DIYRuZ_Flower_V2-2.png)
![](/images/DIYRuZ_Flower_V2-3.png)
![](/images/DIYRuZ_Flower_V2-5.png)

Those who managed to order the boards of the first option shouldn't worry too much, it is enough not to unsolder the elements marked with red crosses and add one jumper marked in green.  
![](/images/DIYRuZ_Flower_V2-4.png)  

Open source firmware wrote by @anonymass. Since release 1.0.9, the firmware is universal, suitable for both sensor options.

## Housing
Mini housing by ftp27  
![](/images/housing_mini.jpg)

Mushroom housing by Bacchus777  
![](/images/housing_mushroom.jpg)

## How to join:
### If device in FN(factory new) state:
1. Press and hold button (1) for 2-3 seconds, until device start flashing led
2. Wait, in case of successful join, the device will flash LED 5 times
3. If join failed, the device will flash LED 3 times

### If the device is in a network:
1. Hold button (1) for 10 seconds, this will reset the device to FN(Factory New) status
2. Go to step 1 for FN device

## Troubleshooting

If a device does not connect to your coordinator, please try the following:

1. Power off all routers in your network.
2. Move the device near to your coordinator (about 1 meter).

or if you cannot disable routers (for example, internal switches), you may try the following:

1. Disconnect an external antenna from your coordinator.
2. Move a device to your coordinator closely (1-3 centimetres).
3. Power on, power on the device.
4. Restart your coordinator (for example, restart Zigbee2MQTT if you use it).

## Other checks

Please, ensure the following:

1. Your power source is OK (a battery has more than 3V). You can temporarily use an external power source for testings (for example, from a debugger).
2. The RF part of your E18 board works. You can upload another firmware to it and try to pair it with your coordinator. Or you may use another coordinator and build a separate Zigbee network for testing.
3. Your coordinator has free slots for direct connections.
4. You permit joining on your coordinator.
5. Your device did not join another opened Zigbee network. When you press and hold the button, it should flash every 3-4 seconds. It means that the device is joining.

### Files to reproduce
* [v2 Gerbers and BOM](https://github.com/diyruz/flower/hardware/v2) by [Jager](https://t.me/Jager_f)  

* [Firmware](https://github.com/diyruz/flower/releases) by [@anonymass](https://t.me/anonymass)  

* [Mini housing](https://www.thingiverse.com/thing:4722125) by [ftp27](https://www.thingiverse.com/ftp27)  

* [Mushroom housing](https://www.thingiverse.com/thing:4629055) by [Bacchus777](https://www.thingiverse.com/Bacchus777)

* [OLD v1 Gerbers and BOM](https://github.com/diyruz/flower/hardware/v1) by [Jager](https://t.me/Jager_f)  


[Original v1 post by Jager](https://modkam.ru/?p=1671)

[Original v2 post by Jager](https://modkam.ru/?p=1700)
