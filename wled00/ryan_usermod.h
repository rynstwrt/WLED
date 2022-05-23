#pragma once

#include "wled.h"
#include <Arduino.h>
#include <U8x8lib.h>

#define OLED_PIN_SCL 5  // D1
#define OLED_PIN_SDA 4  // D2
#define OLED_REFRESH_RATE_MS 125

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE, OLED_PIN_SCL, OLED_PIN_SDA); // Pins are Reset, SCL, SDA


class RyanUsermod : public Usermod 
{
    private:
        unsigned long lastTime;
        unsigned long currentTime;
        unsigned long loopTime;

        enum State {effect, palette, brightness, speed, intensity};
        State selectedState = effect;
        const char* friendlyStateNames[5] = { "EFFECT", "PALETTE", "BRIGHTNESS", "SPEED", "FX SPECIFIC" };

        unsigned int numEffects = 118;
        unsigned int effectIndexStart = 1;
        unsigned int effectIndex = effectIndexStart;
        int8_t bannedEffects[9] = { 0, 50, 62, 82, 83, 84, 96, 98, 116 };
        char* effectFriendlyName;

        unsigned int numPalettes = 71;
        unsigned int paletteIndexStart = 6;
        unsigned int paletteIndex = paletteIndexStart;
        int8_t bannedPalettes[6] = { 0, 1, 2, 3, 4, 5 };
        char* paletteFriendlyName;
        
        unsigned char buttonState = HIGH;
        unsigned char prevButtonState = HIGH;

        CRGB fastled_col;
        CHSV prim_hsv;
        int16_t newVal;

        unsigned char encoderAPrev = 0;

        int clkPin = 14;  // d5
        int dtPin = 12;    // d6
        int swPin = 13;    // d7

        unsigned int scrollStep = 5;

        long lastOledUpdate = 0;
        bool oledTurnedOff = true;

    public:
        void setup()
        {
            pinMode(clkPin, INPUT_PULLUP);
            pinMode(dtPin, INPUT_PULLUP);
            pinMode(swPin, INPUT_PULLUP);

            currentTime = millis();
            loopTime = currentTime;

            col[0] = fastled_col.Black;
            col[1] = fastled_col.Black;
            col[2] = fastled_col.Black;
            effectCurrent = effectIndex;
            effectPalette = paletteIndex;

            colorUpdated(CALL_MODE_DIRECT_CHANGE);
            updateInterfaces(CALL_MODE_DIRECT_CHANGE);

            u8x8.begin();
            u8x8.setPowerSave(0);
            u8x8.setContrast(125);
            //u8x8.setFont(u8x8_font_5x8_f);
            u8x8.setFont(u8x8_font_chroma48medium8_r);
            u8x8.drawString(1, 1, "LED MATRIX");
            u8x8.drawString(1, 3, "PRESS TO");
            u8x8.drawString(1, 4, "CHANGE FUNCTION");
            u8x8.drawString(1, 6, "TURN TO");
            u8x8.drawString(1, 7, "CHANGE VALUE");
        }


        void setCurrentEffectName()
        {
            char lineBuffer[21];

            uint8_t printedChars = extractModeName(effectCurrent, JSON_mode_names, lineBuffer, 19);

            if (lineBuffer[0]=='*' && lineBuffer[1]==' ')
            {
                for (byte i=2; i<=printedChars; i++)
                    lineBuffer[i-2] = lineBuffer[i]; //include '\0'

                printedChars -= 2;
            }

            effectFriendlyName = lineBuffer;      
        }


        void setCurrentPaletteName()
        {
            char lineBuffer[21];

            uint8_t printedChars = extractModeName(effectPalette, JSON_palette_names, lineBuffer, 19);

            if (lineBuffer[0]=='*' && lineBuffer[1]==' ')
            {
                for (byte i=2; i<=printedChars; i++)
                    lineBuffer[i-2] = lineBuffer[i]; //include '\0'

                printedChars -= 2;
            }

            paletteFriendlyName = lineBuffer;      
        }


