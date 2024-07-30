//-----------------------------------------------------------------------------
//           Name: skeleton.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "skeleton.h"

#include <Internal/filesystem.h>
#include <Internal/timer.h>
#include <Internal/snprintf.h>

#include <Graphics/model.h>
#include <Graphics/models.h>
#include <Graphics/camera.h>
#include <Graphics/pxdebugdraw.h>

#include <Physics/bulletworld.h>
#include <Physics/bulletobject.h>
#include <Physics/physics.h>

#include <Utility/compiler_macros.h>
#include <Utility/assert.h>

#include <Math/vec3math.h>
#include <Math/enginemath.h>

#include <Asset/Asset/skeletonasset.h>
#include <Scripting/angelscript/ascollisions.h>
#include <GUI/gui.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#include <btBulletDynamicsCommon.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <cmath>
#include <set>

struct SelectedJoint {
    std::vector<int> bones;
};

static std::string LabelFromBone(int bone, const Skeleton::SimpleIKBoneMap &simple_ik_bones, const std::vector<int> &parents) {
    for (const auto &simple_ik_bone : simple_ik_bones) {
        const SimpleIKBone &ik_bone = simple_ik_bone.second;
        const char *label = simple_ik_bone.first.c_str();
        int curr_bone = ik_bone.bone_id;
        for (int i = 0; i < ik_bone.chain_length; ++i) {
            if (curr_bone == bone) {
                const int BUF_SIZE = 256;
                char temp[BUF_SIZE];
                snprintf(temp, BUF_SIZE, "%s%d", label, i);
                return std::string(temp);
            }
            curr_bone = parents[curr_bone];
            if (curr_bone == -1) {
                break;
            }
        }
    }
    return "unknown_bone";
}

void Skeleton::CreateHinge(const SelectedJoint &selected_joint, const vec3 &force_axis, float *initial_angle_ptr) {
    int shared_point = -1;
    Bone *joint_bones[2];
    joint_bones[0] = &bones[selected_joint.bones[0]];
    joint_bones[1] = &bones[selected_joint.bones[1]];
    for (int point : joint_bones[0]->points) {
        for (int j : joint_bones[1]->points) {
            if (point == j) {
                shared_point = point;
            }
        }
    }
    if (shared_point == -1) {
        std::vector<int> point_ids(2, -1);
        for (unsigned i = 0; i < points.size(); ++i) {
            if (joint_bones[0]->points[0] == (int)i) {
                point_ids[0] = i;
            }
            if (joint_bones[0]->points[1] == (int)i) {
                point_ids[1] = i;
            }
        }
        if (!parents.empty() && parents[point_ids[0]] == point_ids[1]) {
            shared_point = point_ids[1];
        } else {
            shared_point = point_ids[0];
        }
    }

    std::vector<BulletObject *> selected_bullet_bones(2);
    GetPhysicsObjectsFromSelectedJoint(selected_bullet_bones, selected_joint);
    DeleteJointOnBones(selected_bullet_bones);

    vec3 anchor = points[shared_point];
    vec3 axis = force_axis;
    if (axis == vec3(0.0f)) {
        axis = normalize(ActiveCameras::Get()->GetPos() - anchor);
    }

    btTypedConstraint *bt_joint = bullet_world->AddHingeJoint(selected_bullet_bones[0], selected_bullet_bones[1], anchor, axis, 0.0f, (float)PI * 0.5f, initial_angle_ptr);

    btTypedConstraint *fixed_joint =
        bullet_world->AddFixedJoint(selected_bullet_bones[0],
                                    selected_bullet_bones[1],
                                    anchor);

    physics_joints.resize(physics_joints.size() + 1);
    physics_joints.back().type = HINGE_JOINT;
    physics_joints.back().anchor = anchor;
    physics_joints.back().bt_joint = bt_joint;
    physics_joints.back().fixed_joint = fixed_joint;
    physics_joints.back().bt_bone[0] = selected_bullet_bones[0];
    physics_joints.back().bt_bone[1] = selected_bullet_bones[1];
    physics_joints.back().stop_angle[0] = 0.0f;
    physics_joints.back().stop_angle[1] = (float)PI * 0.5f;

    mat4 rotation1 = selected_bullet_bones[0]->GetRotation();
    mat4 rotation2 = selected_bullet_bones[1]->GetRotation();

    physics_joints.back().initial_axis = axis;
}

void Skeleton::SetGravity(bool enable) {
    if (enable) {
        for (auto &physics_bone : physics_bones) {
            if (!physics_bone.bullet_object) {
                continue;
            }
            physics_bone.bullet_object->SetGravity(true);
            // physics_bones[i].bullet_object->SetDamping(0.0f);
        }
    } else {
        for (auto &physics_bone : physics_bones) {
            if (!physics_bone.bullet_object) {
                continue;
            }
            physics_bone.bullet_object->SetGravity(false);
            // physics_bones[i].bullet_object->SetDamping(1.0f);
        }
    }
}

void Skeleton::UnlinkFromBulletWorld() {
    for (auto &physics_bone : physics_bones) {
        if (physics_bone.bullet_object) {
            bullet_world->UnlinkObject(physics_bone.bullet_object);
        }
    }
    for (auto &joint : physics_joints) {
        if (joint.bt_joint) {
            bullet_world->UnlinkConstraint(joint.bt_joint);
        }
        if (joint.fixed_joint && joint.fixed_joint_enabled) {
            bullet_world->UnlinkConstraint(joint.fixed_joint);
        }
    }
    for (auto &null_constraint : null_constraints) {
        bullet_world->UnlinkConstraint(null_constraint);
    }
}

void Skeleton::LinkToBulletWorld() {
    int num = 0;
    for (unsigned i = 0; i < physics_bones.size(); i++) {
        if (!physics_bones[i].bullet_object || fixed_obj[i]) {
            continue;
        }
        bullet_world->LinkObject(physics_bones[i].bullet_object);
        ++num;
    }
    num = 0;
    int num2 = 0;
    int num3 = 0;
    for (auto &joint : physics_joints) {
        if (joint.bt_joint) {
            bullet_world->LinkConstraint(joint.bt_joint);
            if (joint.type == FIXED_JOINT) {
                ++num3;
            } else {
                ++num;
            }
        }
        if (joint.fixed_joint && joint.fixed_joint_enabled) {
            bullet_world->LinkConstraint(joint.fixed_joint);
            ++num2;
        }
    }
    num = 0;
    for (auto &null_constraint : null_constraints) {
        bullet_world->LinkConstraint(null_constraint);
        ++num;
    }
}

void Skeleton::AddModifiedSphere(btCompoundShape *compound_shape, const vec3 &scale, const vec3 &offset, const quaternion &rotation, const mat4 &capsule_transform, const vec3 &skel_offset, float body_scale) {
    quaternion quat(rotation[1], rotation[3], -rotation[2], rotation[0]);
    float radius = (scale[0] + scale[1] + scale[2]) / 3.0f * body_scale;
    vec3 modified_scale = scale / radius * body_scale;
    modified_scale = vec3(modified_scale[0], modified_scale[2], modified_scale[1]);
    btVector3 position(0.0f, 0.0f, 0.0f);
    btMultiSphereShape *sphere_shape = new btMultiSphereShape(&position, &radius, 1);
    child_shapes.push_back(sphere_shape);
    sphere_shape->setLocalScaling(btVector3(modified_scale[0], modified_scale[1], modified_scale[2]));

    quat = invert(QuaternionFromMat4(capsule_transform)) * quat;

    btTransform trans;
    trans.setIdentity();
    trans.setRotation(btQuaternion(quat.entries[0], quat.entries[1], quat.entries[2], quat.entries[3]));
    vec3 test = invert(capsule_transform) * ((vec3(offset[0], offset[2], -offset[1]) - skel_offset) * body_scale);
    trans.setOrigin(btVector3(test[0], test[1], test[2]));
    compound_shape->addChildShape(trans, sphere_shape);

    /*mat4 offset_mat;
    offset_mat.SetTranslationPart(test);
    mat4 transform = capsule_transform * offset_mat * Mat4FromQuaternion(quat);
    transform.AddTranslation(vec3(0,20,0));
    DebugDraw::Instance()->AddTransformedWireScaledSphere(transform, radius, modified_scale, vec4(1.0f), _persistent);*/
}

