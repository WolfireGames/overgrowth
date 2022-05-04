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
#pragma once
#include "im_behaviors.h"

#include <GUI/IMUI/imgui.h>
#include <GUI/IMUI/im_support.h>
#include <GUI/IMUI/im_element.h>
#include <GUI/IMUI/im_text.h>
#include <GUI/IMUI/im_image.h>
#include <GUI/IMUI/im_tween.h>

#include <Sound/sound.h>
#include <Main/engine.h>


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
struct IMFadeIn : public IMUpdateBehavior {

	IMTween* tweener;
    IMTweenType tweenType;
	uint64_t elapsed;
	uint64_t targetTime;
	float originalAlpha;

    IMFadeIn( uint64_t time, IMTweenType _tweener );

    static IMFadeIn* ASFactory( uint64_t time, IMTweenType _tweener ) {
        return new IMFadeIn(time, _tweener);
    }

    bool initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    void cleanUp( IMElement* element ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMUpdateBehavior* clone() override {
        IMFadeIn* c = new IMFadeIn(targetTime, tweenType);
        c->originalAlpha = originalAlpha;
        return c;
    }

};

/**
 * Uses rendering displacement to 'move in' the element from a given offset
 **/
struct IMMoveIn : public IMUpdateBehavior {

	IMTween* tweener;
    IMTweenType tweenType;
	uint64_t elapsed;
	uint64_t targetTime;
	vec2 offset;

    IMMoveIn( uint64_t time, vec2 _offset, IMTweenType _tweener );

    static IMMoveIn* ASFactory( uint64_t _time, vec2 _offset, IMTweenType _tweener ) {
        return new IMMoveIn( _time, _offset, _tweener);
    }

    bool initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMUpdateBehavior* clone() override {
        IMMoveIn* c = new IMMoveIn(targetTime, offset, tweenType);
        return c;
    }

};

/**
 *  Transition text by fading in and fading out
 **/
// NOTE: Target must be a text element
/**
 * Fades the element in over a given time
 **/
struct IMChangeTextFadeOutIn : public IMUpdateBehavior {

	IMTween* tweenerOut;
	IMTween* tweenerIn;

    IMTweenType tweenerOutType;
    IMTweenType tweenerInType;

	uint64_t elapsed;
	uint64_t targetTime;
	float originalAlpha;
	std::string targetText;
	bool fadeAwayDone;

    IMChangeTextFadeOutIn( uint64_t time, std::string const& _targetText, IMTweenType _tweenerOut, IMTweenType _tweenerIn );

    static IMChangeTextFadeOutIn* ASFactory( uint64_t time, std::string const& _targetText, IMTweenType _tweenerOut, IMTweenType _tweenerIn ) {
        return new IMChangeTextFadeOutIn(time, _targetText, _tweenerOut, _tweenerIn);
    }

    bool initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMUpdateBehavior* clone() override {
        IMChangeTextFadeOutIn* c = new IMChangeTextFadeOutIn(targetTime, targetText, tweenerOutType, tweenerInType);
        return c;
    }

};

/**
 *  Transition image by fading in and fading out
 **/
// NOTE: Target must be a image element

struct IMChangeImageFadeOutIn : public IMUpdateBehavior {

	IMTween* tweenerOut;
	IMTween* tweenerIn;
    IMTweenType tweenerOutType;
    IMTweenType tweenerInType;

	uint64_t elapsed;
	uint64_t targetTime;
	float originalAlpha;
	std::string targetImage;
	bool fadeAwayDone;

    IMChangeImageFadeOutIn( uint64_t time, std::string const& _targetImage, IMTweenType _tweenerOut, IMTweenType _tweenerIn );

    static IMChangeImageFadeOutIn* ASFactory( uint64_t time, std::string const& _targetImage, IMTweenType _tweenerOut, IMTweenType _tweenerIn ) {
        return new IMChangeImageFadeOutIn(time, _targetImage, _tweenerOut, _tweenerIn);
    }

    bool initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMUpdateBehavior* clone() override {
        IMChangeImageFadeOutIn* c = new IMChangeImageFadeOutIn(targetTime, targetImage, tweenerOutType, tweenerInType);
        return c;
    }


};

/*******
 *
 * Continuous update behaviors
 *
 */

/**
 * Continually pulse the alpha of this element
 **/

struct IMPulseAlpha : public IMUpdateBehavior {
	float midPoint;
	float difference;
	float speed;
	uint64_t elapsedTime;

