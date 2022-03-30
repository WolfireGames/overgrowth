//-----------------------------------------------------------------------------
//           Name: levelinfo.as
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

class LevelInfo
{
    string name;
    string id;
    string file;
    string image;
	string campaign_id;
    string lock_icon = "Textures/ui/menus/main/icon-lock.png";
    ModLevel modlevel;
    int highest_diff;
    bool level_played = false;
    bool hide_stars = false;
	bool coming_soon = false;
    bool unlocked = true;
    bool last_played = false;
    bool disabled = false;
    int completed_levels = -1;
    int total_levels = -1;

    LevelInfo(ModLevel level, int _highest_diff, bool _level_played, bool _unlocked, bool _last_played)
    {
        name = level.GetTitle();
        file = level.GetPath();
        image = level.GetThumbnail();
        id = level.GetID();
        //GetHighestDifficultyFinishedCampaign(_campaign_id);
        highest_diff = _highest_diff;
        //GetLevelPlayed(level.GetID());
        level_played = _level_played;
        modlevel = level;
        unlocked = _unlocked;
        last_played = _last_played;
    }

    LevelInfo(string _file, string _name, string _image, string _campaign_id, int _highest_diff, bool _level_played, bool _unlocked, bool _last_played, int _completed_levels, int _total_levels)
    {
        name = _name;
        file = _file;
        image = _image;
		campaign_id = _campaign_id;
        id = _name;
        highest_diff = _highest_diff; 
        level_played = _level_played;
        unlocked = _unlocked;
        last_played = _last_played;
        completed_levels = _completed_levels;
        total_levels = _total_levels;
    }

	LevelInfo(string _file, string _name, string _image, bool _coming_soon, bool _unlocked, bool _last_played)
    {
        name = _name;
        file = _file;
        image = _image;
		coming_soon = _coming_soon;
        id = _name;
        highest_diff = 0;
        unlocked = _unlocked;
        last_played = _last_played;
    }

	LevelInfo(string _file, string _name, string _image, int _highest_diff, bool _unlocked, bool _last_played)
    {
        name = _name;
        file = _file;
        image = _image;
		coming_soon = false;
        id = _name;
        highest_diff = _highest_diff;
        unlocked = _unlocked;
        last_played = _last_played;
    }

	LevelInfo(string _file, string _name, string _image, int _highest_diff, bool _level_played, bool _unlocked, bool _last_played)
    {
        name = _name;
        file = _file;
        image = _image;
		coming_soon = false;
        id = _name;
        highest_diff = _highest_diff;
        level_played = _level_played;
        unlocked = _unlocked;
        last_played = _last_played;
    }

	LevelInfo(string _file, string _name, string _image, bool _unlocked, bool _last_played)
    {
        name = _name;
        file = _file;
        image = _image;
		coming_soon = false;
        id = _name;
        highest_diff = 0;
        unlocked = _unlocked;
        last_played = _last_played;
    }

    ModLevel GetModLevel() {
        return modlevel;
    }
};