        void updateOled()
        {
            lastOledUpdate = millis();

            u8x8.clear();
            u8x8.setCursor(1, 0);
            u8x8.println(friendlyStateNames[selectedState]);

            if (selectedState == effect)
            {
                u8x8.setCursor(1, 1);
                u8x8.println("MODE:");

                u8x8.setCursor(1, 3);
                setCurrentEffectName();
                u8x8.print(effectFriendlyName);
            }
            else if (selectedState == palette)
            {
                u8x8.setCursor(1, 1);
                u8x8.println("NAME:");

                u8x8.setCursor(1, 3);
                setCurrentPaletteName();
                u8x8.print(paletteFriendlyName);
            }
            else if (selectedState == brightness)
            {
                u8x8.setCursor(1, 1);
                u8x8.println("PERCENT:");

                double val = bri;
                val /= 255;
                val *= 100;
                val = floor(val);

                u8x8.setCursor(1, 3);
                u8x8.print((int) val);
            }
            else if (selectedState == speed)
            {
                u8x8.setCursor(1, 1);
                u8x8.println("PERCENT:");

                double val = effectSpeed;
                val /= 255;
                val *= 100;
                val = floor(val);

                u8x8.setCursor(1, 3);
                u8x8.print((int) val);
            }
            else if (selectedState == intensity)
            {
                u8x8.setCursor(1, 1);
                u8x8.println("OPTION PERCENT:");

                double val = effectIntensity;
                val /= 255;
                val *= 100;
                val = floor(val);

                u8x8.setCursor(1, 3);
                u8x8.print((int) val);
            }
        }