void Skeleton::AddModifiedCapsule(btCompoundShape *compound_shape, const vec3 &scale, const vec3 &offset, const quaternion &rotation, const mat4 &capsule_transform, const vec3 &skel_offset, float body_scale) {
    quaternion quat(rotation[1], rotation[3], -rotation[2], rotation[0]);
    float radius[2] = {scale[0] * body_scale, scale[0] * body_scale};
    btVector3 position[2];
    position[0] = btVector3(0.0f, -scale[2] * body_scale, 0.0f);
    position[1] = btVector3(0.0f, scale[2] * body_scale, 0.0f);
    btMultiSphereShape *sphere_shape = new btMultiSphereShape(position, radius, 2);
    child_shapes.push_back(sphere_shape);

    quat = invert(QuaternionFromMat4(capsule_transform)) * quat;

    btTransform trans;
    trans.setIdentity();
    trans.setRotation(btQuaternion(quat.entries[0], quat.entries[1], quat.entries[2], quat.entries[3]));
    vec3 test = invert(capsule_transform) * ((vec3(offset[0], offset[2], -offset[1]) - skel_offset) * body_scale);
    trans.setOrigin(btVector3(test[0], test[1], test[2]));
    compound_shape->addChildShape(trans, sphere_shape);

    vec3 vec_pos[2];
    vec_pos[0] = vec3(position[0][0], position[0][1], position[0][2]);
    vec_pos[1] = vec3(position[1][0], position[1][1], position[1][2]);
    vec_pos[0] = test + quat * vec_pos[0];
    vec_pos[1] = test + quat * vec_pos[1];

    /*for(int i=0; i<2; ++i){
        DebugDraw::Instance()->AddWireSphere(capsule_transform * vec_pos[i] + vec3(0,20,0), radius[i], vec4(1.0f), _persistent);
    }*/
}

void Skeleton::AddModifiedBox(btCompoundShape *compound_shape, const vec3 &scale, const vec3 &offset, const quaternion &rotation, const mat4 &capsule_transform, const vec3 &skel_offset, float body_scale) {
    quaternion quat(rotation[1], rotation[3], -rotation[2], rotation[0]);
    vec3 new_scale(scale[0] * body_scale, scale[2] * body_scale, scale[1] * body_scale);
    btBoxShape *box_shape = new btBoxShape(btVector3(new_scale[0], new_scale[1], new_scale[2]));
    child_shapes.push_back(box_shape);

    quat = invert(QuaternionFromMat4(capsule_transform)) * quat;

    btTransform trans;
    trans.setIdentity();
    trans.setRotation(btQuaternion(quat.entries[0], quat.entries[1], quat.entries[2], quat.entries[3]));
    vec3 test = invert(capsule_transform) * ((vec3(offset[0], offset[2], -offset[1]) - skel_offset) * body_scale);
    trans.setOrigin(btVector3(test[0], test[1], test[2]));
    compound_shape->addChildShape(trans, box_shape);
}

namespace FZX {
const char *key_words[] = {"l", "r", "m", "capsule", "box", "sphere", "eartip", "earbase", "head", "chest", "upperarm", "forearm", "abdomen", "hip", "thigh", "shin", "foot", "tail1", "tail2", "tail3", "tail4", "tail5", "tail6"};
enum { left,
       right,
       mirrored,
       capsule,
       box,
       sphere,
       eartip,
       earbase,
       head,
       chest,
       upperarm,
       forearm,
       abdomen,
       hip,
       thigh,
       shin,
       foot,
       tail1,
       tail2,
       tail3,
       tail4,
       tail5,
       tail6 };
const int num_key_words = sizeof(key_words) / sizeof(key_words[0]);
}  // namespace FZX

static int GetChainMember(const std::vector<int> &parents, int tip, int depth) {
    int bone = tip;
    for (int i = 0; i < depth; ++i) {
        bone = parents[bone];
    }
    return bone;
}

enum ShapeType { NONE,
                 SPHERE,
                 CAPSULE,
                 BOX };
struct CustomShape {
    ShapeType shape;
    quaternion rotation;
    vec3 scale;
    vec3 location;
    CustomShape() : shape(NONE) {}
};
const int MAX_SHAPES_PER_BONE = 5;

void GetShapeCOMAndVolume(const CustomShape &custom_shape, const vec3 &skel_offset, float body_scale, vec3 *com, float *volume) {
    const vec3 &offset = custom_shape.location;
    *com = (vec3(offset[0], offset[2], -offset[1]) - skel_offset) * body_scale;
    switch (custom_shape.shape) {
        case CAPSULE: {
            float radius = custom_shape.scale[0] * body_scale;
            float sphere_volume = 4.0f / 3.0f * 3.1417f * pow(radius, 3);
            float cylinder_volume = 3.1417f * pow(radius, 2) * custom_shape.scale[2] * 2.0f;
            *volume = sphere_volume + cylinder_volume;
            break;
        }
        case SPHERE: {
            vec3 scale = custom_shape.scale * body_scale;
            *volume = 4.0f / 3.0f * 3.1417f * scale[0] * scale[1] * scale[2];
            break;
        }
        case BOX: {
            vec3 scale = custom_shape.scale * body_scale * 2.0f;
            *volume = scale[0] * scale[1] * scale[2];
            break;
        }
        case NONE:
            LOGW << "Requested volume of NONE shape" << std::endl;
            break;
    }
}

