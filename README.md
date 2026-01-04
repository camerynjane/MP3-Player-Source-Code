# MP3-Player-Source-Code
Source code for MP3 Project + Project Slide Link

## Step-by-Step Software Setup

1. Install Arduino IDE
    - Download & install the latest version of the Arduino IDE from the official website

2. Add Required Libraries
    Open Arduino IDE and install or add the following libraries via Library Manager or manual download:
    - DFRobotDFPlayerMini
    - U8g2 (for OLED display)
    - OneButton (for button handling)
    - EEPROM (usually pre-installed)

3. Connect Arduino Nano
    Connect the Arduino Nano board to your PC using a USB cable. Select the correct board type ("Arduino "Nano") and COM port from the "Tools" menu

4. Load the Source Code
    Copy the provided Arduino sketch source code and paste it into the Arduino IDE.

5. Verify and Upload
    Click the "Verify" button to compile the code. Fix any errors if they occur (Tip: The Serial Moniter tab will let you know if there's trouble compiling @ any certain areas).
    Then, click "Upload" to flash the code onto the Arduino Nano.

6. Prepare the SD Card
    Format the 8GB micro SD card w/ FAT32 file system. Copy MP3 audio files named sequentially (e.g. 001.mp3, 002.mp3) to the root directory of the SD card.

    Should look like this:
    SD Card/
        1/
            001.mp3
        2/
            002.mp3
        3/
            003.mp3

7. Insert the SD card
    Insert the formatted micro SD card into the DFPlayer Mini module's SD card slot. (Tip: Run the code from the Arduino IDE and ensure it reads the SD card files properly by looking at the Serial Monitor)

8. Power Up and Test
    Power your project via USB. The OLED will display the splash screen, and you ca use the buttons to navigate and play music.



        