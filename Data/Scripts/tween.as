//-----------------------------------------------------------------------------
//           Name: tween.as
//      Developer: Wolfire Games LLC
//    Script Type:
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------

const float PI = 3.14159265359f;

enum TweenType {
					linear,
					inQuad,
					outQuad,
					inOutQuad,
					outInQuad,
					inCubic,
					outCubic,
					inOutCubic,
					outInCubic,
					inQuart,
					outQuart,
					inOutQuart,
					outInQuart,
					inQuint,
					outQuint,
					inOutQuint,
					outInQuint,
					inSine,
					outSine,
					inOutSine,
					outInSine,
					inExpo,
					outExpo,
					inOutExpo,
					outInExpo,
					inCirc,
					outCirc,
					inOutCirc,
					outInCirc,
					inBounce,
					outBounce,
					inOutBounce,
					outInBounce
				};

int GetTweenType(string tween_name){
    // This function should only be called once to convert the name into an enum value.
    if(tween_name == "linear")          {return linear;}
    if(tween_name == "inQuad")          {return inQuad;}
    if(tween_name == "outQuad")         {return outQuad;}
    if(tween_name == "inOutQuad")       {return inOutQuad;}
    if(tween_name == "outInQuad")       {return outInQuad;}
    if(tween_name == "inCubic")         {return inCubic;}
    if(tween_name == "outCubic")        {return outCubic;}
    if(tween_name == "inOutCubic")      {return inOutCubic;}
    if(tween_name == "outInCubic")      {return outInCubic;}
    if(tween_name == "inQuart")         {return inQuart;}
    if(tween_name == "outQuart")        {return outQuart;}
    if(tween_name == "inOutQuart")      {return inOutQuart;}
    if(tween_name == "outInQuart")      {return outInQuart;}
    if(tween_name == "inQuint")         {return inQuint;}
    if(tween_name == "outQuint")        {return outQuint;}
    if(tween_name == "inOutQuint")      {return inOutQuint;}
    if(tween_name == "outInQuint")      {return outInQuint;}
    if(tween_name == "inSine")          {return inSine;}
    if(tween_name == "outSine")         {return outSine;}
    if(tween_name == "inOutSine")       {return inOutSine;}
    if(tween_name == "outInSine")       {return outInSine;}
    if(tween_name == "inExpo")          {return inExpo;}
    if(tween_name == "outExpo")         {return outExpo;}
    if(tween_name == "inOutExpo")       {return inOutExpo;}
    if(tween_name == "outInExpo")       {return outInExpo;}
    if(tween_name == "inCirc")          {return inCirc;}
    if(tween_name == "outCirc")         {return outCirc;}
    if(tween_name == "inOutCirc")       {return inOutCirc;}
    if(tween_name == "outInCirc")       {return outInCirc;}
    if(tween_name == "inBounce")        {return inBounce;}
    if(tween_name == "outBounce")       {return outBounce;}
    if(tween_name == "inOutBounce")     {return inOutBounce;}
    if(tween_name == "outInBounce")     {return outInBounce;}

    return linear;
}

float InQuad(float progress){
	return progress * progress;
}

float OutQuad(float progress){
	return 1 - (1 - progress) * (1 - progress);
}

float InOutQuad(float progress){
	return progress < 0.5 ? 2 * progress * progress : 1 - pow(-2 * progress + 2, 2) / 2;
}

float OutInQuad(float progress){
	return mix(OutQuad(progress), InQuad(progress), progress);
}

float InCubic(float progress){
	return progress * progress * progress;
}

float OutCubic(float progress){
	return 1 - pow(1 - progress, 3);
}

float InOutCubic(float progress){
	return progress < 0.5 ? 4 * progress * progress * progress : 1 - pow(-2 * progress + 2, 3) / 2;
}

float OutInCubic(float progress){
	return mix(OutCubic(progress), InCubic(progress), progress);
}

float InQuart(float progress){
	return progress * progress * progress * progress;
}

float OutQuart(float progress){
	return 1 - pow(1 - progress, 4);
}

float InOutQuart(float progress){
	return progress < 0.5 ? 8 * progress * progress * progress * progress : 1 - pow(-2 * progress + 2, 4) / 2;
}

float OutInQuart(float progress){
	return mix(OutQuart(progress), InQuart(progress), progress);
}

float InQuint(float progress){
	return progress * progress * progress * progress * progress;
}

float OutQuint(float progress){
	return 1 - pow(1 - progress, 5);
}

