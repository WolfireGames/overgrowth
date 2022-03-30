//-----------------------------------------------------------------------------
//           Name: menu_common_t2.as
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

string GetRandomBackground(){
	array<string> background_paths;
	int counter = 0;
	while(true){
		string path = "Textures/ui/menus/main/background_" + counter + ".jpg";
		if(FileExists("Data/" + path)){
	    	background_paths.insertLast(path);
	    	counter++;
		}else{
	    	break;
		}
	}
	if(background_paths.size() < 1){
		return "Textures/error.tga";
	}else{
		return background_paths[rand()%background_paths.size()];
	}
}