static void ShapesFromFZX(const FZXAssetRef &fzx_ref,
                          std::vector<CustomShape> *custom_shapes_ptr,
                          std::vector<int> *num_custom_shapes_ptr,
                          const Skeleton::SimpleIKBoneMap &simple_ik_bones,
                          const std::vector<int> &parents,
                          const std::vector<int> &symmetry) {
    std::vector<CustomShape> &custom_shapes = *custom_shapes_ptr;
    std::vector<int> &num_custom_shapes = *num_custom_shapes_ptr;
    for (auto &object : fzx_ref->objects) {
        // Parse label string
        const std::string &label = object.label;
        int last_space_index = 0;
        int side = -1;
        int bodypart = -1;
        int shape = -1;
        bool mirrored = false;
        for (int j = 0, len = label.size() + 1; j < len; ++j) {
            if (label[j] == ' ' || label[j] == '\0') {
                std::string sub_str_buf;
                sub_str_buf.resize(j - last_space_index);
                int count = 0;
                for (int k = last_space_index; k < j; ++k) {
                    char c = label[k];
                    if (c >= 'A' && c <= 'Z') {
                        c -= ('A' - 'a');
                    }
                    sub_str_buf[count] = c;
                    ++count;
                }
                for (int k = 0; k < FZX::num_key_words; ++k) {
                    bool word_match = (strcmp(FZX::key_words[k], sub_str_buf.c_str()) == 0);
                    if (word_match) {
                        switch (k) {
                            case FZX::left:
                            case FZX::right:
                                side = k;
                                break;
                            case FZX::capsule:
                            case FZX::box:
                            case FZX::sphere:
                                shape = k;
                                break;
                            case FZX::mirrored:
                                mirrored = true;
                                break;
                            default:
                                bodypart = k;
                                break;
                        };
                    }
                }
                last_space_index = j + 1;
            }
        }
        const bool print_info = false;
        if (print_info) {
            if (side != -1) {
                LOGI << "Side: " << FZX::key_words[side] << std::endl;
            } else {
                LOGI << "Side: undefined" << std::endl;
            }
            if (shape != -1) {
                LOGI << "Shape: " << FZX::key_words[shape] << std::endl;
            } else {
                LOGI << "Shape: undefined" << std::endl;
            }
            if (bodypart != -1) {
                LOGI << "Bodypart: " << FZX::key_words[bodypart] << std::endl;
            } else {
                LOGI << "Bodypart: undefined" << std::endl;
            }
        }
        if (shape != -1 && bodypart != -1) {
            int bone = -1;
            switch (bodypart) {
                case FZX::eartip:
                    if (side == FZX::right) {
                        bone = simple_ik_bones.find("rightear")->second.bone_id;
                    } else if (side == FZX::left) {
                        bone = simple_ik_bones.find("leftear")->second.bone_id;
                    }
                    break;
                case FZX::earbase:
                    if (side == FZX::right) {
                        bone = GetChainMember(parents, simple_ik_bones.find("rightear")->second.bone_id, 1);
                    } else if (side == FZX::left) {
                        bone = GetChainMember(parents, simple_ik_bones.find("leftear")->second.bone_id, 1);
                    }
                    break;
                case FZX::upperarm:
                    if (side == FZX::right) {
                        bone = GetChainMember(parents, simple_ik_bones.find("rightarm")->second.bone_id, 5);
                    } else if (side == FZX::left) {
                        bone = GetChainMember(parents, simple_ik_bones.find("leftarm")->second.bone_id, 5);
                    }
                    break;
                case FZX::forearm:
                    if (side == FZX::right) {
                        bone = GetChainMember(parents, simple_ik_bones.find("rightarm")->second.bone_id, 3);
                    } else if (side == FZX::left) {
                        bone = GetChainMember(parents, simple_ik_bones.find("leftarm")->second.bone_id, 3);
                    }
                    break;
                case FZX::head:
                    bone = simple_ik_bones.find("head")->second.bone_id;
                    break;
                case FZX::chest:
                    bone = simple_ik_bones.find("torso")->second.bone_id;
                    break;
                case FZX::abdomen:
                    bone = GetChainMember(parents, simple_ik_bones.find("torso")->second.bone_id, 1);
                    break;
                case FZX::hip:
                    bone = GetChainMember(parents, simple_ik_bones.find("torso")->second.bone_id, 2);
                    break;
                case FZX::tail1:
                    bone = GetChainMember(parents, simple_ik_bones.find("tail")->second.bone_id, simple_ik_bones.find("tail")->second.chain_length - 1);
                    break;
                case FZX::tail2:
                    bone = GetChainMember(parents, simple_ik_bones.find("tail")->second.bone_id, simple_ik_bones.find("tail")->second.chain_length - 2);
                    break;
                case FZX::tail3:
                    bone = GetChainMember(parents, simple_ik_bones.find("tail")->second.bone_id, simple_ik_bones.find("tail")->second.chain_length - 3);
                    break;
                case FZX::tail4:
                    bone = GetChainMember(parents, simple_ik_bones.find("tail")->second.bone_id, simple_ik_bones.find("tail")->second.chain_length - 4);
                    break;
                case FZX::tail5:
                    bone = GetChainMember(parents, simple_ik_bones.find("tail")->second.bone_id, simple_ik_bones.find("tail")->second.chain_length - 5);
                    break;
                case FZX::tail6:
                    bone = GetChainMember(parents, simple_ik_bones.find("tail")->second.bone_id, simple_ik_bones.find("tail")->second.chain_length - 6);
                    break;
                case FZX::thigh:
                    if (side == FZX::right) {
                        bone = GetChainMember(parents, simple_ik_bones.find("right_leg")->second.bone_id, 5);
                    } else if (side == FZX::left) {
                        bone = GetChainMember(parents, simple_ik_bones.find("left_leg")->second.bone_id, 5);
                    }
                    break;
                case FZX::shin:
                    if (side == FZX::right) {
                        bone = GetChainMember(parents, simple_ik_bones.find("right_leg")->second.bone_id, 3);
                    } else if (side == FZX::left) {
                        bone = GetChainMember(parents, simple_ik_bones.find("left_leg")->second.bone_id, 3);
                    }
                    break;
                case FZX::foot:
                    if (side == FZX::right) {
                        bone = GetChainMember(parents, simple_ik_bones.find("right_leg")->second.bone_id, 1);
                    } else if (side == FZX::left) {
                        bone = GetChainMember(parents, simple_ik_bones.find("left_leg")->second.bone_id, 1);
                    }
                    break;
            }
            if (bone != -1) {
                CustomShape &custom_shape = custom_shapes[bone * MAX_SHAPES_PER_BONE + num_custom_shapes[bone]];
                switch (shape) {
                    case FZX::sphere:
                        custom_shape.shape = SPHERE;
                        break;
                    case FZX::capsule:
                        custom_shape.shape = CAPSULE;
                        break;
                    case FZX::box:
                        custom_shape.shape = BOX;
                        break;
                }
                custom_shape.location = object.location;
                custom_shape.scale = object.scale;
                custom_shape.rotation = object.rotation;
                int symmetry_bone = symmetry[bone];
                if (mirrored && symmetry_bone != -1) {
                    CustomShape &sym_custom_shape = custom_shapes[symmetry_bone * MAX_SHAPES_PER_BONE + num_custom_shapes[symmetry_bone]];
                    sym_custom_shape = custom_shape;
                    sym_custom_shape.location[0] *= -1.0f;
                    sym_custom_shape.rotation[2] *= -1.0f;
                    sym_custom_shape.rotation[3] *= -1.0f;
                    ++num_custom_shapes[symmetry_bone];
                }
                ++num_custom_shapes[bone];
            }
        }
    }
}

void Skeleton::CreatePhysicsSkeleton(float scale, const FZXAssetRef &fzx_ref) {
    // Parse FZX file to get collision shapes for each bone
    std::vector<int> num_custom_shapes(bones.size(), 0);
    std::vector<CustomShape> custom_shapes;
    custom_shapes.resize(bones.size() * MAX_SHAPES_PER_BONE);
    if (fzx_ref.valid()) {
        ShapesFromFZX(fzx_ref, &custom_shapes, &num_custom_shapes, simple_ik_bones, parents, skeleton_asset_ref->GetData().symmetry);
    }

    std::map<int, std::vector<BulletObject *> > joints;
    std::map<int, std::vector<BulletObject *> >::iterator iter;
    for (unsigned i = 0; i < bones.size(); i++) {
        Bone *bone = &bones[i];
        vec3 bone_points[2];
        bone_points[0] = points[bone->points[0]];
        bone_points[1] = points[bone->points[1]];

        // DebugDraw::Instance()->AddLine(bone_points[0]+vec3(0,20,0), bone_points[1]+vec3(0,20,0), vec4(1.0f), _persistent);

        vec3 old_midpoint = (bone_points[1] + bone_points[0]) * 0.5f;

        BulletObject *b_capsule = NULL;
        if (!fzx_ref.valid()) {
            b_capsule = bullet_world->CreateCapsule(bone_points[0], bone_points[1], 0.05f, physics_bones[i].physics_mass);
        } else {
            if (num_custom_shapes[i] == 0) {
                b_capsule = bullet_world->CreateCapsule(bone_points[0],
                                                        bone_points[1],
                                                        0.05f,
                                                        physics_bones[i].physics_mass,
                                                        BW_NO_DYNAMIC_COLLISIONS | BW_NO_STATIC_COLLISIONS);
                b_capsule->SetVisibility(true);
            } else {
                mat4 capsule_transform = BulletWorld::GetCapsuleTransform(bone_points[0], bone_points[1]);
                btCompoundShape *compound_shape = new btCompoundShape();
                SharedShapePtr shape(compound_shape);
                float total_volume = 0.0f;
                vec3 com;
                for (int j = 0; j < num_custom_shapes[i]; ++j) {
                    CustomShape custom_shape = custom_shapes[i * MAX_SHAPES_PER_BONE + j];
                    vec3 shape_com;
                    float shape_volume;
                    GetShapeCOMAndVolume(custom_shape, skeleton_asset_ref->GetData().old_model_center, scale, &shape_com, &shape_volume);
                    com += shape_com * shape_volume;
                    total_volume += shape_volume;
                }
                com /= total_volume;
                capsule_transform.SetTranslationPart(com);
                for (int j = 0; j < num_custom_shapes[i]; ++j) {
                    CustomShape custom_shape = custom_shapes[i * MAX_SHAPES_PER_BONE + j];
                    switch (custom_shape.shape) {
                        case CAPSULE:
                            AddModifiedCapsule(compound_shape, custom_shape.scale, custom_shape.location, custom_shape.rotation, capsule_transform, skeleton_asset_ref->GetData().old_model_center, scale);
                            break;

                        case SPHERE:
                            AddModifiedSphere(compound_shape, custom_shape.scale, custom_shape.location, custom_shape.rotation, capsule_transform, skeleton_asset_ref->GetData().old_model_center, scale);
                            break;

                        case BOX:
                            AddModifiedBox(compound_shape, custom_shape.scale, custom_shape.location, custom_shape.rotation, capsule_transform, skeleton_asset_ref->GetData().old_model_center, scale);
                            break;

                        case NONE:
                            // This should never happen, if it does something's gone wrong ShapesFromFZX
                            LOG_ASSERT_MSG(false, "Unhandled case of custom shape");
                            break;
                    }
                }
                b_capsule = bullet_world->CreateRigidBody(shape, physics_bones[i].physics_mass);
                b_capsule->SetVisibility(true);
                b_capsule->SetTransform(capsule_transform);
            }
        }

        /*if(!fixed_obj[i] && has_verts_assigned[i]){
            LOGI < "Creating bone " << LabelFromBone(i, simple_ik_bones, parents) << " with mass " << physics_bones[i].physics_mass << std::endl;
        }*/

        b_capsule->SetGravity(false);
        b_capsule->color = vec3(0.8f);
        b_capsule->com_offset = invert(b_capsule->GetTransform()) * (old_midpoint) * -1.0f;

        PhysicsBone &physics_bone = physics_bones[i];
        physics_bone.bullet_object = b_capsule;
        physics_bone.bone = i;
        physics_bone.initial_position = b_capsule->GetPosition();
        physics_bone.initial_rotation = b_capsule->GetRotation();

        BulletObject *object = b_capsule;
        joints[bone->points[0]].push_back(object);
        joints[bone->points[1]].push_back(object);
    }

    int editor;
    btTypedConstraint *bt_joint;
    physics_joints.clear();
    for (iter = joints.begin(); iter != joints.end(); ++iter) {
        editor = iter->first;
        std::vector<BulletObject *> &shared_objects = iter->second;
        for (unsigned i = 0; i < shared_objects.size(); i++) {
            for (unsigned j = i + 1; j < shared_objects.size(); j++) {
                bt_joint = bullet_world->AddBallJoint(shared_objects[i],
                                                      shared_objects[j],
                                                      points[editor]);

                physics_joints.resize(physics_joints.size() + 1);
                physics_joints.back().type = BALL_JOINT;
                physics_joints.back().bt_joint = bt_joint;
                physics_joints.back().bt_bone[0] = shared_objects[i];
                physics_joints.back().bt_bone[1] = shared_objects[j];
            }
        }
    }
}