        void loop()
        {
            currentTime = millis();


            if (currentTime >= (loopTime + 2))
            {
                buttonState = digitalRead(swPin);

                if (prevButtonState != buttonState)
                {
                    if (buttonState == LOW)
                    {
                        if (oledTurnedOff)
                        {
                            u8x8.setPowerSave(0);
                            oledTurnedOff = false;
                        }
                        else if (selectedState == effect)
                        {
                            selectedState = palette;
                        }
                        else if (selectedState == palette)
                        {
                            selectedState = brightness;
                        }
                        else if (selectedState == brightness)
                        {
                            selectedState = speed;
                        }
                        else if (selectedState == speed)
                        {
                            selectedState = intensity;
                        }
                        else if (selectedState == intensity)
                        {
                            selectedState = effect;
                        }

                        prevButtonState = buttonState;
                    }
                    else
                    {
                        prevButtonState = buttonState;
                    }

                    updateOled();
                }
            

                int encoderA = digitalRead(dtPin);
                int encoderB = digitalRead(clkPin);

                if ((!encoderA) && encoderAPrev)
                {
                    // ****FLIPPED FOR MY FIRST D1 MINI**** 
                    // Change to encoderB == HIGH and else encoderB == LOW for next d1 minis
                    if (encoderB == LOW) 
                    {
                        if (oledTurnedOff)
                        {
                            u8x8.setPowerSave(0);
                            oledTurnedOff = false;
                        }
                        else if (selectedState == effect)
                        {
                            Serial.print("effect change: ");

                            ++effectIndex;

                            int numBannedEffects = sizeof(bannedEffects) / sizeof(*bannedEffects);

                            while (std::find(bannedEffects, bannedEffects + numBannedEffects, effectIndex) != bannedEffects + numBannedEffects)
                            {
                                ++effectIndex;
                            }

                            if (effectIndex >= numEffects)
                            {
                                effectIndex = effectIndexStart;
                            }

                            effectCurrent = effectIndex;

                            Serial.println(effectCurrent);

                            strip.restartRuntime();
                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }
                        else if (selectedState == brightness)
                        {
                            Serial.print("brightness change: ");

                            int currentBrightness = bri;
                            currentBrightness += scrollStep;

                            if (currentBrightness > 255)
                                currentBrightness = 255;

                            bri = currentBrightness;

                            Serial.println(bri);

                            stateUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }
                        else if (selectedState == palette)
                        {
                            Serial.print("palette change: ");
                            ++paletteIndex;

                            int numBannedPalettes = sizeof(bannedPalettes) / sizeof(*bannedPalettes);

                            while (std::find(bannedPalettes, bannedPalettes + numBannedPalettes, paletteIndex) != bannedPalettes + numBannedPalettes)
                            {
                                ++paletteIndex;
                            }

                            if (paletteIndex >= numPalettes)
                            {
                                paletteIndex = paletteIndexStart;
                            }
                            
                            effectPalette = paletteIndex;

                            Serial.println(effectPalette);

                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                            
                        }
                        else if (selectedState == speed)
                        {
                            Serial.print("speed change: ");
                            
                            int currentSpeed = effectSpeed;
                            currentSpeed += scrollStep;

                            if (currentSpeed > 255)
                                currentSpeed = 255;

                            effectSpeed = currentSpeed;

                            Serial.println(effectSpeed);

                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }
                        else if (selectedState == intensity)
                        {
                            Serial.print("intensity change: ");

                            int currentIntensity = effectIntensity;
                            currentIntensity += scrollStep;

                            if (currentIntensity > 255)
                                currentIntensity = 255;

                            effectIntensity = currentIntensity;

                            Serial.println(effectIntensity);

                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }

                        updateOled();
                    }
                    else if (encoderB == HIGH)
                    {
                        if (selectedState == effect)
                        {
                            Serial.print("effect change: ");
                            
                            --effectIndex;

                            int numBannedEffects = sizeof(bannedEffects) / sizeof(*bannedEffects);

                            while (std::find(bannedEffects, bannedEffects + numBannedEffects, effectIndex) != bannedEffects + numBannedEffects)
                            {
                                --effectIndex;
                            }

                            if (effectIndex < 0)
                            {
                                effectIndex = numEffects - 1;
                            }

                            effectCurrent = effectIndex;

                            Serial.println(effectCurrent);

                            strip.restartRuntime();
                            colorUpdated(CALL_MODE_BUTTON);
                            updateInterfaces(CALL_MODE_BUTTON);
                        }
                        else if (selectedState == brightness)
                        {

                            Serial.print("brightness change: ");

                            int currentBrightness = bri;
                            currentBrightness -= scrollStep;

                            if (currentBrightness < 0)
                                currentBrightness = 0;

                            bri = currentBrightness;

                            Serial.println(bri);

                            stateUpdated(CALL_MODE_BUTTON);
                            updateInterfaces(CALL_MODE_BUTTON);
                        }
                        else if (selectedState == palette)
                        {
                            Serial.print("palette change: ");

                            --paletteIndex;

                            int numBannedPalettes = sizeof(bannedPalettes) / sizeof(*bannedPalettes);

                            while (std::find(bannedPalettes, bannedPalettes + numBannedPalettes, paletteIndex) != bannedPalettes + numBannedPalettes)
                            {
                                --paletteIndex;
                            }

                            if (paletteIndex < 0)
                            {
                                paletteIndex = numPalettes - 1;
                            }

                            effectPalette = paletteIndex;

                            Serial.println(effectPalette);

                            colorUpdated(CALL_MODE_BUTTON);
                            updateInterfaces(CALL_MODE_BUTTON);

                        }
                        else if (selectedState == speed)
                        {
                            Serial.print("speed change: ");
                            
                            int currentSpeed = effectSpeed;
                            currentSpeed -= scrollStep;

                            if (currentSpeed < 0)
                                currentSpeed = 0;

                            effectSpeed = currentSpeed;

                            Serial.println(effectSpeed);

                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }
                        else if (selectedState == intensity)
                        {
                            Serial.print("intensity change: ");

                            int currentIntensity = effectIntensity;
                            currentIntensity -= scrollStep;

                            if (currentIntensity < 0)
                                currentIntensity = 0;

                            effectIntensity = currentIntensity;

                            Serial.println(effectIntensity);

                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }

                        updateOled();
                    }


                    
                }
                

                if (!oledTurnedOff && millis() - lastOledUpdate > 5 * 60 * 1000)
                {
                    u8x8.setPowerSave(1);
                    oledTurnedOff = true;
                }

                encoderAPrev = encoderA;
                loopTime = currentTime;
            }
        }
};