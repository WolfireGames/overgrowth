//-----------------------------------------------------------------------------
//           Name: state_machine.h
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

template <class T>
class BaseState {
   protected:
    T* actor;

   public:
    void SetActor(T* actor) {
        this->actor = actor;
    }

    virtual void OnEnter(){};
    virtual BaseState<T>* OnUpdate() = 0;
    virtual void OnExit(){};

    virtual ~BaseState() {}
};

template <class T>
class StateMachine {
   private:
    BaseState<T>* current_state = nullptr;
    BaseState<T>* start_state = nullptr;

   public:
    T actor;

   private:
    void SwitchToState(BaseState<T>* new_state) {
        if (current_state != new_state) {
            ExitState();

            current_state = new_state;
            if (new_state != nullptr) {
                new_state->SetActor(&actor);
                new_state->OnEnter();
            }
        }
    }

    void ExitState() {
        if (current_state != nullptr) {
            current_state->OnExit();
            delete current_state;
            current_state = nullptr;
        }
    }

   public:
    void Update() {
        if (current_state != nullptr) {
            BaseState<T>* new_state = current_state->OnUpdate();

            if (current_state != new_state) {
                SwitchToState(new_state);
            }
        } else {
            // When we first build the state machine, we'll be stateless but are given a pointer to the first state.
            if (start_state != nullptr) {
                SwitchToState(start_state);
                start_state = nullptr;
            }
        }
    }

    StateMachine() {
    }

    StateMachine(BaseState<T>* initial_state, T actor) : actor(actor) {
        // We can't call SwitchToState here, as that would make it fire in whatever thread we are right now
        start_state = initial_state;
    }

    ~StateMachine() {
        ExitState();
    }
};