int Skeleton::GetAttachedBone(BulletObject *object) {
    for (auto &physics_bone : physics_bones) {
        if (physics_bone.bullet_object == object) {
            return physics_bone.bone;
        }
    }
    return -1;
}

int Skeleton::Read(const std::string &path, float scale, float mass_scale, const FZXAssetRef &fzx_ref) {
    // skeleton_asset_ref = SkeletonAssets::Instance()->ReturnRef(path);
    skeleton_asset_ref = Engine::Instance()->GetAssetManager()->LoadSync<SkeletonAsset>(path);
    const SkeletonFileData &data = skeleton_asset_ref->GetData();

    RiggingStage rigging_stage = data.rigging_stage;

    // Load points
    int num_points = data.points.size();
    for (int i = 0; i < num_points; i++) {
        points.push_back(data.points[i] * scale);
    }

    // Load bones that connect points
    int num_bones = data.bone_ends.size() / 2;
    bones.resize(num_bones);
    for (int i = 0; i < num_bones; i++) {
        bones[i].points[0] = data.bone_ends[i * 2 + 0];
        bones[i].points[1] = data.bone_ends[i * 2 + 1];
    }

    // Load mass of each bone
    if (!data.bone_mass.empty()) {
        for (int i = 0; i < num_bones; i++) {
            bones[i].mass = data.bone_mass[i] * mass_scale * mass_scale * mass_scale;
        }
    }

    // Load center of mass for each bone
    if (!data.bone_com.empty()) {
        for (int i = 0; i < num_bones; i++) {
            bones[i].center_of_mass = data.bone_com[i] * scale;
        }
    }

    simple_ik_bones = data.simple_ik_bones;

    if (rigging_stage == _control_joints) {
        PhysicsDispose();
        col_bullet_world.reset(new BulletWorld());
        col_bullet_world->Init();
        physics_bones.resize(bones.size());

        has_verts_assigned.clear();
        has_verts_assigned.resize(physics_bones.size(), false);
        for (unsigned i = 0; i < data.model_bone_ids.size(); ++i) {
            for (int j = 0; j < 4; ++j) {
                if (data.model_bone_weights[i][j] > 0.0f) {
                    has_verts_assigned[int(data.model_bone_ids[i][j])] = true;
                }
            }
        }

        parents = data.hier_parents;

        for (unsigned i = 0; i < physics_bones.size(); ++i) {
            physics_bones[i].physics_mass = bones[i].mass;
        }

        // Identify objects with fixed joint to parent
        fixed_obj.resize(physics_bones.size(), false);
        for (const auto &joint : data.joints) {
            if (joint.type == _fixed_joint) {
                int id[2];
                id[0] = joint.bone_id[0];
                id[1] = joint.bone_id[1];
                if (parents[id[0]] == id[1]) {
                    fixed_obj[id[0]] = true;
                    physics_bones[id[1]].physics_mass += bones[id[0]].mass;
                } else if (parents[id[1]] == id[0]) {
                    fixed_obj[id[1]] = true;
                    physics_bones[id[0]].physics_mass += bones[id[1]].mass;
                }
            }
        }

        // Remove hands and toes from physics world
        int toe_bones[2];
        toe_bones[0] = simple_ik_bones["left_leg"].bone_id;
        toe_bones[1] = simple_ik_bones["right_leg"].bone_id;
        int hand_bones[2];
        hand_bones[0] = simple_ik_bones["leftarm"].bone_id;
        hand_bones[1] = simple_ik_bones["rightarm"].bone_id;
        for (int i = 0, len = physics_bones.size(); i < len; ++i) {
            int parent = parents[i];
            int grandparent = -1;
            if (parent != -1) {
                grandparent = parents[parent];
            }
            if (i == toe_bones[0] || i == toe_bones[1]) {
                fixed_obj[i] = true;
                physics_bones[parent].physics_mass += bones[i].mass;
            }
            if (i == hand_bones[0] || i == hand_bones[1]) {
                fixed_obj[i] = true;
                physics_bones[parents[grandparent]].physics_mass += bones[i].mass;
            }
            if (parent == hand_bones[0] || parent == hand_bones[1]) {
                fixed_obj[i] = true;
                physics_bones[parents[parents[grandparent]]].physics_mass += bones[i].mass;
            }
            if (grandparent == hand_bones[0] || grandparent == hand_bones[1]) {
                fixed_obj[i] = true;
                physics_bones[parents[parents[parents[grandparent]]]].physics_mass += bones[i].mass;
            }
        }
        CreatePhysicsSkeleton(scale, fzx_ref);

        // Add joints
        int num_joints = data.joints.size();
        for (int i = 0; i < num_joints; i++) {
            const JointData &joint = data.joints[i];
            SelectedJoint selected;

            if (joint.bone_id[0] != joint.bone_id[1]) {
                selected.bones.push_back(joint.bone_id[0]);
                selected.bones.push_back(joint.bone_id[1]);

                float initial_angle = 0.0f;
                if (joint.type == _hinge_joint) {
                    CreateHinge(selected, joint.axis, &initial_angle);
                } else if (joint.type == _amotor_joint) {
                    CreateRotationalConstraint(selected);
                } else if (joint.type == _fixed_joint) {
                    CreateFixed(selected);
                }
                PhysicsJoint &physics_joint = physics_joints.back();
                physics_joint.initial_angle = initial_angle;
                physics_joint.stop_angle[0] = joint.stop_angle[0];
                physics_joint.stop_angle[1] = joint.stop_angle[1];
                if (joint.type == _amotor_joint) {
                    physics_joint.stop_angle[2] = joint.stop_angle[2];
                    physics_joint.stop_angle[3] = joint.stop_angle[3];
                    physics_joint.stop_angle[4] = joint.stop_angle[4];
                    physics_joint.stop_angle[5] = joint.stop_angle[5];
                }
                if (joint.type != _fixed_joint) {
                    ApplyJointRange(&physics_joint);
                }
            } else {
                LOGW << "Error when setting up joints in skeleton from path \"" << path << "\", trying to create a joint between bone " << joint.bone_id[0] << " and itself." << std::endl;
            }
        }
    }

    std::map<BulletObject *, int> bo_id;
    for (unsigned i = 0; i < physics_bones.size(); ++i) {
        bo_id[physics_bones[i].bullet_object] = i;
    }

    // Remove bones that are fixed in place
    for (unsigned i = 0; i < physics_bones.size(); ++i) {
        if (fixed_obj[i] && has_verts_assigned[i]) {
            PhysicsBone &pbi = physics_bones[i];
            for (auto &joint : physics_joints) {
                int id[2];
                id[0] = bo_id[joint.bt_bone[0]];
                id[1] = bo_id[joint.bt_bone[1]];
                if (id[0] != (int)i && id[1] != (int)i) {
                    continue;
                }
                int other = id[0] == (int)i ? id[1] : id[0];
                if (other == parents[i]) {
                    DeleteJoint(&joint);
                } else {
                    ShiftJoint(joint,
                               id[0] == (int)i ? parents[i] : id[0],
                               id[1] == (int)i ? parents[i] : id[1]);
                }
            }
            PhysicsBone &pb = physics_bones[parents[i]];
            pb.bullet_object->color = vec3(1.0f, 0.0f, 0.0f);
            pbi.bullet_object->color = vec3(0.5f, 0.5f, 0.5f);
        }
    }

    // Delete joints that involve fixed objects
    for (unsigned i = 0; i < physics_bones.size(); ++i) {
        if (fixed_obj[i] && has_verts_assigned[i]) {
            PhysicsBone &pbi = physics_bones[i];
            for (auto &joint : physics_joints) {
                if (joint.bt_bone[0] == pbi.bullet_object || joint.bt_bone[1] == pbi.bullet_object) {
                    DeleteJoint(&joint);
                }
            }
            bullet_world->UnlinkObjectPermanent(pbi.bullet_object);
        }
    }

    // Remove invisible bones from parent hierarchy as well as physics world
    for (int &parent : parents) {
        while (parent != -1 && !has_verts_assigned[parent]) {
            parent = parents[parent];
        }
    }
    for (unsigned i = 0; i < physics_bones.size(); ++i) {
        if (!has_verts_assigned[i]) {
            PhysicsBone &pbi = physics_bones[i];
            for (auto &joint : physics_joints) {
                if (joint.bt_bone[0] == pbi.bullet_object ||
                    joint.bt_bone[1] == pbi.bullet_object) {
                    DeleteJoint(&joint);
                }
            }
            bullet_world->RemoveObject(&pbi.bullet_object);
        }
    }

    // Add constraints that do nothing but prevent collision detection between bones
    // that start out overlapping
    AddNullConstraints();

    phys_id.clear();
    for (unsigned i = 0; i < physics_bones.size(); ++i) {
        phys_id[physics_bones[i].bullet_object] = i;
    }

    return (int)rigging_stage;
}

