/* OpenHoW
 * Copyright (C) 2017-2019 Mark Sowden <markelswo@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../engine.h"
#include "../input.h"

#include "Actor.h"
#include "Game.h"

Actor::Actor() = default;
Actor::~Actor() = default;

#include <PL/platform_graphics_camera.h>

void Actor::HandleInput() {
    BaseGameMode* mode = Game_GetMode();
    if(mode == nullptr) {
        return;
    }

    Player* player = mode->GetCurrentPlayer();

    if(Input_GetActionState(player->input_slot, ACTION_MOVE_FORWARD)) {
        //position_.z += 100.f;
        position_.x += 100.f * g_state.camera->forward.x;
        position_.y += 100.f * g_state.camera->forward.y;
        position_.z += 100.f * g_state.camera->forward.z;
    } else if(Input_GetActionState(player->input_slot, ACTION_MOVE_BACKWARD)) {
        //position_.z -= 100.f;
        position_.x -= 100.f * g_state.camera->forward.x;
        position_.y -= 100.f * g_state.camera->forward.y;
        position_.z -= 100.f * g_state.camera->forward.z;
    }

    if(Input_GetActionState(player->input_slot, ACTION_SELECT)) {
        if (Input_GetActionState(player->input_slot, ACTION_TURN_LEFT)) {
            position_.x += 100.f;
        } else if (Input_GetActionState(player->input_slot, ACTION_TURN_RIGHT)) {
            position_.x -= 100.f;
        }
    } else {
        if (Input_GetActionState(player->input_slot, ACTION_TURN_LEFT)) {
            angles_.y -= 2.f;
        } else if (Input_GetActionState(player->input_slot, ACTION_TURN_RIGHT)) {
            angles_.y += 2.f;
        }
    }

    if( Input_GetActionState(player->input_slot, ACTION_JUMP) ||
        Input_GetActionState(player->input_slot, ACTION_AIM_UP)) {
        position_.y += 100.f;
    } else if(Input_GetActionState(player->input_slot, ACTION_AIM_DOWN)) {
        position_.y -= 100.f;
    }

    VecAngleClamp(&angles_);
}

void Actor::SetAngles(PLVector3 angles) {
    VecAngleClamp(&angles);
    angles_ = angles;
    /* todo: limit angles... */
}