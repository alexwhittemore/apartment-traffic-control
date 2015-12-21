# ATC
ATC (Apartment Traffic Control) is an arduino-based traffic light that, with a high degree of inaccuracy, indicates level of intoxication:

  - Green for about two drinks or less
  - Yellow for about four drinks or less
  - Red for three sheets to the wind (or more)

### Requirements
ATC is designed targeting the Arduino Yun, and uses the [SoftTimer](https://github.com/prampec/arduino-softtimer) library to handle task organization. That library can be downloaded and installed either manually from the provided GitHub link, or using the built-in Arduino library manager. Yun was used for the sake of easily pushing new software mid-party, as well as future expansion options.

### Hardware
I'd like to quickly draw up the schematic for the project (Update: see pictures below), but it's terribly simple, anyway: a [Sparkfun MQ-3](https://www.sparkfun.com/products/8880) alcohol gas sensor is connected with a 2.7k pull-up resistor to a stable 5V supply, with the center point attached to A0. A simple external homemade relay shield with 3 channels is used to switch the mains-connected bulbs behind each lens of the traffic light - any external relay board from Amazon/eBay/Seeed, or even a purpose-made relay shield would be suitable by simply assigning the right digital pins.

#### Pictures
The schematic of the sensor circuit and the relays and drivers (the transistor is a 2n2222, the relay is... that 5V relay from Radio Shack):
![schematics](https://raw.githubusercontent.com/alexwhittemore/apartment-traffic-control/master/schematic.jpg)

The light, put together:
![light assembled](https://raw.githubusercontent.com/alexwhittemore/apartment-traffic-control/master/light.jpg)

The tangled mess within:
![wiring](https://raw.githubusercontent.com/alexwhittemore/apartment-traffic-control/master/mess.jpg)

### Future Ideas
One of the benefits of choosing the Yun platform is the network connectivity it provides. In the future, I'd like to implement external features for the hardware platform to coopt the breathalyzer aspect and enable other modes of operation, perhaps related to drinking games. Other possiblities include score indication for a connected beerpong or foosball table.
