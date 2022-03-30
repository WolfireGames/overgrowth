//-----------------------------------------------------------------------------
//           Name: im_tween.cpp
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
#include "im_tween.h"

#include <cmath>

// MJB Note: this is a really ugly kludge, but it's the least
//  work to fix anoter weird angelscript error
LinearTween linearTween_inst;
InQuadTween inQuadTween_inst;
OutQuadTween outQuadTween_inst;
InOutQuadTween inOutQuadTween_inst;
OutInQuadTween outInQuadTween_inst;
InCubicTween inCubicTween_inst;
OutCubicTween outCubicTween_inst;
InOutCubicTween inOutCubicTween_inst;
OutInCubicTween outInCubicTween_inst;
InQuartTween inQuartTween_inst;
OutQuartTween outQuartTween_inst;
InOutQuartTween inOutQuartTween_inst;
OutInQuartTween outInQuartTween_inst;
InQuintTween inQuintTween_inst;
OutQuintTween outQuintTween_inst;
InOutQuintTween inOutQuintTween_inst;
OutInQuintTween outInQuintTween_inst;
InSineTween inSineTween_inst;
OutSineTween outSineTween_inst;
InOutSineTween inOutSineTween_inst;
OutInSineTween outInSineTween_inst;
InExpoTween inExpoTween_inst;
OutExpoTween outExpoTween_inst;
InOutExpoTween inOutExpoTween_inst;
OutInExpoTween outInExpoTween_inst;
InCircTween inCircTween_inst;
OutCircTween outCircTween_inst;
InOutCircTween inOutCircTween_inst;
OutInCircTween outInCircTween_inst;
OutBounceTween outBounceTween_inst;
InBounceTween inBounceTween_inst;
InOutBounceTween inOutBounceTween_inst;
OutInBounceTween outInBounceTween_inst;

IMTween* getTweenInstance( IMTweenType tweenType ) {
    switch(tweenType) {
		case linearTween: { 
			return (IMTween*) &linearTween_inst;
		}
		break;

		case inQuadTween: { 
			return (IMTween*) &inQuadTween_inst;
		}
		break;

		case outQuadTween: { 
			return (IMTween*) &outQuadTween_inst;
		}
		break;

		case inOutQuadTween: { 
			return (IMTween*) &inOutQuadTween_inst;
		}
		break;

		case outInQuadTween: { 
			return (IMTween*) &outInQuadTween_inst;
		}
		break;

		case inCubicTween: { 
			return (IMTween*) &inCubicTween_inst;
		}
		break;

		case outCubicTween: { 
			return (IMTween*) &outCubicTween_inst;
		}
		break;

		case inOutCubicTween: { 
			return (IMTween*) &inOutCubicTween_inst;
		}
		break;

		case outInCubicTween: { 
			return (IMTween*) &outInCubicTween_inst;
		}
		break;

		case inQuartTween: { 
			return (IMTween*) &inQuartTween_inst;
		}
		break;

		case outQuartTween: { 
			return (IMTween*) &outQuartTween_inst;
		}
		break;

		case inOutQuartTween: { 
			return (IMTween*) &inOutQuartTween_inst;
		}
		break;

		case outInQuartTween: { 
			return (IMTween*) &outInQuartTween_inst;
		}
		break;

		case inQuintTween: { 
			return (IMTween*) &inQuintTween_inst;
		}
		break;

		case outQuintTween: { 
			return (IMTween*) &outQuintTween_inst;
		}
		break;

		case inOutQuintTween: { 
			return (IMTween*) &inOutQuintTween_inst;
		}
		break;

		case outInQuintTween: { 
			return (IMTween*) &outInQuintTween_inst;
		}
		break;

		case inSineTween: { 
			return (IMTween*) &inSineTween_inst;
		}
		break;

		case outSineTween: { 
			return (IMTween*) &outSineTween_inst;
		}
		break;

		case inOutSineTween: { 
			return (IMTween*) &inOutSineTween_inst;
		}
		break;

		case outInSineTween: { 
			return (IMTween*) &outInSineTween_inst;
		}
		break;

		case inExpoTween: { 
			return (IMTween*) &inExpoTween_inst;
		}
		break;

		case outExpoTween: { 
			return (IMTween*) &outExpoTween_inst;
		}
		break;

		case inOutExpoTween: { 
			return (IMTween*) &inOutExpoTween_inst;
		}
		break;

		case outInExpoTween: { 
			return (IMTween*) &outInExpoTween_inst;
		}
		break;

		case inCircTween: { 
			return (IMTween*) &inCircTween_inst;
		}
		break;

		case outCircTween: { 
			return (IMTween*) &outCircTween_inst;
		}
		break;

		case inOutCircTween: { 
			return (IMTween*) &inOutCircTween_inst;
		}
		break;

		case outInCircTween: { 
			return (IMTween*) &outInCircTween_inst;
		}
		break;

		case outBounceTween: { 
			return (IMTween*) &outBounceTween_inst;
		}
		break;

		case inBounceTween: { 
			return (IMTween*) &inBounceTween_inst;
		}
		break;

		case inOutBounceTween: { 
			return (IMTween*) &inOutBounceTween_inst;
		}
		break;

		case outInBounceTween: { 
			return (IMTween*) &outInBounceTween_inst;
		}
		break;

	}
    return NULL;
}
