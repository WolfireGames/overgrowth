#include "ui_tools/ui_support.as"
#include "ui_tools/ui_Element.as"
#include "ui_tools/ui_Text.as"
#include "ui_tools/ui_Image.as"
#include "ui_tools/ui_GUI.as"
#include "ui_tools/ui_tween.as"


/*******
 *  
 * ui_behaviors.as
 *
 * A collection of useful behaviors for the UI tools  
 *
 */
namespace AHGUI {
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
class FadeIn : UpdateBehavior {

	TweenFunc@ tweener;
	uint64 elapsed = 0;
	uint64 targetTime;
	float originalAlpha;

	FadeIn( uint64 time, TweenFunc@ _tweener ) {
		@tweener = _tweener;
		targetTime = time;
	}

	bool initialize( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        
      	originalAlpha = element.getAlpha();
      	element.setEffectAlpha( 0.0f );
        return true;

    }

	bool update( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        
		float newValue = tweener( float(elapsed), 0.0f, originalAlpha, float( targetTime ) );
			
		element.setEffectAlpha(newValue );

		elapsed += delta;

		if( elapsed >= targetTime ) {
			// We're done so inform the element to remove this tween
			element.clearEffectAlpha();
			return false;
		}
		else {
			// keep going
			return true;	
		}

    }

    void cleanUp( Element@ element ) {
        element.clearEffectAlpha();
    }

}

/**
 * Uses rendering displacement to 'move in' the element from a given offset
 **/
class MoveIn : UpdateBehavior {

	TweenFunc@ tweener;
	uint64 elapsed = 0;
	uint64 targetTime;
	ivec2 offset;

	MoveIn( ivec2 _offset, uint64 time, TweenFunc@ _tweener ) {
		@tweener = _tweener;
		targetTime = time;
		offset = _offset;
	}

	bool initialize( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        element.setDisplacement( offset );
        return true;
    }

	bool update( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        
		elapsed += delta;

		float tweenValue = tweener( float(elapsed), 0.0f, 1.0f, float( targetTime ) );

		ivec2 newDisplacement = ivec2( int(mix( float( offset.x), 0.0, tweenValue ) ),
		 							   int(mix( float( offset.y), 0.0, tweenValue ) ) );

		element.setDisplacement( newDisplacement );

		if( elapsed >= targetTime ) {
			// We're done so inform the element to remove this tween
			element.setDisplacement( ivec2(0, 0) );
			return false;
		}
		else {
			// keep going
			return true;	
		}
    }
}

/**
 *  Transition text by fading in and fading out 
 **/
// NOTE: Target must be a text element
/**
 * Fades the element in over a given time
 **/
class ChangeTextFadeOutIn : UpdateBehavior {

	TweenFunc@ tweenerOut;
	TweenFunc@ tweenerIn;
	uint64 elapsed = 0;
	uint64 targetTime;
	float originalAlpha;
	string targetText;
	bool fadeAwayDone;

	ChangeTextFadeOutIn( uint64 time, string _targetText, TweenFunc@ _tweenerOut, TweenFunc@ _tweenerIn ) {
		@tweenerOut = _tweenerOut;
		@tweenerIn = _tweenerIn;
		targetTime = time;
		targetText = _targetText;
		fadeAwayDone = false;
	}

	bool initialize( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        
      	originalAlpha = element.getAlpha();
        return true;

    }

	bool update( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        
    	if( !fadeAwayDone ) { 
			float newValue = tweenerOut( float(elapsed), originalAlpha, 0.0f, float( targetTime ) );
				
			element.setEffectAlpha(newValue );

			elapsed += delta;

			if( elapsed >= targetTime ) {
				// We're done so change the text
				Text@ textElement = cast<Text>( element );

				textElement.setText( targetText );
				textElement.setEffectAlpha( 0.0f );
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
			float newValue = tweenerIn( float(elapsed), 0.0f, originalAlpha, float( targetTime ) );
				
			element.setEffectAlpha(newValue );

			elapsed += delta;

			if( elapsed >= targetTime ) {
				// We're done so inform the element to remove this tween
				element.clearEffectAlpha();
				return false;
			}
			else {
				// keep going
				return true;	
			}
		}
    }
}

/**
 *  Transition image by fading in and fading out 
 **/
// NOTE: Target must be a image element

class ChangeImageFadeOutIn : UpdateBehavior {