void Skeleton::ReduceROM(float how_much) {
    std::map<BulletObject *, int> boid;
    for (unsigned i = 0; i < physics_bones.size(); ++i) {
        boid[physics_bones[i].bullet_object] = i;
    }
    std::vector<int> affected(physics_bones.size(), 1);
    if (simple_ik_bones.find("tail") != simple_ik_bones.end()) {
        const SimpleIKBone &ik_bone = simple_ik_bones["tail"];
        int bone = ik_bone.bone_id;
        int length = ik_bone.chain_length;
        for (int i = 0; i < length; ++i) {
            affected[bone] = 0;
            bone = parents[bone];
        }
    }
    for (auto &physics_joint : physics_joints) {
        if (physics_joint.type == ROTATION_JOINT && affected[boid[physics_joint.bt_bone[0]]] == 1 && affected[boid[physics_joint.bt_bone[1]]] == 1) {
            float mid = (physics_joint.stop_angle[0] +
                         physics_joint.stop_angle[1]) *
                        0.5f;
            physics_joint.stop_angle[0] = mix(physics_joint.stop_angle[0], mid, how_much);
            physics_joint.stop_angle[1] = mix(physics_joint.stop_angle[1], mid, how_much);

            mid = (physics_joint.stop_angle[2] +
                   physics_joint.stop_angle[3]) *
                  0.5f;
            physics_joint.stop_angle[2] = mix(physics_joint.stop_angle[2], mid, how_much);
            physics_joint.stop_angle[3] = mix(physics_joint.stop_angle[3], mid, how_much);

            mid = (physics_joint.stop_angle[4] +
                   physics_joint.stop_angle[5]) *
                  0.5f;
            physics_joint.stop_angle[4] = mix(physics_joint.stop_angle[4], mid, how_much);
            physics_joint.stop_angle[5] = mix(physics_joint.stop_angle[5], mid, how_much);

            ApplyJointRange(&physics_joint);
        }
    }
}

void Skeleton::CreateFixed(const SelectedJoint &selected_joint) {
    std::vector<BulletObject *> selected_bullet_bones(2);
    GetPhysicsObjectsFromSelectedJoint(selected_bullet_bones, selected_joint);

    // It doesn't make sense to create a joint between a bone and itself.
    assert(selected_bullet_bones[0] != selected_bullet_bones[1]);

    DeleteJointOnBones(selected_bullet_bones);

    btTypedConstraint *bt_joint =
        bullet_world->AddFixedJoint(selected_bullet_bones[0],
                                    selected_bullet_bones[1],
                                    (selected_bullet_bones[0]->GetPosition() + selected_bullet_bones[1]->GetPosition()) * 0.5f);

    physics_joints.resize(physics_joints.size() + 1);
    physics_joints.back().type = FIXED_JOINT;
    physics_joints.back().bt_joint = bt_joint;
    physics_joints.back().fixed_joint = NULL;
    physics_joints.back().bt_bone[0] = selected_bullet_bones[0];
    physics_joints.back().bt_bone[1] = selected_bullet_bones[1];
}

vec3 Skeleton::GetCenterOfMass() {
    vec3 center(0.0f);
    float total_mass = 0.0f;
    for (auto &physics_bone : physics_bones) {
        if (!physics_bone.bullet_object) {
            continue;
        }
        center += physics_bone.bullet_object->GetPosition() *
                  physics_bone.bullet_object->GetMass();
        total_mass += physics_bone.bullet_object->GetMass();
    }
    center /= total_mass;
    return center;
}

void Skeleton::CreateBallJoint(const SelectedJoint &selected_joint) {
    int shared_point = -1;
    Bone *joint_bones[2];
    joint_bones[0] = &bones[selected_joint.bones[0]];
    joint_bones[1] = &bones[selected_joint.bones[1]];
    for (int point : joint_bones[0]->points) {
        for (int j : joint_bones[1]->points) {
            if (point == j) {
                shared_point = point;
            }
        }
    }
    if (shared_point == -1) {
        std::vector<int> point_ids(2, -1);
        for (unsigned i = 0; i < points.size(); ++i) {
            if (joint_bones[0]->points[0] == (int)i) {
                point_ids[0] = i;
            }
            if (joint_bones[0]->points[1] == (int)i) {
                point_ids[1] = i;
            }
        }
        if (!parents.empty() && parents[point_ids[0]] == point_ids[1]) {
            shared_point = point_ids[1];
        } else {
            shared_point = point_ids[0];
        }
    }

    std::vector<BulletObject *> selected_bullet_bones(2);
    GetPhysicsObjectsFromSelectedJoint(selected_bullet_bones, selected_joint);
    DeleteJointOnBones(selected_bullet_bones);

    vec3 anchor = points[shared_point];

    btTypedConstraint *bt_joint = bullet_world->AddBallJoint(selected_bullet_bones[0],
                                                             selected_bullet_bones[1],
                                                             anchor);

    physics_joints.resize(physics_joints.size() + 1);
    physics_joints.back().type = BALL_JOINT;
    physics_joints.back().anchor = anchor;
    physics_joints.back().bt_joint = bt_joint;
    physics_joints.back().bt_bone[0] = selected_bullet_bones[0];
    physics_joints.back().bt_bone[1] = selected_bullet_bones[1];

    mat4 rotation1 = selected_bullet_bones[0]->GetRotation();
    mat4 rotation2 = selected_bullet_bones[1]->GetRotation();
}

