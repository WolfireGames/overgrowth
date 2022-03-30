//-----------------------------------------------------------------------------
//           Name: disable_object.as
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

void Init(string level_name) {
}

void ReceiveMessage(string message) {
    TokenIterator token_iter;
    token_iter.Init();

    if(!token_iter.FindNextToken(message)) {
        return;
    }

    string token = token_iter.GetToken(message);

    if(token == "disable") {
        if(!token_iter.FindNextToken(message)) {
            return;
        }

        int target_object_id = atoi(token_iter.GetToken(message));

        if(ObjectExists(target_object_id)) {
            Object@ target_object = ReadObjectFromID(target_object_id);
            target_object.SetEnabled(false);
        }
    } else if(token == "enable") {
        if(!token_iter.FindNextToken(message)) {
            return;
        }

        int target_object_id = atoi(token_iter.GetToken(message));

        if(ObjectExists(target_object_id)) {
            Object@ target_object = ReadObjectFromID(target_object_id);
            target_object.SetEnabled(true);
        }
    }
}