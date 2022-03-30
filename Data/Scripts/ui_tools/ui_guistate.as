#include "ui_tools/ui_support.as"

/*******
 *  
 * ui_guistate.as
 *
 * Internal helper class for creating adhoc GUIs as part of the UI tools  
 *
 */

namespace AHGUI {

    /*******************************************************************************************/
    /**
     * @brief The current state of the GUI passed during update
     *
     */
    class GUIState
    {
        ivec2 mousePosition;
        UIMouseState leftMouseState;
    }

} // namespace AHGUI

