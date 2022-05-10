//-----------------------------------------------------------------------------
//           Name: im_tween.h
//      Developer: Wolfire Games LLC
//    Description: A collection of tweening functions
//        License: Read below
//-----------------------------------------------------------------------------
/*******
 *
 * Based on tween.lua by Enrique García Cota (https://github.com/kikito/tween.lua)
 *
 * Original license (for this file):
 *
 *  MIT LICENSE
 *
 *  Copyright (c) 2014 Enrique García Cota, Yuichi Tateno, Emmanuel Oga
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included
 *  in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#pragma once

#include <Math/enginemath.h>
#include <cmath>

using std::pow;

// For all functions:
// time     t     running time. How much time has passed *right now*
// begin    b     starting property value
// change   c     ending - beginning
// duration d     how much time has to pass for the tweening to complete

/*******************************************************************************************/
/**
 * @brief  Base struct for all tweens
 *
 */

struct IMTween {
    int refCount;

    virtual float compute(float t, float b, float c, float d) = 0;

    // dummy functions as we're not ref counting these
    void AddRef() {
    }
    void Release() {
    }
};

/**
 * Linear
 **/

struct LinearTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return c * t / d + b;
    }
};

/**
 * Quad
 **/

struct InQuadTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return c * pow(t / d, 2) + b;
    }
};

struct OutQuadTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        t = t / d;
        return -c * t * (t - 2) + b;
    }
};

struct InOutQuadTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        t = t / d * 2;
        if (t < 1) {
            return c / 2 * pow(t, 2) + b;
        }
        return -c / 2 * ((t - 1) * (t - 3) - 1) + b;
    }
};

struct OutInQuadTween : public IMTween {
    InQuadTween inQuad;
    OutQuadTween outQuad;

    float compute(float t, float b, float c, float d) override {
        if (t < d / 2) {
            return outQuad.compute(t * 2, b, c / 2, d);
        }
        return inQuad.compute((t * 2) - d, b + c / 2, c / 2, d);
    }
};

/**
 * Cubic
 **/

struct InCubicTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return c * pow(t / d, 3) + b;
    }
};

struct OutCubicTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return c * (pow(t / d - 1, 3) + 1) + b;
    }
};

struct InOutCubicTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        t = t / d * 2;
        if (t < 1) {
            return c / 2 * t * t * t + b;
        }
        t = t - 2;
        return c / 2 * (t * t * t + 2) + b;
    }
};

struct OutInCubicTween : public IMTween {
    OutCubicTween outCubic;
    InCubicTween inCubic;

    float compute(float t, float b, float c, float d) override {
        if (t < d / 2) {
            return outCubic.compute(t * 2, b, c / 2, d);
        }
        return inCubic.compute((t * 2) - d, b + c / 2, c / 2, d);
    }
};

/**
 * Quart
 **/

struct InQuartTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return c * pow(t / d, 4) + b;
    }
};

struct OutQuartTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return -c * (pow(t / d - 1, 4) - 1) + b;
    }
};

struct InOutQuartTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        t = t / d * 2;
        if (t < 1) {
            return c / 2 * pow(t, 4) + b;
        }
        return -c / 2 * (pow(t - 2, 4) - 2) + b;
    }
};

struct OutInQuartTween : public IMTween {
    OutQuartTween outQuart;
    InQuartTween inQuart;

    float compute(float t, float b, float c, float d) override {
        if (t < d / 2) {
            return outQuart.compute(t * 2, b, c / 2, d);
        }
        return inQuart.compute((t * 2) - d, b + c / 2, c / 2, d);
    }
};

/**
 * Quint
 **/

struct InQuintTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return c * pow(t / d, 5) + b;
    }
};

struct OutQuintTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return c * (pow(t / d - 1, 5) + 1) + b;
    }
};

struct InOutQuintTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        t = t / d * 2;

        if (t < 1) {
            return c / 2 * pow(t, 5) + b;
        }

        return c / 2 * (pow(t - 2, 5) + 2) + b;
    }
};

struct OutInQuintTween : public IMTween {
    InQuintTween inQuint;
    OutQuintTween outQuint;

    float compute(float t, float b, float c, float d) override {
        if (t < d / 2) {
            return outQuint.compute(t * 2, b, c / 2, d);
        }
        return inQuint.compute((t * 2) - d, b + c / 2, c / 2, d);
    }
};

/**
 * Sine
 **/

struct InSineTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return -c * cos(t / d * (PI_f / 2)) + c + b;
    }
};

struct OutSineTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return c * sin(t / d * (PI_f / 2)) + b;
    }
};

struct InOutSineTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return -c / 2 * (cos(PI_f * t / d) - 1) + b;
    }
};

struct OutInSineTween : public IMTween {
    InSineTween inSine;
    OutSineTween outSine;