void Skeleton::CreateRotationalConstraint(const SelectedJoint &selected_joint) {
    int shared_point = -1;
    Bone *joint_bones[2];
    joint_bones[0] = &bones[selected_joint.bones[0]];
    joint_bones[1] = &bones[selected_joint.bones[1]];
    for (int point : joint_bones[0]->points) {
        for (int j : joint_bones[1]->points) {
            if (point == j) {
                shared_point = point;
            }
        }
    }
    if (shared_point == -1) {
        std::vector<int> point_ids(2, -1);
        for (unsigned i = 0; i < points.size(); ++i) {
            if (joint_bones[0]->points[0] == (int)i) {
                point_ids[0] = i;
            }
            if (joint_bones[0]->points[1] == (int)i) {
                point_ids[1] = i;
            }
        }
        if (!parents.empty() && parents[point_ids[0]] == point_ids[1]) {
            shared_point = point_ids[1];
        } else {
            shared_point = point_ids[0];
        }
    }

    std::vector<BulletObject *> selected_bullet_bones(2);
    GetPhysicsObjectsFromSelectedJoint(selected_bullet_bones, selected_joint);
    DeleteJointOnBones(selected_bullet_bones);

    vec3 anchor = points[shared_point];

    btTypedConstraint *bt_joint =
        bullet_world->AddAngleConstraints(selected_bullet_bones[0],
                                          selected_bullet_bones[1],
                                          anchor,
                                          (float)PI * 0.25f);

    btTypedConstraint *fixed_joint =
        bullet_world->AddFixedJoint(selected_bullet_bones[0],
                                    selected_bullet_bones[1],
                                    anchor);

    physics_joints.resize(physics_joints.size() + 1);
    physics_joints.back().type = ROTATION_JOINT;
    physics_joints.back().anchor = anchor;
    physics_joints.back().bt_joint = bt_joint;
    physics_joints.back().fixed_joint = fixed_joint;
    physics_joints.back().bt_bone[0] = selected_bullet_bones[0];
    physics_joints.back().bt_bone[1] = selected_bullet_bones[1];
    physics_joints.back().stop_angle[0] = -(float)PI * 0.25f;
    physics_joints.back().stop_angle[1] = (float)PI * 0.25f;
    physics_joints.back().stop_angle[2] = -(float)PI * 0.25f;
    physics_joints.back().stop_angle[3] = (float)PI * 0.25f;
    physics_joints.back().stop_angle[4] = -(float)PI * 0.25f;
    physics_joints.back().stop_angle[5] = (float)PI * 0.25f;

    mat4 rotation1 = selected_bullet_bones[0]->GetRotation();
    mat4 rotation2 = selected_bullet_bones[1]->GetRotation();

    physics_joints.back().initial_axis = bullet_world->GetD6Axis(bt_joint, 0);
    physics_joints.back().initial_axis2 = bullet_world->GetD6Axis(bt_joint, 2);
}

void Skeleton::ApplyJointRange(PhysicsJoint *joint) {
    if (joint->type == HINGE_JOINT) {
        btTypedConstraint *bt_joint = joint->bt_joint;
        if (bt_joint) {
            btHingeConstraint *hinge = (btHingeConstraint *)bt_joint;
            if (joint->stop_angle[0] < joint->stop_angle[1]) {
                hinge->setLimit(joint->stop_angle[0] + joint->initial_angle, joint->stop_angle[1] + joint->initial_angle);
            } else {
                hinge->setLimit(joint->stop_angle[1] + joint->initial_angle, joint->stop_angle[0] + joint->initial_angle);
            }
        }
    }
    if (joint->type == ROTATION_JOINT) {
        btTypedConstraint *bt_joint = joint->bt_joint;
        if (bt_joint) {
            btGeneric6DofConstraint *constraint = (btGeneric6DofConstraint *)bt_joint;
            constraint->setAngularLowerLimit(btVector3(joint->stop_angle[0],
                                                       joint->stop_angle[2],
                                                       joint->stop_angle[4]));
            constraint->setAngularUpperLimit(btVector3(joint->stop_angle[1],
                                                       joint->stop_angle[3],
                                                       joint->stop_angle[5]));
        }
    }
}

void Skeleton::DeleteJoint(PhysicsJoint *joint) {
    if (joint->bt_joint) {
        bullet_world->RemoveJoint(&joint->bt_joint);
    }
    if (joint->fixed_joint) {
        bullet_world->RemoveJoint(&joint->fixed_joint);
    }
}

void Skeleton::RefreshFixedJoints(const std::vector<BoneTransform> &mats) {
    for (auto &joint : physics_joints) {
        if (joint.fixed_joint) {
            BoneTransform a = mats[phys_id[joint.bt_bone[0]]];
            a.origin += a.rotation * joint.bt_bone[0]->com_offset;
            BoneTransform b = mats[phys_id[joint.bt_bone[1]]];
            b.origin += b.rotation * joint.bt_bone[1]->com_offset;
            BulletWorld::UpdateFixedJoint(joint.fixed_joint,
                                          a.GetMat4(),
                                          b.GetMat4());
        }
    }
}

void Skeleton::PhysicsDispose() {
    // Remove child shapes
    for (auto &child_shape : child_shapes) {
        delete child_shape;
    }
    child_shapes.clear();

    // Remove joints
    for (auto &joint : physics_joints) {
        if (joint.bt_joint) {
            bullet_world->RemoveJoint(&joint.bt_joint);
        }
        if (joint.fixed_joint) {
            bullet_world->RemoveJoint(&joint.fixed_joint);
        }
    }
    physics_joints.clear();

    // Remove null constraints
    for (auto &null_constraint : null_constraints) {
        bullet_world->RemoveJoint(&null_constraint);
    }
    null_constraints.clear();

    // Remove objects
    for (auto &physics_bone : physics_bones) {
        bullet_world->RemoveObject(&physics_bone.bullet_object);
        col_bullet_world->RemoveObject(&physics_bone.col_bullet_object);
    }
    physics_bones.clear();

    // Remove local physics worlds
    if (col_bullet_world.get()) {
        col_bullet_world->Dispose();
        col_bullet_world.reset(NULL);
    }
}

void Skeleton::Dispose() {
    points.clear();
    bones.clear();

    PhysicsDispose();
}

void Skeleton::SetBulletWorld(BulletWorld *_bullet_world) {
    bullet_world = _bullet_world;
}

Skeleton::~Skeleton() {
    Dispose();
}

void Skeleton::GetPhysicsObjectsFromSelectedJoint(
    std::vector<BulletObject *> &selected_bullet_bones,
    const SelectedJoint &selected_joint) {
    for (auto &physics_bone : physics_bones) {
        if (physics_bone.bone == selected_joint.bones[0]) {
            selected_bullet_bones[0] = physics_bone.bullet_object;
        }
        if (physics_bone.bone == selected_joint.bones[1]) {
            selected_bullet_bones[1] = physics_bone.bullet_object;
        }
    }
}

void Skeleton::DeleteJointOnBones(std::vector<BulletObject *> &selected_bullet_bones) {
    std::vector<PhysicsJoint>::iterator iter;
    for (iter = physics_joints.begin(); iter != physics_joints.end();) {
        if (((*iter).bt_bone[0] == selected_bullet_bones[0] &&
             (*iter).bt_bone[1] == selected_bullet_bones[1]) ||
            ((*iter).bt_bone[1] == selected_bullet_bones[0] &&
             (*iter).bt_bone[0] == selected_bullet_bones[1])) {
            DeleteJoint(&(*iter));
            iter = physics_joints.erase(iter);
        } else {
            ++iter;
        }
    }
}

