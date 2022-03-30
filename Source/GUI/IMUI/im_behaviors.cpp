//-----------------------------------------------------------------------------
//           Name: im_behaviors.cpp
//      Developer: Wolfire Games LLC
//    Description: A collection of useful behaviors for the UI tools
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "im_behaviors.h"

/*************************************
*****************
*******
*
* update behaviors
*
*******/

/*******
*
* Tweens
*
*/

/**
* Fades the element in over a given time
**/

IMFadeIn::IMFadeIn( uint64_t time, IMTweenType _tweener ) :
    elapsed( 0 )
{
    tweenType = _tweener;
    tweener = getTweenInstance( tweenType );
    targetTime = time;
    tweener->compute( 0, 0.1f, 0.5, float( targetTime ) );
}


bool IMFadeIn::initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {

    originalAlpha = element->getAlpha();
    element->setEffectAlpha( 0.0f );
    return true;

}

bool IMFadeIn::update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {

    float newValue = tweener->compute( float(elapsed), 0.0f, originalAlpha, float( targetTime ) );

    element->setEffectAlpha(newValue );

    elapsed += delta;

    if( elapsed >= targetTime ) {
        // We're done so inform the element to remove this tween
        element->clearEffectAlpha();
        return false;
    }
    else {
        // keep going
        return true;
    }

}

void IMFadeIn::cleanUp( IMElement* element ) {
    element->clearEffectAlpha();
}

/**
* Uses rendering displacement to 'move in' the element from a given offset
**/
IMMoveIn::IMMoveIn( uint64_t time, vec2 _offset, IMTweenType _tweener ) :
    elapsed(0),
    targetTime(time),
    tweenType(_tweener),
    offset(_offset)
{
    tweener = getTweenInstance(tweenType);
}

bool IMMoveIn::initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    element->setDisplacement( offset );
    return true;
}

bool IMMoveIn::update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {

    elapsed += delta;

    float tweenValue = tweener->compute( (float) elapsed, 0.0f, 1.0f, (float) targetTime );

    vec2 newDisplacement = vec2( mix( offset.x(), 0.0f, tweenValue ),
                                 mix( offset.y(), 0.0f, tweenValue ) );

    element->setDisplacement( newDisplacement );

    if( elapsed >= targetTime ) {
        // We're done so inform the element to remove this tween
        element->setDisplacement( vec2(0, 0) );
        return false;
    }
    else {
        // keep going
        return true;
    }
}


/**
*  Transition text by fading in and fading out
**/
// NOTE: Target must be a text element
/**
* Fades the element in over a given time
**/

IMChangeTextFadeOutIn::IMChangeTextFadeOutIn( uint64_t time, std::string const& _targetText, IMTweenType _tweenerOut, IMTweenType _tweenerIn ) :
    elapsed(0)
{
    tweenerOutType = _tweenerOut;
    tweenerInType = _tweenerIn;
    tweenerOut = getTweenInstance( tweenerOutType );
    tweenerIn = getTweenInstance( tweenerInType );
    targetTime = time;
    targetText = _targetText;
    fadeAwayDone = false;
}

bool IMChangeTextFadeOutIn::initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {

    originalAlpha = element->getAlpha();
    return true;

}

bool IMChangeTextFadeOutIn::update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {

    if( !fadeAwayDone ) {
        float newValue = tweenerOut->compute( float(elapsed), originalAlpha, 0.0f, float( targetTime ) );

        element->setEffectAlpha(newValue );

        elapsed += delta;

        if( elapsed >= targetTime ) {
            // We're done so change the text
            IMText* textElement = static_cast<IMText*>( element );

            textElement->setText( targetText );
            textElement->setEffectAlpha( 0.0f );
            elapsed = 0;

            fadeAwayDone = true;

            return true;
        }
        else {
            // keep going
            return true;
        }
    }
    else {
        float newValue = tweenerIn->compute( float(elapsed), 0.0f, originalAlpha, float( targetTime ) );

        element->setEffectAlpha(newValue );

        elapsed += delta;

        if( elapsed >= targetTime ) {
            // We're done so inform the element to remove this tween
            element->clearEffectAlpha();
            return false;
        }
        else {
            // keep going
            return true;
        }
    }
}