	TweenFunc@ tweenerOut;
	TweenFunc@ tweenerIn;
	uint64 elapsed = 0;
	uint64 targetTime;
	float originalAlpha;
	string targetImage;
	bool fadeAwayDone;

	ChangeImageFadeOutIn( uint64 time, string _targetImage, TweenFunc@ _tweenerOut, TweenFunc@ _tweenerIn ) {
		@tweenerOut = _tweenerOut;
		@tweenerIn = _tweenerIn;
		targetTime = time;
		targetImage = _targetImage;
		fadeAwayDone = false;
	}

	bool initialize( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        
      	originalAlpha = element.getAlpha();
        return true;

    }

	bool update( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        
    	if( !fadeAwayDone ) { 
			float newValue = tweenerOut( float(elapsed), originalAlpha, 0.0f, float( targetTime ) );
				
			element.setEffectAlpha(newValue );

			elapsed += delta;

			if( elapsed >= targetTime ) {
				// We're done so change the image
				Image@ imageElement = cast<Image>( element );

				// store the old x value so we can rescale
				int oldX = imageElement.getSizeX();

				imageElement.setImageFile( targetImage );
				imageElement.scaleToSizeX( oldX );
				imageElement.setEffectAlpha( 0.0f );
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
			float newValue = tweenerIn( float(elapsed), 0.0f, originalAlpha, float( targetTime ) );
				
			element.setEffectAlpha(newValue );

			elapsed += delta;

			if( elapsed >= targetTime ) {
				// We're done so inform the element to remove this tween
				element.clearEffectAlpha();
				return false;
			}
			else {
				// keep going
				return true;	
			}
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

class PulseAlpha : UpdateBehavior {
	float midPoint;
	float difference;
	float speed;
	uint64 elapsedTime;

	PulseAlpha( float lower, float upper, float _speed ) {
		speed = _speed;
			midPoint = (lower + upper )/2;
			difference = (lower - upper )/2;
	}

	bool initialize( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
	    return true;
	}

	float computeTransition( float base, float range ) {
		return base + sin( float( elapsedTime ) / (1000.0 / speed ) * (2.0 * pi) ) * range;
	}

	bool update( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
	    float currentAlpha = computeTransition( midPoint, difference );
		element.setEffectAlpha( currentAlpha );
		elapsedTime += delta;
		
		return true;	
		
	}
}


/**
 * Continually pulse the alpha of this element's border
 **/

class PulseBorderAlpha : UpdateBehavior {
	float midPoint;
	float difference;
	float speed;
	uint64 elapsedTime;

	PulseBorderAlpha( float lower, float upper, float _speed ) {
		speed = _speed;
			midPoint = (lower + upper )/2;
			difference = (lower - upper )/2;
	}

	bool initialize( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
	    return true;
	}

	float computeTransition( float base, float range ) {
		return base + sin( float( elapsedTime ) / (1000.0 / speed ) * (2.0 * pi) ) * range;
	}

	bool update( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
	    float currentAlpha = computeTransition( midPoint, difference );
		element.setBorderAlpha( currentAlpha );
		elapsedTime += delta;
		
		return true;	
		
	}
}




/*************************************
 *****************
 *******
 *  
 * mouse over behaviors 
 *
 *******/

/**
 * Shows the border when the mouse is over
 **/

class MouseOverShowBorder : MouseOverBehavior {

 	bool originalBorderState;

 	MouseOverShowBorder() {
 	}

    void onStart( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	originalBorderState = element.border;
    	element.showBorder();
    }

    void onContinue( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    }

    bool onFinish( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        element.showBorder( originalBorderState );
        return true;
    }

}


 /**
  * Causes the element to pulse between two given colors when the mouse is hovering
  **/
 class MouseOverPulseColor : MouseOverBehavior {

 	vec4 midPoints;
 	vec4 differences;
 	vec4 originalColor;
 	uint64 elapsedTime;
 	float speed;

 	MouseOverPulseColor( vec4 first, vec4 second, float _speed ) {

 		speed = _speed;

 		midPoints = vec4( first.x + second.x /2,
 		 				  first.y + second.y /2, 
 		 				  first.z + second.z /2,
 		 				  first.a + second.a /2 );

 		differences = vec4( (first.x - second.x)/2,
 		 				    (first.y - second.y)/2, 
 		 				    (first.z - second.z)/2,
 		 				    (first.a - second.a)/2 );


 	}

    void onStart( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	originalColor = element.getColor();
    	elapsedTime = 0;
    }

    float computeTransition( float base, float range ) {
    	return base + sin( float( elapsedTime ) / (1000.0 / speed ) * (2.0 * pi) ) * range;
    }

    void onContinue( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	vec4 currentColor = vec4( computeTransition( midPoints.x, differences.x ),
    							  computeTransition( midPoints.y, differences.y ),
    						      computeTransition( midPoints.z, differences.z ),
    							  computeTransition( midPoints.a, differences.a ) );

    	element.setEffectColor( currentColor );

    	elapsedTime += delta;
    }

    bool onFinish( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        element.clearColorEffect();
        return true;
    }
}

 /**
  * Causes the listed elements to pulse when the given object is hovered.
  **/
 class MouseOverPulseColorSubElements : MouseOverBehavior {

 	vec4 midPoints;
 	vec4 differences;
 	vec4 originalColor;
 	uint64 elapsedTime;
 	float speed;
    array<string> sub_elements;

 	MouseOverPulseColorSubElements( vec4 first, vec4 second, float _speed, array<string> _sub_elements) {

 		speed = _speed;

 		midPoints = vec4( first.x + second.x /2,
 		 				  first.y + second.y /2, 
 		 				  first.z + second.z /2,
 		 				  first.a + second.a /2 );

 		differences = vec4( (first.x - second.x)/2,
 		 				    (first.y - second.y)/2, 
 		 				    (first.z - second.z)/2,
 		 				    (first.a - second.a)/2 );

        sub_elements = _sub_elements;
 	}

    void onStart( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	originalColor = element.getColor();
    	elapsedTime = 0;
    }

    float computeTransition( float base, float range ) {
    	return base + sin( float( elapsedTime ) / (1000.0 / speed ) * (2.0 * pi) ) * range;
    }

    void onContinue( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	vec4 currentColor = vec4( computeTransition( midPoints.x, differences.x ),
    							  computeTransition( midPoints.y, differences.y ),
    						      computeTransition( midPoints.z, differences.z ),
    							  computeTransition( midPoints.a, differences.a ) );

        for( uint i = 0; i < sub_elements.size(); i++ )
        {
            Element@ el_inst = element.findElement( sub_elements[i] );
            if( el_inst !is null )
    	        el_inst.setEffectColor( currentColor );
        }

    	elapsedTime += delta;
    }

    bool onFinish( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        for( uint i = 0; i < sub_elements.size(); i++ )
        {
            Element@ el_inst = element.findElement( sub_elements[i] );
            if( el_inst !is null )
    	        el_inst.clearColorEffect();
        }
        return true;
    }
}

 /**
  * Causes the element's border to pulse between two given colors when the mouse is hovering
  **/
class MouseOverPulseBorder : MouseOverBehavior {

 	vec4 midPoints;
 	vec4 differences;
 	vec4 originalColor;
 	uint64 elapsedTime;
 	float speed;

 	MouseOverPulseBorder( vec4 first, vec4 second, float _speed ) {

 		speed = _speed;

 		midPoints = vec4( first.x + second.x /2,
 		 				  first.y + second.y /2, 
 		 				  first.z + second.z /2,
 		 				  first.a + second.a /2 );

 		differences = vec4( (first.x - second.x)/2,
 		 				    (first.y - second.y)/2, 
 		 				    (first.z - second.z)/2,
 		 				    (first.a - second.a)/2 );


 	}

    void onStart( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	originalColor = element.getBorderColor();
    	elapsedTime = 0;
    }

    float computeTransition( float base, float range ) {
    	return base + sin( float( elapsedTime ) / (1000.0 / speed ) * (2.0 * pi) ) * range;
    }

    void onContinue( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	vec4 currentColor = vec4( computeTransition( midPoints.x, differences.x ),
    							  computeTransition( midPoints.y, differences.y ),
    						      computeTransition( midPoints.z, differences.z ),
    							  computeTransition( midPoints.a, differences.a ) );

    	element.setBorderColor( currentColor );

    	elapsedTime += delta;
    }

    bool onFinish( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        element.setBorderColor( originalColor );
        return true;
    }

}

/**
 * Just pulse the border alpha 
 **/

class MouseOverPulseBorderAlpha : MouseOverBehavior {


	float midPoint;
	float difference;
 	float originalAlpha;
 	float speed;
 	uint64 elapsedTime;

 	MouseOverPulseBorderAlpha( float lower, float upper, float _speed ) {

 		speed = _speed;
 		midPoint = (lower + upper )/2;
 		difference = (lower - upper )/2;

 	}

    void onStart( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	originalAlpha = element.getBorderAlpha();
    	elapsedTime = 0;
    }

    float computeTransition( float base, float range ) {
    	return base + sin( float( elapsedTime ) / (1000.0 / speed ) * (2.0 * pi) ) * range;
    }

    void onContinue( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	float currentAlpha = computeTransition( midPoint, difference );
    	element.setBorderAlpha( currentAlpha );
    	elapsedTime += delta;
    }

    bool onFinish( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        element.setBorderAlpha( originalAlpha );
        return true;
    }

}

/**
 * Sends a specific message on the different mouse over states
 **/
class FixedMessageOnMouseOver : AHGUI::MouseOverBehavior {

    AHGUI::Message@ enterMessage;
	AHGUI::Message@ overMessage;
 	AHGUI::Message@ leaveMessage;

 	FixedMessageOnMouseOver( AHGUI::Message@ _enterMessage, 
 							 AHGUI::Message@ _overMessage, 
 							 AHGUI::Message@ _leaveMessage ) {

 		@enterMessage = @_enterMessage;
 		@overMessage = @_overMessage;
 		@leaveMessage = @_leaveMessage;

 	}

    void onStart( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	if( enterMessage !is null ) {
    		element.sendMessage( enterMessage );
    	}
    }

    void onContinue( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
    	if( overMessage !is null ) {
    		element.sendMessage( overMessage );
    	}
    }

    bool onFinish( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        if( leaveMessage !is null ) {
    		element.sendMessage( leaveMessage );
    	}
        return true;
    }

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
class FixedMessageOnClick : AHGUI::MouseClickBehavior {

    AHGUI::Message@ theMessage;

    /*******************************************************************************************/
    /**
     * @brief  Various constructors
     *
     */
    FixedMessageOnClick( string messageName ) {
        @theMessage = @AHGUI::Message(messageName);
    }

    FixedMessageOnClick( string messageName, int param ) {
        @theMessage = @AHGUI::Message(messageName, param);
    }

    FixedMessageOnClick( string messageName, string param ) {
        @theMessage = @AHGUI::Message(messageName, param);
    }

    FixedMessageOnClick( string messageName, float param ) {
        @theMessage = @AHGUI::Message(messageName, param);
    }

    FixedMessageOnClick( Message@ message ) {
        @theMessage = @message;
    }

    bool onUp( AHGUI::Element@ element, uint64 delta, ivec2 drawOffset, AHGUI::GUIState& guistate ) {
        element.sendMessage( theMessage );
        return true;
    }

}

} // namespace AHGUI


