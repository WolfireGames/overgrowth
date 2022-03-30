#include "ui_tools/ui_support.as"

/*******
 *  
 * ui_Message.as
 *
 * Internal helper class for communication in adhoc GUIs as part of the UI tools  
 *
 */

namespace AHGUI {



/*******************************************************************************************/
/**
 * @brief A container for messages and its parameters
 *
 */
class Message
{
	Element@ sender;
	string name;
	array<string> stringParams;
	array<int> intParams;
	array<float> floatParams;

	/*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     */
	Message(string _name = "INVALID" ) {
		name = _name;
	}

	/*******************************************************************************************/
    /**
     * @brief  Copy constructor
     * 
     */
	Message( const Message &in other ) {
        @sender = null;
        name = other.name;
        stringParams = other.stringParams;
        intParams = other.intParams;
        floatParams = other.floatParams;
    }

    /*******************************************************************************************/
    /**
     * @brief  Assignment operator 
     * 
     */
    Message@ opAssign(const Message &in other)
	{
		@sender = @other.sender;
		name = other.name;
        stringParams = other.stringParams;
        intParams = other.intParams;
        floatParams = other.floatParams;
		return this;
	}

	// A few shortcuts for quickly filling in messages
	//  given that the majority of messages will have 0 or 1 parameters
	Message(string _name, int param ) {
		name = _name;
		intParams.insertLast( param );
	}

	Message(string _name, float param ) {
		name = _name;
		floatParams.insertLast( param );
	}

	Message(string _name, string param ) {
		name = _name;
		stringParams.insertLast( param );
	}

}

} // namespace AHGUI

