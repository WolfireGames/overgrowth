//-----------------------------------------------------------------------------
//           Name: mouse.cpp
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
#include "mouse.h"

#include <Threading/sdl_wrapper.h>
#include <Logging/logdata.h>

#include <SDL.h>

#include <memory>
#include <iostream>

Mouse::Mouse()
    : button_input_buffer_sequence_counter(0), button_input_buffer_count(0) {
    for (int i = 0; i < 2; ++i) {
        pos_[i] = 0;
        delta_[i] = 0;
    }
    wheel_delta_x_ = 0;
    wheel_delta_y_ = 0;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        double_click_timer_[i] = SDL_TS_GetTicks();
        mouse_click_count_[i] = 0;
        mouse_double_click_[i] = false;
        mouse_down_[i] = UP;
    }
}

void Mouse::Update() {
    wheel_delta_x_ = 0;
    wheel_delta_y_ = 0;
    for (int& i : delta_) {
        i = 0;
    }
    for (int i = 0; i < NUM_BUTTONS; ++i) {
        mouse_double_click_[i] = false;
        if (mouse_down_[i] == CLICKED) {
            mouse_down_[i] = HELD;
        }
    }
}

void Mouse::MouseWheelEvent(int x_amount, int y_amount) {
    wheel_delta_x_ += x_amount;
    wheel_delta_y_ += y_amount;
}

void Mouse::MouseDownEvent(int sdl_button_index) {
    AddKeyCodeBufferItem(sdl_button_index);
    const int _double_click_threshold_ticks = 300;
    int array_button_index = sdl_button_index - 1;

    if (array_button_index < NUM_BUTTONS) {
        mouse_down_[array_button_index]++;
        // handle double clicks
        if (mouse_down_[array_button_index] == CLICKED) {
            int double_click_time = SDL_TS_GetTicks() - double_click_timer_[array_button_index];
            if (double_click_time > _double_click_threshold_ticks) {
                mouse_click_count_[array_button_index] = 1;
            } else {
                mouse_click_count_[array_button_index]++;
            }
            if (mouse_click_count_[array_button_index] == 1) {
                double_click_timer_[array_button_index] = SDL_TS_GetTicks();
            } else if (mouse_click_count_[array_button_index] == 2) {
                mouse_double_click_[array_button_index] = true;
                mouse_click_count_[array_button_index] = 0;
            }
        }
    } else {
        LOGE << "Pressed button index exceeds expected possible range" << std::endl;
    }
}

void Mouse::MouseUpEvent(int sdl_button_index) {
    int array_button_index = sdl_button_index - 1;

    if (array_button_index < NUM_BUTTONS) {
        mouse_down_[array_button_index] = 0;
        mouse_double_click_[array_button_index] = false;
    }
}

void Mouse::AddKeyCodeBufferItem(int sdl_button_index) {
    if (button_input_buffer_count >= button_input_buffer_size) {
        for (unsigned i = 1; i < button_input_buffer_count; i++) {
            button_input_buffer[i - 1] = button_input_buffer[i];
        }
        button_input_buffer_count--;
    }

    button_input_buffer[button_input_buffer_count].s_id = button_input_buffer_sequence_counter++;
    button_input_buffer[button_input_buffer_count].button = (MouseButton)(sdl_button_index - 1);
    button_input_buffer_count++;
}

std::vector<Mouse::MousePress> Mouse::GetMouseInputs() {
    std::vector<Mouse::MousePress> presses;
    presses.resize(button_input_buffer_count);
    for (unsigned i = 0; i < button_input_buffer_count; i++) {
        presses[i] = button_input_buffer[i];
    }
    return presses;
}
