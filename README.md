# Welcome to The B.A.T - A Bitcoin Lightning ATM! 

This repository contains the instructions for building and deploying your very own BAT Bitcoin Lightning ATM.

Support the project or buy one here: https://geyser.fund/project/thebatlightningatm 🦇

Support me here: https://www.buymeacoffee.com/cutthepretence 🍺

Join the support group here: https://t.me/BitcoinAutoTeller


The B.A.T, short for Bitcoin Auto Teller, is based on the original [FOSSA](https://github.com/lnbits/fossa "FOSSA") by Ben Arc and directly integrates with LNBits as a funding wallet. It is a highly functional Bitcoin Lightning ATM designed for an efficient, reliable, and convenient digital transactions experience. It has many features, including thermal printer and Over-The-Air (OTA) update capability, a large screen and enhanced user customisation options. The B.A.T is designed to sit in between a hobbyists dream and commercial environment.

![](https://i.ibb.co/x314MPv/Photo-Room-20230810-234927.png)

### Components:

Before starting, please gather the following components that will be required for this project:

12V DC adapter (Power Source) such as [THIS](https://www.ebay.co.uk/itm/225019408019?mkcid=16&mkevt=1&mkrid=711-127632-2357-0&ssspo=r9ua64hcr--&sssrc=2047675&ssuid=M7LGxC1yShC&var=523896245140&widget_ver=artemis&media=COPY "THIS")

12V/5V converter such as [THIS](https://www.ebay.co.uk/itm/193727621572 "THIS")

12V TTL Thermal Printer (You also need the USB to serial cable for uploading the Logo) [THIS](https://www.aliexpress.com/item/4001275848142.html?channel=twinner "THIS")

NV10 USB+ Note acceptor [THIS](https://www.innovative-technology.com/products/products-main/127-nv10-usb "THIS")

DG600F Coin acceptor [THIS](https://www.aliexpress.com/item/32951358676.html?channel=twinner "THIS")

ZX7D00CE01 7-inch screen with integrated ESP32-S3 (You also need the tool for burning) - [THIS](https://www.aliexpress.com/item/1005005000593458.html?spm=a2g0o.order_list.order_list_main.99.4f051802XMsYg2 "THIS")

Momentary 3.3v button (with a harness) such as [THIS](https://www.ebay.co.uk/itm/255737525408 "THIS")

Power 12v button (with a harness) such as [THIS](https://www.ebay.co.uk/itm/255737525408 "THIS")

5.5mm x 2.1mm Female Male DC Power Plug Adapter such as [THIS](https://www.aliexpress.com/item/32990194795.html?channel=twinner "THIS")

5.1mm X 2.1mm female power connector such as [THIS](https://www.aliexpress.com/item/1005003494724471.html?channel=twinner "THIS")

5.1 X 2.1mm Male to Female dual splitter such as [THIS](https://www.aliexpress.com/item/4000552846445.html?channel=twinner "THIS")

HW-307 Relay module or similar such as [THIS](https://www.ebay.co.uk/itm/225559844989?mkcid=16&mkevt=1&mkrid=711-127632-2357-0&ssspo=cZ33k8PfR2e&sssrc=2047675&ssuid=M7LGxC1yShC&widget_ver=artemis&media=COPY "THIS")

Coin Acceptor LED Surround such as [THIS](https://www.aliexpress.com/item/1005002995360644.html?channel=twinner  "THIS")

### Programming the Thermal Printer:

Start by downloading the printer setting software available here .

• First, you need to ensure that the printer is set to a 19200 baud rate.

• Now, open the printer setting software and navigate to the "Upload" section.

• Select the "printer_logo.bmp" file for upload.

• Proceed to upload the "printer_logo.bmp" file to your thermal printer.

Remember, it is crucial to use the precisely labeled "printer_logo.bmp" file and to set the printer to the 19200 baud rate for successful functioning.

![](https://i.ibb.co/n62Wbxv/Photo-Room-20230811-001006-1.png)

### Programming The DG600F Coin Acceptor:

• Start by holding down the "A" button. Keep it pressed.

• While still pressing the "A" button, press the "A" button again and then hit the "B" button. Do this for the specific coin you want to program (for instance, 5p).

• Continue pressing the buttons for the designated coin until you hear a beep. This usually happens after 20 presses. For accuracy, it is best to use a variety of the same coin.

• Repeat steps 1-3 for all six coins that you want to program.

• Now, press and hold the "B" button, then press the "A" button to switch to the A2 setting. If not sure, the default setting is A1.

• Follow this configuration: Set A2 to 02. Set A3, A4, and A5 to 01.

• Finally, press and hold the "A" button to exit the programming mode.

Remember, each coin needs individual programming to ensure the successful functioning of the DG600F coin acceptor.


### Programming the NV10 USB+ Note Acceptor:

• Download the Validator Manager and Drivers suitable for your specific currency from the respective website.

• After successful download, restart your system for changes to take effect.

• Plug the cable of the NV10 USB+ into your PC.

• Navigate to your device manager and ensure that your PC detects the connected NV10 USB+.

• Next, open the Validator Manager software and locate the programming option. Select it.

• In this section, choose the 'bv1' file to program the device.

• Modify the user mode to 'advanced' setting for additional features.

• In the interface section, set the parameter to 'SIO'.

• Now, enable the 'Credit card rejection' option to ensure the machine doesn't process invalid entries.

• Once all changes are completed, ensure to save and apply these new parameters.

### Programming the 7-inch Screen with Integrated ESP32S3 using Arduino:

• Begin by launching your Arduino IDE.

• Connect  integrated screen to your PC and install relevant drivers.

• Navigate to the 'Tools' tab on the menu bar.

• In the dropdown menu, select the options as shown in the image provided:

![](https://i.ibb.co/8Ngrb1Q/settings.png)

Alternatively, use a buring tool and the .BIN file provided to install the firmware.


### How to build:
• Begin by connecting the 5.1mm power connector to your power switch harness.

• Identify the green cable on your power switch harness and connect it to the negative (-) terminal on your 12V/5V converter.

• Take the black cable from the C terminal of your switch and connect it to the red (+ve) cable on your incoming 5.1mm power supply.

• Connect your NO (blue) cable, along with the yellow cable from the switch, to the positive (+ve) terminal on your 12V/5V converter.

• Depending on the relay's requirements, connect the VCC pin on your relay module to either a 5V or 3.3V power supply.

• Connect the GND pin on your relay module to the ground (GND) of both your power supply and ESP32.

• The IN pin on your relay module should be connected to one of your ESP32's GPIO pins. In this case, use GPIO2.

• The positive terminal (+) of each of your devices should be connected directly to the Normally Open (NO) pin on your relay module.

• Specific Device Connections:

• For the Thermal Printer, connect the TX pin to GPIO42, the RX pin to GPIO19, and the GND to the GND of the ESP32.

• For the NV10 USB+, connect the TX pin to GPIO4 and the RX pin to GPIO1.

• For the DG600F coin acceptor, connect the signal pin to GPIO20 and the inhibit pin to GPIO41. Make sure to enable dips switch 2 for secure mode, and dips switch 3 for your serial mode.

• For the 7-inch screen, connect its cable to a USB serial converter through a USB cable.

### Final Connections:

• Connect the negative terminal (-) of each device directly to the ground (GND or -) of your power supply.

• Connect the Common (COM) pin of your relay module to the positive (+) terminal of your power supply.

For the momentary button (Please verify for your button type - these can differ from button to button):

Connect the black wire to ESP32 GND
Connect the red wire to the +3.3V pin on the ESP32
Connect the blue and yellow wires to the GPIO40 pin

# How to use:
### Booting into Configuration Mode:
When you want to configure your B.A.T. device, you'll need to launch the access point. To do this, press the boot button on the burner cable. The access point will have a name similar to "The B.A.T - {mac address}".

Once you've connected to the access point, you can begin setting up the device.

### Configuration Options:

Network and wallet configuration, You can change several parameters:

- Access point password: Choose a password to secure your device's access point.
- LNURLDevice string: Set the connection string for funding. This can be from lnbits.com or self-hosted.
- Live rate API URL: Set an API endpoint for retrieving live rates. e.g., https://legend.lnbits.com/lnurlp/api/v1/rate/GBP.
- Wallet endpoint URL: Set the endpoint URL for wallet actions. e.g., https://legend.lnbits.com/api/v1/wallet.
- API Key: Input your API Key, such as 69f5a4afe7834ddsj3jd7dj4f44cd82.
- Acceptor and Fiat Settings:
- Enable or disable accepting notes or coins.
- Set coin and bill values for the acceptors.
- Set a maximum deposit value for each transaction.
- Set a fee rate.
- You can set a redemption period for each transaction when the thermal printer is in use.

### Custom Options:
- Turn off/on the LED surround for the coin acceptor.
- Enable/disable displaying the live rate.
- Set a balance check after each transaction.
- QR Output Settings:
- Set the output display options of the transaction QR codes – it could be the screen, print, or both.
- You can decide to display Bitcoin quotes or jokes on each receipt – it would be selected randomly each time.
- You can set a URL of your website or LNBits install URL that prints on the receipt for promotional purposes.
- The maximum number of receipts that can be printed on the thermal printer can be set.

### Network Time Protovol (NTP) Settings:

- Configure your timezone, NTP server, and sync intervals for accurate timestamps on printed receipts.
- A time offset can be set for regions observing daylight saving.
- Display Settings:
- Set the color mode of your display: default grafitti, stealth black and white, or set your custom color options.

### Translation Settings:

- Set the default and second language (with string translations) for your device. A button press when the ATM is in use will change the language.

### Updating Firmware:
- The B.A.T device allows firmware updates via OTA using .BIN files. To apply a new firmware update, navigate to the Update section in the access point, and proceed to flash the new release of the firmware.

------------

![](https://i.ibb.co/bLwT21k/BULL.png)