float InOutQuint(float progress){
	return progress < 0.5 ? 16 * progress * progress * progress * progress * progress : 1 - pow(-2 * progress + 2, 5) / 2;
}

float OutInQuint(float progress){
	return mix(OutQuint(progress), InQuint(progress), progress);
}

float InSine(float progress){
	return 1 - cos((progress * PI) / 2);
}

float OutSine(float progress){
	return sin((progress * PI) / 2);
}

float InOutSine(float progress){
	return -(cos(PI * progress) - 1) / 2;
}

float OutInSine(float progress){
	return mix(OutSine(progress), InSine(progress), progress);
}

float InExpo(float progress){
	return progress == 0 ? 0.0 : pow(2, 10 * progress - 10);
}

float OutExpo(float progress){
	return progress == 1 ? 1.0 : 1 - pow(2, -10 * progress);
}

float InOutExpo(float progress){
	return progress == 0
	  ? 0.0
	  : progress == 1
	  ? 1.0
	  : progress < 0.5 ? pow(2, 20 * progress - 10) / 2
	  : (2 - pow(2, -20 * progress + 10)) / 2;
}

float OutInExpo(float progress){
	return mix(OutExpo(progress), InExpo(progress), progress);
}

float InCirc(float progress){
	return 1 - sqrt(1 - pow(progress, 2));
}

float OutCirc(float progress){
	return sqrt(1 - pow(progress - 1, 2));
}

float InOutCirc(float progress){
	return progress < 0.5
	  ? (1 - sqrt(1 - pow(2 * progress, 2))) / 2
	  : (sqrt(1 - pow(-2 * progress + 2, 2)) + 1) / 2;
}

float OutInCirc(float progress){
	return mix(OutCirc(progress), InCirc(progress), progress);
}

float InBounce(float progress){
	return 1 - OutBounce(1 - progress);
}

float OutBounce(float progress){
	const float n1 = 7.5625;
	const float d1 = 2.75;

	if (progress < 1 / d1) {
	    return n1 * progress * progress;
	} else if (progress < 2 / d1) {
	    return n1 * (progress -= 1.5 / d1) * progress + 0.75;
	} else if (progress < 2.5 / d1) {
	    return n1 * (progress -= 2.25 / d1) * progress + 0.9375;
	} else {
	    return n1 * (progress -= 2.625 / d1) * progress + 0.984375;
	}
}

float InOutBounce(float progress){
	return progress < 0.5
	  ? (1 - OutBounce(1 - 2 * progress)) / 2
	  : (1 + OutBounce(2 * progress - 1)) / 2;
}

float OutInBounce(float progress){
	return mix(OutBounce(progress), InBounce(progress), progress);
}

float ApplyTween(float progress, int tween_type){
    // Using a switch case is more efficient than an if/else structure, especially when calling it once every update.
	switch(tween_type){
		case linear:
			return progress;
		case inQuad:
			return InQuad(progress);
		case outQuad:
			return OutQuad(progress);
		case inOutQuad:
			return InOutQuad(progress);
		case outInQuad:
			return OutInQuad(progress);

		case inCubic:
			return InCubic(progress);
		case outCubic:
			return OutCubic(progress);
		case inOutCubic:
			return InOutCubic(progress);
		case outInCubic:
			return OutInCubic(progress);

		case inQuart:
			return InQuart(progress);
		case outQuart:
			return OutQuart(progress);
		case inOutQuart:
			return InOutQuart(progress);
		case outInQuart:
			return OutInQuart(progress);

		case inQuint:
			return InQuint(progress);
		case outQuint:
			return OutQuint(progress);
		case inOutQuint:
			return InOutQuint(progress);
		case outInQuint:
			return OutInQuint(progress);

		case inSine:
			return InSine(progress);
		case outSine:
			return OutSine(progress);
		case inOutSine:
			return InOutSine(progress);
		case outInSine:
			return OutInSine(progress);

		case inExpo:
			return InExpo(progress);
		case outExpo:
			return OutExpo(progress);
		case inOutExpo:
			return InOutExpo(progress);
		case outInExpo:
			return OutInExpo(progress);

		case inCirc:
			return InCirc(progress);
		case outCirc:
			return OutCirc(progress);
		case inOutCirc:
			return InOutCirc(progress);
		case outInCirc:
			return OutInCirc(progress);

		case inBounce:
			return InBounce(progress);
		case outBounce:
			return OutBounce(progress);
		case inOutBounce:
			return InOutBounce(progress);
		case outInBounce:
			return OutInBounce(progress);
	}
	
	return progress;
}