    float compute(float t, float b, float c, float d) override {
        if (t < d / 2) {
            return outSine.compute(t * 2, b, c / 2, d);
        }
        return inSine.compute((t * 2) - d, b + c / 2, c / 2, d);
    }
};

/**
 * Expo
 **/

struct InExpoTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        if (t == 0) {
            return b;
        }
        return (float)(c * pow(2, 10 * (t / d - 1)) + b - c * 0.001);
    }
};

struct OutExpoTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        if (t == d) {
            return b + c;
        }

        return (float)(c * 1.001 * (-pow(2, -10 * t / d) + 1) + b);
    }
};

struct InOutExpoTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        if (t == 0) {
            return b;
        }

        if (t == d) {
            return b + c;
        }

        t = t / d * 2;

        if (t < 1) {
            return (float)(c / 2 * pow(2, 10 * (t - 1)) + b - c * 0.0005);
        }

        return (float)(c / 2 * 1.0005 * (-pow(2, -10 * (t - 1)) + 2) + b);
    }
};

struct OutInExpoTween : public IMTween {
    InExpoTween inExpo;
    OutExpoTween outExpo;

    float compute(float t, float b, float c, float d) override {
        if (t < d / 2) {
            return outExpo.compute(t * 2, b, c / 2, d);
        }

        return inExpo.compute((t * 2) - d, b + c / 2, c / 2, d);
    }
};

/**
 * Circ
 **/

struct InCircTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return (-c * (sqrt(1 - pow(t / d, 2)) - 1) + b);
    }
};

struct OutCircTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        return (c * sqrt(1 - pow(t / d - 1, 2)) + b);
    }
};

struct InOutCircTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        t = t / d * 2;
        if (t < 1) {
            return -c / 2 * (sqrt(1 - t * t) - 1) + b;
        }
        t = t - 2;
        return c / 2 * (sqrt(1 - t * t) + 1) + b;
    }
};

struct OutInCircTween : public IMTween {
    InCircTween inCirc;
    OutCircTween outCirc;

    float compute(float t, float b, float c, float d) override {
        if (t < d / 2) {
            return outCirc.compute(t * 2, b, c / 2, d);
        }
        return inCirc.compute((t * 2) - d, b + c / 2, c / 2, d);
    }
};

/**
 * Bounce
 **/

struct OutBounceTween : public IMTween {
    float compute(float t, float b, float c, float d) override {
        t = t / d;
        if (t < 1 / 2.75f) {
            return c * (7.5625f * t * t) + b;
        }

        if (t < 2 / 2.75f) {
            t = t - (1.5f / 2.75f);
            return c * (7.5625f * t * t + 0.75f) + b;
        } else if (t < 2.5f / 2.75f) {
            t = t - (2.25f / 2.75f);
            return c * (7.5625f * t * t + 0.9375f) + b;
        }
        t = t - (2.625f / 2.75f);
        return c * (7.5625f * t * t + 0.984375f) + b;
    }
};

struct InBounceTween : public IMTween {
    OutBounceTween outBounce;

    float compute(float t, float b, float c, float d) override {
        return c - outBounce.compute(d - t, 0, c, d) + b;
    }
};

struct InOutBounceTween : public IMTween {
    OutBounceTween outBounce;
    InBounceTween inBounce;

    float compute(float t, float b, float c, float d) override {
        if (t < d / 2) {
            return inBounce.compute(t * 2, 0, c, d) * 0.5f + b;
        }
        return outBounce.compute(t * 2 - d, 0, c, d) * 0.5f + c * 0.5f + b;
    }
};

struct OutInBounceTween : public IMTween {
    InBounceTween inBounce;
    OutBounceTween outBounce;

    float compute(float t, float b, float c, float d) override {
        if (t < d / 2) {
            return outBounce.compute(t * 2, b, c / 2, d);
        }
        return inBounce.compute((t * 2) - d, b + c / 2, c / 2, d);
    }
};

/*******
 *
 * Enum type for enumerating the tweens
 *
 */
enum IMTweenType {
    linearTween,
    inQuadTween,
    outQuadTween,
    inOutQuadTween,
    outInQuadTween,
    inCubicTween,
    outCubicTween,
    inOutCubicTween,
    outInCubicTween,
    inQuartTween,
    outQuartTween,
    inOutQuartTween,
    outInQuartTween,
    inQuintTween,
    outQuintTween,
    inOutQuintTween,
    outInQuintTween,
    inSineTween,
    outSineTween,
    inOutSineTween,
    outInSineTween,
    inExpoTween,
    outExpoTween,
    inOutExpoTween,
    outInExpoTween,
    inCircTween,
    outCircTween,
    inOutCircTween,
    outInCircTween,
    outBounceTween,
    inBounceTween,
    inOutBounceTween,
    outInBounceTween
};

// Turn an instance into a pointer to a tween object
IMTween* getTweenInstance(IMTweenType tweenType);