/**
*  Transition image by fading in and fading out
**/
// NOTE: Target must be a image element


IMChangeImageFadeOutIn::IMChangeImageFadeOutIn( uint64_t time, std::string const& _targetImage, IMTweenType _tweenerOut, IMTweenType _tweenerIn ) :
elapsed(0)
{
    tweenerOutType = _tweenerOut;
    tweenerInType = _tweenerIn;
    tweenerOut = getTweenInstance( tweenerOutType );
    tweenerIn = getTweenInstance( tweenerInType );
    targetTime = time;
    targetImage = _targetImage;
    fadeAwayDone = false;
}

bool IMChangeImageFadeOutIn::initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {

    originalAlpha = element->getAlpha();
    return true;

}

bool IMChangeImageFadeOutIn::update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {

    if( !fadeAwayDone ) {
        float newValue = tweenerOut->compute( float(elapsed), originalAlpha, 0.0f, float( targetTime ) );

        element->setEffectAlpha(newValue );

        elapsed += delta;

        if( elapsed >= targetTime ) {
            // We're done so change the image
            IMImage* imageElement = static_cast<IMImage*>( element );

            // store the old x value so we can rescale
            float oldX = imageElement->getSizeX();

            imageElement->setImageFile( targetImage );
            imageElement->scaleToSizeX( oldX );
            imageElement->setEffectAlpha( 0.0f );
            elapsed = 0;

            fadeAwayDone = true;

            return true;
        }
        else {
            // keep going
            return true;
        }
    }
    else {
        float newValue = tweenerIn->compute( float(elapsed), 0.0f, originalAlpha, float( targetTime ) );

        element->setEffectAlpha(newValue );

        elapsed += delta;

        if( elapsed >= targetTime ) {
            // We're done so inform the element to remove this tween
            element->clearEffectAlpha();
            return false;
        }
        else {
            // keep going
            return true;
        }
    }

}

/*******
*
* Continuous update behaviors
*
*/

/**
* Continually pulse the alpha of this element
**/

IMPulseAlpha::IMPulseAlpha( float lower, float upper, float _speed ) {
    speed = _speed;
    midPoint = (lower + upper )/2;
    difference = (lower - upper )/2;
}

bool IMPulseAlpha::initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    return true;
}

float IMPulseAlpha::computeTransition( float base, float range ) {
    return base + (float) sin( float( elapsedTime ) / (1000.0 / speed ) * (2.0 * PI) ) * range;
}

bool IMPulseAlpha::update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    float currentAlpha = computeTransition( midPoint, difference );
    element->setEffectAlpha( currentAlpha );
    elapsedTime += delta;

    return true;

}

/**
* Continually pulse the alpha of this element's border
**/


IMPulseBorderAlpha::IMPulseBorderAlpha( float lower, float upper, float _speed ) {
    speed = _speed;
    midPoint = (lower + upper )/2;
    difference = (lower - upper )/2;
}

bool IMPulseBorderAlpha::initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    return true;
}

float IMPulseBorderAlpha::computeTransition( float base, float range ) {
    return base + (float) sin( float( elapsedTime ) / (1000.0 / speed ) * (2.0 * PI) ) * range;
}

bool IMPulseBorderAlpha::update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    float currentAlpha = computeTransition( midPoint, difference );
    element->setBorderAlpha( currentAlpha );
    elapsedTime += delta;

    return true;

}


/*************************************
*****************
*******
*
* mouse over behaviors
*
*******/

/**
* Scales when the mouse is over
**/

