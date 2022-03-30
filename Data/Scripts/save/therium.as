#include "save/general.as"

bool IsSubMenuUnlocked(string submenu_name) {
    Campaign c = GetCampaign(GetCurrCampaignID());
    Parameter submenu_list = c.GetParameter()["menu_structures"][submenu_name]["levels"];
    for( uint i = 1; i < submenu_list.size(); i++ ) {
        if( IsLevelUnlocked(submenu_list[i].asString()) ) {
            return true;
        }
    }
    return false;
}