    IMPulseAlpha( float lower, float upper, float _speed );

    static IMPulseAlpha* ASFactory( float lower, float upper, float _speed ) {
        return new IMPulseAlpha(lower, upper, _speed);
    }

    bool initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    float computeTransition( float base, float range );
    bool update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMUpdateBehavior* clone() override {
        IMPulseAlpha* c = new IMPulseAlpha(0, 0, 0);
        c->midPoint = midPoint;
        c->difference = difference;
        c->speed = speed;
        return c;
    }

};


/**
 * Continually pulse the alpha of this element's border
 **/

struct IMPulseBorderAlpha : public IMUpdateBehavior {
	float midPoint;
	float difference;
	float speed;
	uint64_t elapsedTime;

    IMPulseBorderAlpha( float lower, float upper, float _speed );

    static IMPulseBorderAlpha* ASFactory( float lower, float upper, float _speed ) {
        return new IMPulseBorderAlpha(lower, upper, _speed);
    }

    bool initialize( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    float computeTransition( float base, float range );
    bool update( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMUpdateBehavior* clone() override {
        IMPulseBorderAlpha* c = new IMPulseBorderAlpha(0, 0, 0);
        c->midPoint = midPoint;
        c->difference = difference;
        c->speed = speed;
        return c;
    }

};


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

 struct IMMouseOverScale : public IMMouseOverBehavior {

	vec4 midPoints;
	vec4 differences;
	vec4 originalColor;
	uint64_t elapsedTime;
	float speed;
	IMTween* tweener;
	IMTweenType tweenType;
	uint64_t elapsed;
	uint64_t targetTime;
	float offset;
	float oldX;
	float oldY;

 	IMMouseOverScale( uint64_t time, float _offset, IMTweenType _tweener );

 	static IMMouseOverScale* ASFactory( uint64_t _time, float _offset, IMTweenType _tweener ) {
 		return new IMMouseOverScale( _time, _offset, _tweener );
 	}

 	void onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
 	float computeTransition( float base, float range );
 	void onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
 	bool onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

 	/*******************************************************************************************/
 	/**
 	 * @brief  Create a copy of this object (respecting inheritance)
 	 *
 	 */
 	IMMouseOverBehavior* clone() override {
 				IMMouseOverScale* c = new IMMouseOverScale(targetTime, offset, tweenType);
 				c->midPoints = midPoints;
 				c->differences = differences;
 				c->speed = speed;
 				return c;
 	}

 };

 /**
  * Moves when the mouse is over
  **/

 struct IMMouseOverMove : public IMMouseOverBehavior {

			vec4 midPoints;
			vec4 differences;
			vec4 originalColor;
			uint64_t elapsedTime;
			float speed;
			IMTween* tweener;
		  IMTweenType tweenType;
			uint64_t elapsed;
			uint64_t targetTime;
			vec2 offset;

     IMMouseOverMove( uint64_t time, vec2 _offset, IMTweenType _tweener );

     static IMMouseOverMove* ASFactory( uint64_t _time, vec2 _offset, IMTweenType _tweener ) {
         return new IMMouseOverMove( _time, _offset, _tweener );
     }

		 void onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
     float computeTransition( float base, float range );
     void onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
     bool onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

     /*******************************************************************************************/
     /**
      * @brief  Create a copy of this object (respecting inheritance)
      *
      */
     IMMouseOverBehavior* clone() override {
				 IMMouseOverMove* c = new IMMouseOverMove(targetTime, offset, tweenType);
				 c->midPoints = midPoints;
				 c->differences = differences;
				 c->speed = speed;
				 return c;
     }

 };

/**
 * Shows the border when the mouse is over
 **/

struct IMMouseOverShowBorder : public IMMouseOverBehavior {

 	bool originalBorderState;

    IMMouseOverShowBorder();

    static IMMouseOverShowBorder* ASFactory() {
        return new IMMouseOverShowBorder();
    }

    void onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    void onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMMouseOverBehavior* clone() override {
        IMMouseOverShowBorder* c = new IMMouseOverShowBorder;
        return c;
    }

};


 /**
  * Causes the element to pulse between two given colors when the mouse is hovering
  **/
 struct IMMouseOverPulseColor : public IMMouseOverBehavior {

 	vec4 midPoints;
 	vec4 differences;
 	vec4 originalColor;
 	uint64_t elapsedTime;
 	float speed;

    IMMouseOverPulseColor( vec4 first, vec4 second, float _speed );

    static IMMouseOverPulseColor* ASFactory( vec4 first, vec4 second, float _speed ) {
        return new IMMouseOverPulseColor( first, second, _speed );
    }

    void onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    float computeTransition( float base, float range );
    void onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

     /*******************************************************************************************/
     /**
      * @brief  Create a copy of this object (respecting inheritance)
      *
      */
     IMMouseOverBehavior* clone() override {
         IMMouseOverPulseColor* c = new IMMouseOverPulseColor(0,0,0);
         c->midPoints = midPoints;
         c->differences = differences;
         c->speed = speed;
         return c;
     }

};

 /**
  * Causes the element's border to pulse between two given colors when the mouse is hovering
  **/
struct IMMouseOverPulseBorder : public IMMouseOverBehavior {

 	vec4 midPoints;
 	vec4 differences;
 	vec4 originalColor;
 	uint64_t elapsedTime;
 	float speed;

    IMMouseOverPulseBorder( vec4 first, vec4 second, float _speed );

    static IMMouseOverPulseBorder* ASFactory( vec4 first, vec4 second, float _speed ) {
        return new IMMouseOverPulseBorder( first, second, _speed );
    }

    void onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    float computeTransition( float base, float range );
    void onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;


    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMMouseOverBehavior* clone() override {
        IMMouseOverPulseBorder* c = new IMMouseOverPulseBorder(0,0,0);
        c->midPoints = midPoints;
        c->differences = differences;
        c->speed = speed;
        return c;
    }
};

/**
 * Just pulse the border alpha
 **/

struct IMMouseOverPulseBorderAlpha : public IMMouseOverBehavior {

	float midPoint;
	float difference;
 	float originalAlpha;
 	float speed;
 	uint64_t elapsedTime;

    IMMouseOverPulseBorderAlpha( float lower, float upper, float _speed );

    static IMMouseOverPulseBorderAlpha* ASFactory( float lower, float upper, float _speed ) {
        return new IMMouseOverPulseBorderAlpha( lower, upper, _speed );
    }

    void onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    float computeTransition( float base, float range );
    void onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMMouseOverBehavior* clone() override {
        IMMouseOverPulseBorderAlpha* c = new IMMouseOverPulseBorderAlpha(0,0,0);
        c->midPoint = midPoint;
        c->difference = difference;
        c->speed = speed;
        return c;
    }

};

/**
 * Sends a specific message on the different mouse over states
 **/
struct IMFixedMessageOnMouseOver : public IMMouseOverBehavior {

    IMMessage* enterMessage;
	IMMessage* overMessage;
 	IMMessage* leaveMessage;

 	IMFixedMessageOnMouseOver( IMMessage* _enterMessage,
 							   IMMessage* _overMessage,
                               IMMessage* _leaveMessage );

    static IMFixedMessageOnMouseOver* ASFactory( IMMessage* _enterMessage,
                                                   IMMessage* _overMessage,
                                                   IMMessage* _leaveMessage ) {
        return new IMFixedMessageOnMouseOver( _enterMessage, _overMessage, _leaveMessage );
    }

    void onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    void onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMMouseOverBehavior* clone() override {

        IMMessage* eM = NULL;
        IMMessage* oM = NULL;
        IMMessage* lM = NULL;

        if( enterMessage != NULL ) {
            eM = new IMMessage( *enterMessage );
        }
        if( overMessage != NULL ) {
            oM = new IMMessage( *overMessage );
        }
        if( enterMessage != NULL ) {
            lM = new IMMessage( *enterMessage );
        }

        IMFixedMessageOnMouseOver* c = new IMFixedMessageOnMouseOver(eM, oM, lM);
        return c;
    }

    ~IMFixedMessageOnMouseOver() override {
        if( enterMessage != NULL ) {
            enterMessage->Release();
        }
        if( overMessage != NULL ) {
            overMessage->Release();
        }
        if( leaveMessage != NULL ) {
            leaveMessage->Release();
        }
    }

};

 /**
  * Makes the element fade in from 0.0 to 1.0 alpha
  **/
 struct IMMouseOverFadeIn : public IMMouseOverBehavior {

	IMTween* tweener;
    IMTweenType tweenType;
	uint64_t elapsed;
	uint64_t targetTime;
    float targetAlpha;

    IMMouseOverFadeIn( uint64_t time, IMTweenType _tweener, float targetAlpha = 1.0f );

    static IMMouseOverFadeIn* ASFactory( uint64_t time, IMTweenType _tweener, float targetAlpha ) {
        return new IMMouseOverFadeIn(time, _tweener, targetAlpha);
    }

    void onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    float computeTransition( float base, float range );
    void onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

     /*******************************************************************************************/
     /**
      * @brief  Create a copy of this object (respecting inheritance)
      *
      */
     IMMouseOverBehavior* clone() override {
         IMMouseOverFadeIn* c = new IMMouseOverFadeIn(targetTime, tweenType);
         return c;
     }
};

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
struct IMFixedMessageOnClick : public IMMouseClickBehavior {

    IMMessage* theMessage;

    /*******************************************************************************************/
    /**
     * *brief  Various constructors
     *
     */
    IMFixedMessageOnClick( std::string const& messageName );
    IMFixedMessageOnClick( std::string const& messageName, int param );
    IMFixedMessageOnClick( std::string const& messageName, std::string param );
    IMFixedMessageOnClick( std::string const& messageName, float param );
    IMFixedMessageOnClick( IMMessage* message );

    static IMFixedMessageOnClick* ASFactory( std::string const& messageName ) {
        return new IMFixedMessageOnClick( messageName );
    }

    static IMFixedMessageOnClick* ASFactory_int( std::string const& messageName, int param ) {
        return new IMFixedMessageOnClick( messageName, param );
    }

    static IMFixedMessageOnClick* ASFactory_string( std::string const& messageName, std::string const& param ) {
        return new IMFixedMessageOnClick( messageName, param );
    }

    static IMFixedMessageOnClick* ASFactory_float( std::string const& messageName, float param ) {
        return new IMFixedMessageOnClick( messageName, param );
    }

    static IMFixedMessageOnClick* ASFactoryMessage( IMMessage* message) {
        return new IMFixedMessageOnClick( message );
    }

    bool onUp( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMMouseClickBehavior* clone() override {
        IMFixedMessageOnClick* c = new IMFixedMessageOnClick( theMessage );
        return c;
    }

    ~IMFixedMessageOnClick() override {
        if( theMessage != NULL ) {
            theMessage->Release();
        }
    }

};

struct IMMouseOverSound : public IMMouseOverBehavior {

    std::string audioFile = "";

    IMMouseOverSound(std::string const& audioFile);
    ~IMMouseOverSound() override;

    static IMMouseOverSound* ASFactory(std::string const& audioFile) {
        return new IMMouseOverSound(audioFile);
    }

    void onStart( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    void onContinue( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;
    bool onFinish( IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    IMMouseOverBehavior* clone() override {
        IMMouseOverSound* c = new IMMouseOverSound(audioFile);
        return c;
    }

};

struct IMSoundOnClick : public IMMouseClickBehavior {
    std::string audioFile;

    IMSoundOnClick(std::string const& audioFile);

    static IMSoundOnClick* ASFactory(std::string const& audioFile) {
        return new IMSoundOnClick(audioFile);
    }

    bool onUp(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) override;

    IMMouseClickBehavior* clone() override {
        IMSoundOnClick* c = new IMSoundOnClick(audioFile);
        return c;
    }
};

struct IMFuncOnClick : public IMMouseClickBehavior {
    asIScriptFunction * func = 0;

    IMFuncOnClick(asIScriptFunction * func);
    ~IMFuncOnClick() override {
        func->Release();
    }

    static IMFuncOnClick* ASFactory(asIScriptFunction * func) {
        return new IMFuncOnClick(func);
    }

    bool onUp(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) override;

    IMMouseClickBehavior* clone() override {
        IMFuncOnClick* c = new IMFuncOnClick(func);
        return c;
    }
};