IMMouseOverScale::IMMouseOverScale( uint64_t time, float _offset, IMTweenType _tweener ) :
    elapsed(0),
    targetTime(time),
    tweenType(_tweener)
{
    tweener = getTweenInstance(tweenType);
	offset = _offset;
}

void IMMouseOverScale::onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
	IMImage* imageElement = static_cast<IMImage*>( element );
	// store the old x value so we can rescale
	oldX = imageElement->getSizeX();
	oldY = imageElement->getSizeY();
    elapsed = 0;
}

float IMMouseOverScale::computeTransition( float base, float range ) {
    return base + (float)sin( (float(elapsedTime) / (1000.0 * speed )) * (2.0 * PI) ) * range;
}

void IMMouseOverScale::onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    elapsed += delta;
    if(elapsed > targetTime) {
        elapsed = targetTime;
    }

    float tweenValue = tweener->compute( (float)elapsed, 0.0f, offset, (float)targetTime );
	
	IMImage* imageElement = static_cast<IMImage*>( element );

    /*if( elapsed >= targetTime ) {
        // We're done so inform the element to remove this tween
    }
    else {*/
        // keep going
		//imageElement->scaleToSizeX( oldX +  tweenValue );
		imageElement->setSizeX( oldX +  tweenValue );
		imageElement->setSizeY( oldY +  tweenValue );
		float offset = (tweenValue / 2.0f) * -1.0f;
		element->setDisplacement(vec2( offset, offset ));
    //}
}

bool IMMouseOverScale::onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
	IMImage* imageElement = static_cast<IMImage*>( element );
	//imageElement->scaleToSizeX( oldX );
	imageElement->setSizeX( oldX );
	imageElement->setSizeY( oldY );
	element->setDisplacement( vec2(0, 0) );
    return true;
}

/**
* Moves when the mouse is over
**/

IMMouseOverMove::IMMouseOverMove( uint64_t time, vec2 _offset, IMTweenType _tweener ) :
    elapsed(0),
    targetTime(time),
    tweenType(_tweener),
    offset(_offset)
{
    tweener = getTweenInstance(tweenType);
}

void IMMouseOverMove::onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    element->setDisplacement( vec2(0, 0) );
    elapsed = 0;
}

float IMMouseOverMove::computeTransition( float base, float range ) {
    return base + (float) sin( (float(elapsedTime) / (1000.0 * speed )) * (2.0 * PI) ) * range;
}

void IMMouseOverMove::onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    elapsed += delta;

    float tweenValue = tweener->compute( (float)elapsed, 0.0f, 1.0f, (float)targetTime );

    vec2 newDisplacement = vec2( mix( 0.0f, offset.x(), tweenValue ),
                                 mix( 0.0f, offset.y(), tweenValue ) );

    element->setDisplacement( newDisplacement );

    if( elapsed >= targetTime ) {
        // We're done so inform the element to remove this tween
        element->setDisplacement( offset );
    }
}

bool IMMouseOverMove::onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    element->setDisplacement( vec2(0, 0) );
    return true;
}

/**
* Shows the border when the mouse is over
**/


IMMouseOverShowBorder::IMMouseOverShowBorder() {
}

void IMMouseOverShowBorder::onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    originalBorderState = element->border;
    element->showBorder();
}

void IMMouseOverShowBorder::onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
}

bool IMMouseOverShowBorder::onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    element->showBorder( originalBorderState );
    return true;
}


/**
* Causes the element to pulse between two given colors when the mouse is hovering
**/
IMMouseOverPulseColor::IMMouseOverPulseColor( vec4 first, vec4 second, float _speed ) {

    speed = _speed;

    midPoints = vec4( (first.x() + second.x()) /2.0f,
                      (first.y() + second.y()) /2.0f,
                      (first.z() + second.z()) /2.0f,
                      (first.a() + second.a()) /2.0f );

    differences = vec4( (first.x() - second.x())/2.0f,
                        (first.y() - second.y())/2.0f,
                        (first.z() - second.z())/2.0f,
                        (first.a() - second.a())/2.0f );

}

