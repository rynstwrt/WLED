#pragma once

#include "wled.h"

class RyanUsermod : public Usermod 
{
    private:
        unsigned long lastTime;
        unsigned long currentTime;
        unsigned long loopTime;

        enum State {effect, palette, brightness, speed, intensity};
        State selectedState = effect;

        unsigned int numEffects = 118;
        unsigned int effectIndexStart = 1;
        unsigned int effectIndex = effectIndexStart;
        int8_t bannedEffects[9] = { 0, 50, 62, 82, 83, 84, 96, 98, 116 };

        unsigned int numPalettes = 71;
        unsigned int paletteIndexStart = 6;
        unsigned int paletteIndex = paletteIndexStart;
        int8_t bannedPalettes[6] = { 0, 1, 2, 3, 4, 5 };
        
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
                        if (selectedState == effect)
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
                }
            

                int encoderA = digitalRead(dtPin);
                int encoderB = digitalRead(clkPin);

                if ((!encoderA) && encoderAPrev)
                {
                    // ****FLIPPED FOR MY FIRST D1 MINI**** 
                    // Change to encoderB == HIGH and else encoderB == LOW for next d1 minis
                    if (encoderB == LOW) 
                    {
                        if (selectedState == effect)
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
                    }


                    
                }
                

                encoderAPrev = encoderA;
                loopTime = currentTime;
            }
        }
};