std::vector<float> Skeleton::GetBoneMasses() {
    std::vector<float> bone_masses(physics_bones.size(), 0.0f);
    for (unsigned i = 0; i < bone_masses.size(); i++) {
        if (physics_bones[i].bullet_object) {
            bone_masses[i] = physics_bones[i].bullet_object->GetMass();
        }
    }
    return bone_masses;
}

struct JointBoneIDs {
    int id[2];
};

void Skeleton::SetGFStrength(PhysicsJoint &joint, float _strength) {
    if (joint.fixed_joint) {
        if (_strength == 0.0f) {
            if (joint.fixed_joint_enabled) {
                bullet_world->UnlinkConstraint(joint.fixed_joint);
                joint.fixed_joint_enabled = false;
            }
        } else {
            if (!joint.fixed_joint_enabled) {
                bullet_world->LinkConstraint(joint.fixed_joint);
                joint.fixed_joint_enabled = true;
            }
            bullet_world->SetJointStrength(joint.fixed_joint, _strength);
        }
    }
}

void Skeleton::AddNullConstraints() {
    std::map<BulletObject *, std::set<BulletObject *> > connected;

    for (auto &pj : physics_joints) {
        connected[pj.bt_bone[0]].insert(pj.bt_bone[1]);
        connected[pj.bt_bone[1]].insert(pj.bt_bone[0]);
    }

    for (unsigned i = 0; i < physics_bones.size(); i++) {
        if (fixed_obj[i]) {
            continue;
        }
        const PhysicsBone &pbi = physics_bones[i];
        std::set<BulletObject *> &connected_set = connected[pbi.bullet_object];
        for (unsigned j = i + 1; j < physics_bones.size(); j++) {
            if (fixed_obj[j]) {
                continue;
            }
            const PhysicsBone &pbj = physics_bones[j];
            if (!pbi.bullet_object || !pbj.bullet_object) {
                continue;
            }
            if (connected_set.find(pbj.bullet_object) == connected_set.end() &&
                bullet_world->CheckCollision(pbi.bullet_object, pbj.bullet_object)) {
                BulletObject *obj_a = pbi.bullet_object;
                BulletObject *obj_b = pbj.bullet_object;
                if (obj_a->collision_flags & obj_b->collision_group &&
                    obj_b->collision_flags & obj_a->collision_group) {
                    btTypedConstraint *constraint =
                        bullet_world->AddNullConstraint(obj_a, obj_b);
                    null_constraints.push_back(constraint);
                    LOGD << "Creating null constraint between " << LabelFromBone(pbi.bone, simple_ik_bones, parents) << " and " << LabelFromBone(pbj.bone, simple_ik_bones, parents) << std::endl;
                    // bullet_world->CheckCollision(pbi.bullet_object, pbj.bullet_object);
                }
            }
        }
    }
    LOGD << null_constraints.size() << " null constraints." << std::endl;
}

void Skeleton::ShiftJoint(PhysicsJoint &joint, int obj_a, int obj_b) {
    if (joint.type != HINGE_JOINT && joint.type != ROTATION_JOINT) {
        return;
    }
    btTypedConstraint *bt_joint;
    if (joint.type == HINGE_JOINT) {
        bt_joint = bullet_world->AddHingeJoint(
            physics_bones[obj_a].bullet_object,
            physics_bones[obj_b].bullet_object,
            joint.anchor,
            joint.initial_axis,
            joint.stop_angle[0],
            joint.stop_angle[1],
            &joint.initial_angle);
    } else if (joint.type == ROTATION_JOINT) {
        bt_joint = bullet_world->AddAngleConstraints(
            physics_bones[obj_a].bullet_object,
            physics_bones[obj_b].bullet_object,
            joint.anchor,
            (float)PI * 0.25f);
    } else {
        // because above we returned if joint.type was not one of these two
        __builtin_unreachable();
    }
    btTypedConstraint *fixed_joint = bullet_world->AddFixedJoint(
        physics_bones[obj_a].bullet_object,
        physics_bones[obj_b].bullet_object,
        joint.anchor);

    DeleteJoint(&joint);
    joint.bt_joint = bt_joint;
    joint.fixed_joint = fixed_joint;
    joint.bt_bone[0] = physics_bones[obj_a].bullet_object;
    joint.bt_bone[1] = physics_bones[obj_b].bullet_object;

    mat4 rotation1 = physics_bones[obj_a].bullet_object->GetRotation();
    mat4 rotation2 = physics_bones[obj_b].bullet_object->GetRotation();

    if (joint.type == ROTATION_JOINT) {
        joint.initial_axis = bullet_world->GetD6Axis(bt_joint, 0);
        joint.initial_axis2 = bullet_world->GetD6Axis(bt_joint, 2);
    }
}

void Skeleton::UpdateTwistBones(bool update_transform) {
    /*for(unsigned i=0; i<physics_bones.size(); ++i){
        if(fixed_obj[i]){
            const PhysicsBone& pbone = physics_bones[parents[i]];
            mat4 initial_parent_mat = pbone.initial_rotation;
            initial_parent_mat.SetTranslationPart(pbone.initial_position);
            mat4 initial_mat = physics_bones[i].initial_rotation;
            initial_mat.SetTranslationPart(physics_bones[i].initial_position);
            mat4 offset = invert(initial_parent_mat) * initial_mat;

            int child_id = -1;
            for(unsigned j=0; j<physics_bones.size(); ++j){
                if(i==j){
                    continue;
                }
                if(parents[j] == (int)i &&
                   physics_bones[j].bullet_object)
                {
                    child_id = j;
                    break;
                }
            }

            if(child_id == -1){
                continue;
            }
            const PhysicsBone& cbone = physics_bones[child_id];
            mat4 child_mat = cbone.bullet_object->GetTransform();
            mat4 initial_child_mat = cbone.initial_rotation;
            initial_child_mat.SetTranslationPart(cbone.initial_position);

            mat4 parent_transform = pbone.bullet_object->GetTransform();
            parent_transform.AddTranslation(parent_transform.GetRotatedvec3(pbone.bullet_object->com_offset*-1.0f));

            mat4 child_mat_local =
                invert(parent_transform) * child_mat;
            vec3 vec_x = child_mat_local.GetColumnVec3(0);
            vec3 vec_z = vec3(0.0f,0.0f,1.0f);
            vec3 vec_y = normalize(cross(vec_z, vec_x));
            vec_x = normalize(cross(vec_y, vec_z));
            mat4 rot;
            rot.SetColumn(0, vec_x);
            rot.SetColumn(1, vec_y);

            mat4 new_mat = parent_transform * offset * rot;

            physics_bones[i].bullet_object->SetTransform(new_mat);

            if(update_transform){
                physics_bones[i].bullet_object->UpdateTransform();
            }
        }
    }*/

    for (unsigned i = 0; i < physics_bones.size(); ++i) {
        if (fixed_obj[i] && physics_bones[i].bullet_object) {
            int bone = parents[i];
            for (int j = 0; j < 4; ++j) {
                if (!fixed_obj[bone] && has_verts_assigned[bone]) {
                    break;
                }
                bone = parents[bone];
            }
            const PhysicsBone &pbone = physics_bones[bone];
            mat4 initial_parent_mat = pbone.initial_rotation;
            initial_parent_mat.SetTranslationPart(pbone.initial_position);

            mat4 initial_mat = physics_bones[i].initial_rotation;
            initial_mat.SetTranslationPart(physics_bones[i].initial_position);
            mat4 offset = invert(initial_parent_mat) * initial_mat;

            mat4 curr_parent_mat = pbone.bullet_object->GetRotation();
            curr_parent_mat.SetTranslationPart(pbone.bullet_object->GetPosition());

            physics_bones[i].bullet_object->SetRotation(pbone.bullet_object->GetTransform() * offset);
            physics_bones[i].bullet_object->SetPosition(curr_parent_mat * offset * vec3());

            if (update_transform) {
                physics_bones[i].bullet_object->UpdateTransform();
            }
        }
    }
}