void IMMouseOverPulseColor::onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    originalColor = element->getColor();
    elapsedTime = 0;
}

float IMMouseOverPulseColor::computeTransition( float base, float range ) {
    return base + (float) sin( (float(elapsedTime) / (1000.0 * speed )) * (2.0 * PI) ) * range;
}

void IMMouseOverPulseColor::onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    vec4 currentColor = vec4( computeTransition( midPoints.x(), differences.x() ),
                              computeTransition( midPoints.y(), differences.y() ),
                              computeTransition( midPoints.z(), differences.z() ),
                              computeTransition( midPoints.a(), differences.a() ) );

    element->setEffectColor( currentColor );
    elapsedTime += delta;
}

bool IMMouseOverPulseColor::onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    element->clearColorEffect();
    return true;
}


/**
* Causes the element's border to pulse between two given colors when the mouse is hovering
**/

IMMouseOverPulseBorder::IMMouseOverPulseBorder( vec4 first, vec4 second, float _speed ) {

    speed = _speed;

    midPoints = vec4( first.x() + second.x() /2,
                      first.y() + second.y() /2,
                      first.z() + second.z() /2,
                      first.a() + second.a() /2 );

    differences = vec4( (first.x() - second.x())/2,
                        (first.y() - second.y())/2,
                        (first.z() - second.z())/2,
                        (first.a() - second.a())/2 );

}

void IMMouseOverPulseBorder::onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    originalColor = element->getBorderColor();
    elapsedTime = 0;
}

float IMMouseOverPulseBorder::computeTransition( float base, float range ) {
    return base + (float) sin( float( elapsedTime ) / (1000.0 / speed ) * (2.0 * PI) ) * range;
}

void IMMouseOverPulseBorder::onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    vec4 currentColor = vec4( computeTransition( midPoints.x(), differences.x() ),
                              computeTransition( midPoints.y(), differences.y() ),
                              computeTransition( midPoints.z(), differences.z() ),
                              computeTransition( midPoints.a(), differences.a() ) );

    element->setBorderColor( currentColor );

    elapsedTime += delta;
}

bool IMMouseOverPulseBorder::onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    element->setBorderColor( originalColor );
    return true;
}

/**
* Just pulse the border alpha
**/


IMMouseOverPulseBorderAlpha::IMMouseOverPulseBorderAlpha( float lower, float upper, float _speed ) {

    speed = _speed;
    midPoint = (lower + upper )/2;
    difference = (lower - upper )/2;

}

void IMMouseOverPulseBorderAlpha::onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    originalAlpha = element->getBorderAlpha();
    elapsedTime = 0;
}

float IMMouseOverPulseBorderAlpha::computeTransition( float base, float range ) {
    return base + (float) sin( float( elapsedTime ) / (1000.0 / speed ) * (2.0 * PI) ) * range;
}

void IMMouseOverPulseBorderAlpha::onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    float currentAlpha = computeTransition( midPoint, difference );
    element->setBorderAlpha( currentAlpha );
    elapsedTime += delta;
}

bool IMMouseOverPulseBorderAlpha::onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    element->setBorderAlpha( originalAlpha );
    return true;
}

/**
* Fades the element in from 0.0 to 1.0f alpha
**/
IMMouseOverFadeIn::IMMouseOverFadeIn( uint64_t time, IMTweenType _tweener, float targetAlpha/*= 1.0f*/ ) :
    elapsed( 0 ),
    targetAlpha( targetAlpha )
{
    tweenType = _tweener;
    tweener = getTweenInstance( tweenType );
    targetTime = time;
    tweener->compute(0.0f, 0.0f, 1.0f, float( targetTime ) );
}

void IMMouseOverFadeIn::onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    elapsed = 0;
}

void IMMouseOverFadeIn::onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    if(elapsed > targetTime) {
        elapsed = targetTime;
    }
    float newValue = tweener->compute( float(elapsed), 0.0f, targetAlpha, float( targetTime ) );
    element->setEffectAlpha(newValue );
    elapsed += delta;
}

