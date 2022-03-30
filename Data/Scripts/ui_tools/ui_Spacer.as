#include "ui_tools/ui_support.as"
#include "ui_tools/ui_Element.as"

/*******
 *  
 * ui_Spacer.as
 *
 * Blank space element class for creating adhoc GUIs as part of the UI tools  
 *
 */

namespace AHGUI {


/*******************************************************************************************/
/**
 * @brief Blank space
 *
 */
class Spacer : Element 
{
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    Spacer() {
        super();
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     * 
     * @returns name of the element type as a string
     *
     */
    string getElementTypeName() {
        return "Spacer";
    }


}

} // namespace AHGUI

