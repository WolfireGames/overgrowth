/*******
 *  
 * ui_tween.as
 *
 * A collection of tweening functions 
 *
 */

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


// For all functions: 
// time     t     running time. How much time has passed *right now*
// begin    b     starting property value
// change   c     ending - beginning
// duration d     how much time has to pass for the tweening to complete

const float pi = 3.14159265359;

/**
 * Linear 
 **/

float linear( float t, float b, float c, float d ) { 
    return c * t / d + b;
} 

/**
 * Quad
 **/

float inQuad( float t, float b, float c, float d ) { 
    return c * pow(t / d, 2) + b; 
}

float outQuad( float t, float b, float c, float d ) {
    t = t / d;
    return -c * t * (t - 2) + b;
}

float inOutQuad( float t, float b, float c, float d ) {
    t = t / d * 2;
    if ( t < 1 ) { 
        return c / 2 * pow(t, 2) + b;
    }
    return -c / 2 * ((t - 1) * (t - 3) - 1) + b;
}

float outInQuad( float t, float b, float c, float d ) {
    if ( t < d / 2 ) { 
        return outQuad(t * 2, b, c / 2, d);
    }
  return inQuad((t * 2) - d, b + c / 2, c / 2, d);
}

/**
 * Cubic
 **/

float inCubic ( float t, float b, float c, float d ) { 
    return c * pow(t / d, 3) + b;
}

float outCubic( float t, float b, float c, float d ) { 
    return c * (pow(t / d - 1, 3) + 1) + b;
}

float inOutCubic( float t, float b, float c, float d ) {
    t = t / d * 2;
    if ( t < 1 ) { 
        return c / 2 * t * t * t + b;
    }
    t = t - 2;
    return c / 2 * (t * t * t + 2) + b;
}

float outInCubic( float t, float b, float c, float d ) {
    if( t < d / 2 ) { 
        return outCubic(t * 2, b, c / 2, d);
    }
    return inCubic((t * 2) - d, b + c / 2, c / 2, d);
}

/**
 * Quart
 **/

float inQuart( float t, float b, float c, float d ) { 
    return c * pow(t / d, 4) + b;
}

float outQuart( float t, float b, float c, float d ) { 
    return -c * (pow(t / d - 1, 4) - 1) + b;
}

float inOutQuart( float t, float b, float c, float d ) {
    t = t / d * 2;
    if (t < 1 ) { 
        return c / 2 * pow(t, 4) + b; 
    }
    return -c / 2 * (pow(t - 2, 4) - 2) + b;
}

float outInQuart( float t, float b, float c, float d ) {
    if (t < d / 2 ) { 
        return outQuart(t * 2, b, c / 2, d); 
    }
    return inQuart((t * 2) - d, b + c / 2, c / 2, d);
}

/**
 * Quint
 **/

float inQuint( float t, float b, float c, float d ) { 
    return c * pow(t / d, 5) + b; 
}

float outQuint( float t, float b, float c, float d ) { 
    return c * (pow(t / d - 1, 5) + 1) + b;
}

float inOutQuint( float t, float b, float c, float d ) {
    t = t / d * 2;
    
    if (t < 1) { 
        return c / 2 * pow(t, 5) + b;
    }

    return c / 2 * (pow(t - 2, 5) + 2) + b;
}

float outInQuint( float t, float b, float c, float d ) {
    if ( t < d / 2 ) { 
        return outQuint(t * 2, b, c / 2, d);
    }
    return inQuint((t * 2) - d, b + c / 2, c / 2, d);
}

/**
 * Sine
 **/

float inSine( float t, float b, float c, float d ) { 
    return -c * cos(t / d * (pi / 2)) + c + b; 
}

float outSine( float t, float b, float c, float d ) { 
    return c * sin(t / d * (pi / 2)) + b;
}

float inOutSine( float t, float b, float c, float d ) { 
    return -c / 2 * (cos(pi * t / d) - 1) + b;
}

float outInSine( float t, float b, float c, float d ) {
    if( t < d / 2 ) { 
        return outSine(t * 2, b, c / 2, d);
    }
    return inSine((t * 2) -d, b + c / 2, c / 2, d);
}

/**
 * Expo
 **/
float inExpo( float t, float b, float c, float d ) {
    if( t == 0 ){ 
        return b;
    }
    return c * pow(2, 10 * (t / d - 1)) + b - c * 0.001;
}

float outExpo( float t, float b, float c, float d ) {
    if( t == d ){ 
        return b + c;
    }

    return c * 1.001 * (-pow(2, -10 * t / d) + 1) + b;
}

float inOutExpo( float t, float b, float c, float d ) {
    if( t == 0 ){ 
        return b;
    }

    if( t == d ){ 
        return b + c;
    }

    t = t / d * 2;
    
    if( t < 1 ){ 
        return c / 2 * pow(2, 10 * (t - 1)) + b - c * 0.0005; 
    }

    return c / 2 * 1.0005 * (-pow(2, -10 * (t - 1)) + 2) + b;
}

float outInExpo( float t, float b, float c, float d ) {
    if( t < d / 2 ){ 
        return outExpo(t * 2, b, c / 2, d);
    }

    return inExpo((t * 2) - d, b + c / 2, c / 2, d);
}

/**
 * Circ
 **/
float inCirc( float t, float b, float c, float d ) { 
    return(-c * (sqrt(1 - pow(t / d, 2)) - 1) + b);
}

float outCirc( float t, float b, float c, float d ) {  
    return(c * sqrt(1 - pow(t / d - 1, 2)) + b);
}

float inOutCirc( float t, float b, float c, float d ) {
    t = t / d * 2;
    if( t < 1 ){ 
        return -c / 2 * (sqrt(1 - t * t) - 1) + b;
    }
    t = t - 2;
    return c / 2 * (sqrt(1 - t * t) + 1) + b;
}

float outInCirc( float t, float b, float c, float d ) {
    if( t < d / 2 ){ 
        return outCirc(t * 2, b, c / 2, d);
    }
    return inCirc((t * 2) - d, b + c / 2, c / 2, d);
}


/**
 * Bounce
 **/
float outBounce( float t, float b, float c, float d ) {
    t = t / d;
    if( t < 1 / 2.75 ){ 
        return c * (7.5625 * t * t) + b;
    }

    if( t < 2 / 2.75 ) {
        t = t - (1.5 / 2.75);
        return c * (7.5625 * t * t + 0.75) + b;
    }
    else if( t < 2.5 / 2.75 ){
        t = t - (2.25 / 2.75);
        return c * (7.5625 * t * t + 0.9375) + b;
    }
    t = t - (2.625 / 2.75);
    return c * (7.5625 * t * t + 0.984375) + b;

}

float inBounce( float t, float b, float c, float d ) { 
    return c - outBounce(d - t, 0, c, d) + b;
}
float inOutBounce( float t, float b, float c, float d ) {
    if( t < d / 2 ) {
        return inBounce(t * 2, 0, c, d) * 0.5 + b;
    }
    return outBounce(t * 2 - d, 0, c, d) * 0.5 + c * .5 + b;
}

float outInBounce( float t, float b, float c, float d ) {
    if( t < d / 2 ) { 
        return outBounce(t * 2, b, c / 2, d); 
    }
    return inBounce((t * 2) - d, b + c / 2, c / 2, d);
}


/*******
 *  
 * Function definitions
 *
 */

funcdef float TweenFunc( float, float, float, float );