bool IMMouseOverFadeIn::onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    element->clearColorEffect();
    return true;
}

/**
* Sends a specific message on the different mouse over states
**/

IMFixedMessageOnMouseOver::IMFixedMessageOnMouseOver( IMMessage* _enterMessage,
                                                      IMMessage* _overMessage,
                                                      IMMessage* _leaveMessage ) {

    enterMessage = _enterMessage;
    overMessage = _overMessage;
    leaveMessage = _leaveMessage;

}

void IMFixedMessageOnMouseOver::onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    if( enterMessage != NULL ) {
        enterMessage->AddRef();
        element->sendMessage( enterMessage );
    }
}

void IMFixedMessageOnMouseOver::onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    if( overMessage != NULL ) {
        overMessage->AddRef();
        element->sendMessage( overMessage );
    }
}

bool IMFixedMessageOnMouseOver::onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    if( leaveMessage != NULL ) {
        leaveMessage->AddRef();
        element->sendMessage( leaveMessage );
    }
    return true;
}


/*************************************
*****************
*******
*
* mouse click behaviors
*
*******/

/**
* Sends a specific message on click (mouse *up*)
**/


/*******************************************************************************************/
/**
 * @brief  Various constructors
 *
 */
IMFixedMessageOnClick::IMFixedMessageOnClick( std::string const& messageName ) {
    theMessage = new IMMessage(messageName);
}

IMFixedMessageOnClick::IMFixedMessageOnClick( std::string const& messageName, int param ) {
    theMessage = new IMMessage(messageName, param);
}

IMFixedMessageOnClick::IMFixedMessageOnClick( std::string const& messageName, std::string param ) {
    theMessage = new IMMessage(messageName, param);
}

IMFixedMessageOnClick::IMFixedMessageOnClick( std::string const& messageName, float param ) {
    theMessage = new IMMessage(messageName, param);
}

IMFixedMessageOnClick::IMFixedMessageOnClick( IMMessage* message ) {
    theMessage = message;
}

bool IMFixedMessageOnClick::onUp( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    if( theMessage ) {
        theMessage->AddRef();
        element->sendMessage( theMessage );
    }
    return true;
}

IMMouseOverSound::IMMouseOverSound(std::string const& audioFile) {
    this->audioFile = audioFile;
}

IMMouseOverSound::~IMMouseOverSound() {
}

void IMMouseOverSound::onStart(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
    SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(audioFile);
    SoundGroupPlayInfo sgpi = SoundGroupPlayInfo(*sgr, vec3(0.0f));
    sgpi.flags = sgpi.flags | SoundFlags::kRelative;

    int audioHandle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->PlayGroup(audioHandle, sgpi);
}

void IMMouseOverSound::onContinue(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {

}

bool IMMouseOverSound::onFinish(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
    return true;
}

IMSoundOnClick::IMSoundOnClick(std::string const& audioFile) {
    this->audioFile = audioFile;
}

bool IMSoundOnClick::onUp(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
    SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(audioFile);
    SoundGroupPlayInfo sgpi = SoundGroupPlayInfo(*sgr, vec3(0.0f));
    sgpi.flags = sgpi.flags | SoundFlags::kRelative;

    int audioHandle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->PlayGroup(audioHandle, sgpi);
    return true;
}

IMFuncOnClick::IMFuncOnClick(asIScriptFunction * func) {
    func->AddRef();
    this->func = func;
}

bool IMFuncOnClick::onUp(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
    asIScriptContext* ctx = func->GetEngine()->CreateContext();
    ctx->Prepare(func);
    int r = ctx->Execute();
    if(r != asEXECUTION_FINISHED) {
        LOGE << "There was an issue executing an IMFuncOnClick function! Error code: " << r << std::endl;
    }
    ctx->Release();
    return true;
}
