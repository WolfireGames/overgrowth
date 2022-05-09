//-----------------------------------------------------------------------------
//           Name: mouse.h
//      Developer: Wolfire Games LLC
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
#pragma once

#include <cstdio>
#include <cstdint>
#include <vector>

class Mouse {
   public:
    enum ClickState {
        UP,
        CLICKED,
        HELD
    };
    enum MouseButton {
        LEFT,
        MIDDLE,
        RIGHT,
        FOURTH,
        FIFTH,
        SIXTH,
        SEVENTH,
        EIGHT,
        NINTH,
        TENTH,
        TWELFTH,
        NUM_BUTTONS
    };

    static const size_t button_input_buffer_size = 16;
    uint16_t button_input_buffer_sequence_counter;
    struct MousePress {
        uint16_t s_id;
        MouseButton button;
    };
    MousePress button_input_buffer[button_input_buffer_size];
    size_t button_input_buffer_count;

    int pos_[2];
    int delta_[2];
    int wheel_delta_x_;
    int wheel_delta_y_;
    bool mouse_double_click_[NUM_BUTTONS];
    int mouse_down_[NUM_BUTTONS];

    Mouse();
    void Update();
    void MouseDownEvent(int which_button);
    void MouseUpEvent(int sdl_button_index);
    void MouseWheelEvent(int x_amount, int y_amount);

    void AddKeyCodeBufferItem(int sdl_button_index);
    std::vector<MousePress> GetMouseInputs();

   private:
    int double_click_timer_[NUM_BUTTONS];
    int mouse_click_count_[NUM_BUTTONS];
};