void Skeleton::CheckForNAN() {
    for (auto &physics_bone : physics_bones) {
        if (physics_bone.bullet_object) {
            physics_bone.bullet_object->CheckForNAN();
        }
    }
}

void Skeleton::AlternateHull(const std::string &model_name, const vec3 &old_center, float model_char_scale) {
    std::string alt_model = model_name.substr(0, model_name.size() - 4) + "_hulls.obj";
    LOGD << "Checking for " << alt_model << std::endl;
    if (!FileExists(alt_model.c_str(), kDataPaths | kModPaths)) {
        return;
    }
    int hull_id = Models::Instance()->loadModel(alt_model, _MDL_SIMPLE);
    const Model &hull_model = Models::Instance()->GetModel(hull_id);

    // Create map containing all the neighboring vertices for each vertex
    std::map<int, std::set<int> > vert_connections;
    unsigned index = 0;
    const GLuint *tri_faces;
    for (int i = 0, len = hull_model.faces.size() / 3; i < len; ++i) {
        tri_faces = &hull_model.faces[index];
        for (unsigned j = 0; j < 3; ++j) {
            vert_connections[tri_faces[j]].insert(tri_faces[(j + 1) % 3]);
            vert_connections[tri_faces[j]].insert(tri_faces[(j + 2) % 3]);
        }
        index += 3;
    }

    // For each vertex, propagate group id to all connected vertices
    std::vector<std::vector<int> > groups;
    std::vector<int> vert_remap(hull_model.vertices.size() / 3);
    std::map<int, int> group_id;
    std::queue<int> verts_to_process;
    int the_label = 0;
    int remap_id;
    for (int i = 0, len = hull_model.vertices.size() / 3; i < len; ++i) {
        if (group_id.find(i) != group_id.end()) {
            continue;
        }
        remap_id = 0;
        verts_to_process.push(i);
        while (!verts_to_process.empty()) {
            int vert_id = verts_to_process.front();
            verts_to_process.pop();
            if (group_id.find(vert_id) != group_id.end()) {
                continue;
            }
            group_id.insert(std::pair<int, int>(vert_id, the_label));
            groups.resize(the_label + 1);
            groups[the_label].push_back(vert_id);
            vert_remap[vert_id] = remap_id;
            ++remap_id;
            const std::set<int> &connections = vert_connections[vert_id];
            std::set<int>::const_iterator iter = connections.begin();
            for (; iter != vert_connections[vert_id].end(); ++iter) {
                verts_to_process.push((*iter));
            }
        }
        ++the_label;
    }

    // Calculate the average position of each group
    std::vector<vec3> group_centers(groups.size());
    for (unsigned i = 0; i < groups.size(); ++i) {
        const std::vector<int> &group = groups[i];
        vec3 center_accum(0.0f);
        for (int j : group) {
            const int vert_index = j * 3;
            center_accum += vec3(hull_model.vertices[vert_index + 0],
                                 hull_model.vertices[vert_index + 1],
                                 hull_model.vertices[vert_index + 2]);
        }
        group_centers[i] = center_accum / (float)group.size();
        group_centers[i] -= old_center;
        group_centers[i] *= model_char_scale;
    }

    std::vector<int> group_closest_bone(groups.size(), -1);
    std::vector<float> group_closest_dist(groups.size(), 0.0f);

    for (unsigned i = 0; i < physics_bones.size(); ++i) {
        if (fixed_obj[i] || !has_verts_assigned[i]) {
            continue;
        }
        int closest_group = -1;
        float closest_dist = 0.0f;
        float dist;
        for (unsigned j = 0; j < groups.size(); ++j) {
            dist = distance_squared(group_centers[j], physics_bones[i].initial_position);
            if (closest_group == -1 || dist < closest_dist) {
                closest_group = j;
                closest_dist = dist;
            }
        }
        if (group_closest_bone[closest_group] == -1 ||
            closest_dist < group_closest_dist[closest_group]) {
            group_closest_dist[closest_group] = closest_dist;
            group_closest_bone[closest_group] = i;
        }
    }

    std::vector<std::vector<vec3> > groups_verts(groups.size());
    for (unsigned i = 0; i < groups.size(); ++i) {
        std::vector<vec3> &group_verts = groups_verts[i];
        std::vector<int> &group = groups[i];
        group_verts.resize(group.size());
        int vert_index;
        for (unsigned j = 0; j < group_verts.size(); ++j) {
            vert_index = group[j] * 3;
            group_verts[j] = vec3(hull_model.vertices[vert_index + 0],
                                  hull_model.vertices[vert_index + 1],
                                  hull_model.vertices[vert_index + 2]);
            group_verts[j] -= old_center;
            group_verts[j] *= model_char_scale;
        }
    }

    for (unsigned i = 0; i < groups.size(); ++i) {
        if (group_closest_bone[i] != -1) {
            std::vector<int> faces;
            for (int j = 0, len = hull_model.faces.size() / 3; j < len; ++j) {
                if (group_id[hull_model.faces[j * 3]] == (int)i) {
                    faces.push_back(vert_remap[hull_model.faces[j * 3 + 0]]);
                    faces.push_back(vert_remap[hull_model.faces[j * 3 + 1]]);
                    faces.push_back(vert_remap[hull_model.faces[j * 3 + 2]]);
                }
            }
            PhysicsBone &pb = physics_bones[group_closest_bone[i]];
            std::vector<vec3> &group_verts = groups_verts[i];
            mat4 mat;
            mat.SetTranslationPart(pb.initial_position * -1.0f);
            mat = transpose(pb.initial_rotation) * mat;
            for (auto &group_vert : group_verts) {
                group_vert = mat * group_vert;
            }

            pb.col_bullet_object =
                col_bullet_world->CreateConvexObject(group_verts, faces);
            pb.col_bullet_object->SetVisibility(true);
            pb.col_bullet_object->owner_object = (Object *)&physics_bones[group_closest_bone[i]];
        }
    }
}

void Skeleton::GetSweptSphereCollisionCharacter(const vec3 &pos, const vec3 &pos2, float radius, SphereCollision &as_col) {
    SweptSlideCallback cb;
    col_bullet_world->GetSweptSphereCollisions(pos, pos2, radius, cb);
    as_col.adjusted_position = col_bullet_world->ApplySphereSlide(pos2,
                                                                  radius,
                                                                  cb.collision_info);
    if (cb.true_closest_hit_fraction != 1.0f) {
        LOGI << "Hit something" << std::endl;
    }
    as_col.SetFromCallback(pos, pos2, cb);
}

void Skeleton::UpdateCollisionWorldAABB() {
    col_bullet_world->UpdateAABB();
}

const btCollisionObject *Skeleton::CheckRayCollision(const vec3 &start, const vec3 &end, vec3 *point, vec3 *normal) {
    return col_bullet_world->CheckRayCollision(start, end, point, normal, false);
}

int Skeleton::CheckRayCollisionBone(const vec3 &start, const vec3 &end) {
    SimpleRayResultCallbackInfo cb;
    col_bullet_world->CheckRayCollisionInfo(start, end, cb, false);
    if (cb.contact_info.size() == 0) {
        LOGI << "Did not hit anything!" << std::endl;
        return -1;
    }
    float closest_dist = FLT_MAX;
    int closest_hit = -1;
    for (int contact_index = 0, len = cb.contact_info.size(); contact_index < len; ++contact_index) {
        BulletObject *bo = cb.contact_info[contact_index].object;
        for (unsigned bone_index = 0; bone_index < physics_bones.size(); ++bone_index) {
            if (physics_bones[bone_index].col_bullet_object == bo) {
                float dist = distance_squared(start, cb.contact_info[contact_index].point);
                if (dist < closest_dist) {
                    closest_hit = bone_index;
                    closest_dist = dist;
                }
            }
        }
    }
    if (closest_hit != -1) {
        return closest_hit;
    }

    LOGI << "Hit unknown bone!" << std::endl;
    return -1;
}

PhysicsBone::PhysicsBone() : bone(0),
                             bullet_object(NULL),
                             col_bullet_object(NULL),
                             display_scale(1.0f) {}
