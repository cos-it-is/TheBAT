# Welcome to The B.A.T - A Bitcoin Lightning ATM! 

This repository contains the instructions for building and deploying your very own B.A.T Bitcoin Lightning ATM.

Support the project or buy one here: https://geyser.fund/project/thebatlightningatm or https://satsbat.com 🦇

Support me here: https://www.buymeacoffee.com/cutthepretence 🍺

Join the support group here: https://t.me/BitcoinAutoTeller

![](https://i.ibb.co/r4MHh2r/Photo-Room-20240103-000242.png)

The B.A.T, short for Bitcoin Auto Teller, is based on the original [FOSSA](https://github.com/lnbits/fossa "FOSSA") by Ben Arc and directly integrates with LNBits as a funding wallet. 
It is the first highly functional Bitcoin Lightning ATM to incorporate NFC withdrawls that support LNURL, LN Address and [Boltcard](https://www.boltcard.org/ "Boltcard"). It has many features, including thermal printer, NFC withdrawl, and Over-The-Air (OTA) update capability, a large screen and enhanced user customisation options. The B.A.T is designed to sit in between a hobbyists dream and commercial environment.
It was born from a vision to creatly a very useable, small and portable ATM that can spark Bitcoin circular economies anywhere. 
It can be taken to events, markets, shops, parties. All you need is a power supply (plug or battery) and a wifi connection (for some options such as NFC).
It does however, work completely offline.
The ATM also functions as a straight paper Gift voucher dispenser, adding a completely secondary and unique function.

![](https://i.ibb.co/gzDjzWB/Photo-Room-20240103-220857.png)


### Trademark

"The BAT" is a registered trademark. You are welcome to hack, fork, build, and use the source code and instructions found in this repository. However, the right to use the name "The BAT" or logo with any products or services is withheld and reserved for the trademark owner. Please be respectful and utilise your own logo/brand.

### Components:

Before starting, please gather the following components that will be required for this project:

12V 3A DC adapter (Power Source) such as [THIS](https://www.ebay.co.uk/itm/225019408019?mkcid=16&mkevt=1&mkrid=711-127632-2357-0&ssspo=r9ua64hcr--&sssrc=2047675&ssuid=M7LGxC1yShC&var=523896245140&widget_ver=artemis&media=COPY "THIS")

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

Coin Acceptor LED Surround such as [THIS](https://www.aliexpress.com/item/1005002995360644.html?channel=twinner "THIS")

NV10 Host Cable - needed to change currency datasets or custom options on the acceptor [THIS](https://innovative-technology.com/shop/cables/nv9-nv10-usb-host-cable-detail "THIS") 


### Programming the Thermal Printer:

Start by downloading the printer setting software available [THIS](https://www.hsprinter.com/?m=common&a=down&name=639a91a94e329.zip&g=e "THIS") .

• First, you need to ensure that the printer is set to a 19200 baud rate.

• Now, open the printer setting software and navigate to the "Upload" section.

• Select your own printer_logo.bmp file for upload. This should be in greyscale and around 270x270 pixels.

• Proceed to upload the "printer_logo.bmp" file to your thermal printer.

Remember, it is crucial to use the precisely labeled "printer_logo.bmp" file and to set the printer to the 19200 baud rate for successful functioning.




### Programming The DG600F Coin Acceptor:

• Start by holding down the "A" button. Keep it pressed.

• While still pressing the "A" button, press the "A" button again and then hit the "B" button. Do this for the specific coin you want to program (for instance, 5p).

• Continue pressing the buttons for the designated coin until you hear a beep. This usually happens after 20 presses. For accuracy, it is best to use a variety of the same coin.

• Repeat steps 1-3 for all six coins that you want to program.

• Now, press and hold the "B" button, then press the "A" button to switch to the A2 setting. If not sure, the default setting is A1.

• Follow this configuration: Set A2 to 02. Set A3, A4, and A5 to 01.

• Finally, press and hold the "A" button to exit the programming mode.

There are instructions in the manual and many youtube videos of this process available.


### Programming the NV10 USB+ Note Acceptor:

• Download the Validator Manager and Drivers suitable for your specific currency from the Innovative Technologies website (Create a free account).

• Install and plug the cable of the NV10 USB+ into your PC using the host cable cable for programming

• Navigate to your device manager and ensure that your PC detects the connected NV10 USB+ using the instructions in the manual.

• Next, open the Validator Manager software and locate the programming option. Select it.

• In this section, choose the 'bv1' file to program the device.

• Modify the user mode to 'advanced' setting for additional features.

• In the interface section, set the parameter to 'SIO'.

• Now, enable the 'Credit card rejection' option to ensure the machine doesn't process invalid entries.

• Once all changes are completed, ensure to save and apply these new parameters.


### Programming the 7-inch Screen with Integrated ESP32S3 using Arduino:

• Begin by launching your Arduino IDE.

• Connect integrated screen to your PC and install relevant drivers and arduino libraries.

• Navigate to the 'Tools' tab on the menu bar.

• In the dropdown menu, select the options as shown in the image provided:

![](https://i.ibb.co/tzXJSHV/settings.png)

Alternatively, use a buring tool and the .BIN file provided in releases to install the firmware.


### How to build:
• Connect the devices together as per the wiring diagram. You can use Dupont wires or for the board, you can use a 2x40 (40 pin) IDC connector to breakout the pins. This is a 2mm pitch (not common 2.54mm)

# How to use:

### Booting into Configuration Mode:
When you want to configure your B.A.T. device, you'll need to launch the access point. To do this, press the boot button on the burner cable during boot. The access point will have a name similar to "The B.A.T - {mac address}".

Once you've connected to the access point, you can begin setting up the device.

This can sometimes be an issue when using OTA updates. It is best (on android at least) I have found using edge browser to perform OTA updates.

### Configuration Options:

Network and wallet configuration, You can change several parameters:

### General Options:

- Access point password: Choose a password to secure your device's access point. Detault is 'thebatatm'
- LNURLDevice string: Set the connection string for funding. This can be from lnbits.com or self-hosted.
- Live rate API URL: Set an API endpoint for retrieving live rates. e.g., https://legend.lnbits.com/lnurlp/api/v1/rate/GBP.
- Wallet endpoint URL: Set the endpoint URL for wallet actions. e.g., https://legend.lnbits.com/api/v1/wallet.
- API Key: Input your API Key, such as 69f5a4afe7834ddsj3jd7dj4f44cd82.


### Acceptor and Fiat Settings:

- Enable or disable accepting notes or coins.
- Set coin and bill values for the acceptors.
- Set a maximum deposit value for each transaction.
- Set a fee rate.
- You can set a redemption period for each transaction when the thermal printer is in use.

### Custom Options:

- Turn off/on the LED surround for the coin acceptor.
- Enable/disable displaying the live rate. (Wifi connection required)
- Set a balance check after each transaction. (Wifi connection required)

### Withdrawl Settings:
- Set the output display options of the transaction QR codes – it could be the screen, print, or both.
- You can decide to display Bitcoin quotes or jokes on each receipt – it would be selected randomly each time.
- You can set a URL of your website or LNBits install URL (And other information you like) that prints on the receipt for promotional purposes.
- The maximum number of receipts that can be printed on the thermal printer can be set.
- Set in ATM or Gift voucher modes. ATM acts like an ATM, but Gift voucher acts like a voucher dispenser, printing to receipt only and adding different information to the receipt.
- Enable/Disable NFC withdrawls. (Wifi connection required)
- Withdrawl period timeout for transaction (with backup receipt printing if needed)


### Network Time Protovol (NTP) Settings:

- Configure your timezone, NTP server, and sync intervals for accurate timestamps on printed receipts. (Wifi connection required)
- A time offset can be set for regions observing daylight saving.

### Display Settings:

- Change screen brightness
- Set the color mode of your display: default grafitti, stealth black and white, or set your custom color options.

### Translation Settings:

- Set the default and second language (with string translations) for your device. A button press when the ATM is in use will change the language.

### Updating Firmware:
- The B.A.T device allows firmware updates via OTA using .BIN files. To apply a new firmware update, navigate to the Update section in the access point, and proceed to flash the new release of the firmware.

------------

![](https://i.ibb.co/bLwT21k/BULL.png)

