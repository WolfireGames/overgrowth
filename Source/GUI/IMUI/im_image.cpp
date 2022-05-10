//-----------------------------------------------------------------------------
//           Name: im_image.cpp
//      Developer: Wolfire Games LLC
//    Description: Image element class for creating adhoc GUIs as part of
//                 the UI tools
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
#include "im_image.h"

#include <GUI/IMUI/imgui.h>
#include <Math/vec2math.h>

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 */
IMImage::IMImage() : rotation(0.0),
                     skip_aspect_fitting(false),
                     center(false)

{
    IMrefCountTracker.addRefCountObject(getElementTypeName());
    setColor(vec4(1.0, 1.0, 1.0, 1.0));
}

/*******************************************************************************************/
/**
 * @brief  Copy constructor
 *
 */
IMImage::IMImage(IMImage const& other) {
    IMrefCountTracker.addRefCountObject(getElementTypeName());

    imageFileName = other.imageFileName;
    originalImageSize = other.originalImageSize;
    rotation = other.rotation;
    skip_aspect_fitting = other.skip_aspect_fitting;
    center = other.center;
    textureOffset = other.textureOffset;
    textureSize = other.textureSize;
}

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 * @param imageName Filename for the image
 *
 */
IMImage::IMImage(std::string const& imageName) : IMElement(imageName),
                                                 rotation(0.0),
                                                 skip_aspect_fitting(false),
                                                 center(false) {
    IMrefCountTracker.addRefCountObject(getElementTypeName());

    setImageFile(imageName);
    setColor(vec4(1.0, 1.0, 1.0, 1.0));
}

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 * @param imageName Path for the image
 *
 */
IMImage::IMImage(std::string const& imageName, bool abs_path) : IMElement(imageName),
                                                                rotation(0.0),
                                                                skip_aspect_fitting(false),
                                                                center(false) {
    IMrefCountTracker.addRefCountObject(getElementTypeName());

    if (abs_path) {
        setImageFileAbs(imageName);
    } else {
        setImageFile(imageName);
    }
    setColor(vec4(1.0, 1.0, 1.0, 1.0));
}

/*******************************************************************************************/
/**
 * @brief  Gets the name of the type of this element — for autonaming and debugging
 *
 * @returns name of the element type as a string
 *
 */
std::string IMImage::getElementTypeName() {
    return "Image";
}

void IMImage::setSkipAspectFitting(bool val) {
    skip_aspect_fitting = val;
}

void IMImage::setCenter(bool val) {
    center = val;
}

/*******************************************************************************************/
/**
 * @brief  Sets the source for the image
 *
 * @param _fileName
 *
 */
void IMImage::setImageFile(std::string const& _fileName) {
    if (_fileName == "") {
        return;
    }

    imageFileName = "Data/" + _fileName;
    if (!imuiImage.loadImage(imageFileName)) {
        IMDisplayError("Error", std::string("Unable to locate image " + imageFileName));
        return;
    }

    // Get the size
    originalImageSize = vec2(int(imuiImage.getTextureWidth()),
                             int(imuiImage.getTextureHeight()));

    setSize(originalImageSize);
}

void IMImage::setImageFileAbs(std::string const& _fileName) {
    if (_fileName == "") {
        return;
    }

    imageFileName = _fileName;
    if (!imuiImage.loadImage(imageFileName)) {
        IMDisplayError("Error", std::string("Unable to locate image " + imageFileName));
        return;
    }

    // Get the size
    originalImageSize = vec2(int(imuiImage.getTextureWidth()),
                             int(imuiImage.getTextureHeight()));

    setSize(originalImageSize);
}

/*******************************************************************************************/
/**
 * @brief  Rescales the image to a specified width
 *
 * @param newSize new x size
 *
 */
void IMImage::scaleToSizeX(float newSize) {
    float newYSize = (originalImageSize.y() / originalImageSize.x()) * newSize;
    setSize(vec2(newSize, newYSize));
}

/*******************************************************************************************/
/**
 * @brief  Rescales the image to a specified height
 *
 * @param newSize new y size
 *
 */
void IMImage::scaleToSizeY(float newSize) {
    float newXSize = (originalImageSize.x() / originalImageSize.y()) * newSize;
    setSize(vec2(newXSize, newSize));
}

/*******************************************************************************************/
/**
 * @brief  Rather counter-intuitively, this draws this object on the screen
 *
 * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
 * @param clipPos pixel location of upper lefthand corner of clipping region
 * @param clipSize size of clipping region
 *
 */
void IMImage::render(vec2 drawOffset, vec2 currentClipPos, vec2 currentClipSize) {
    Graphics* graphics = Graphics::Instance();

    // Make sure we have an an image and we're supposed draw it
    if (imageFileName != "" && show) {
        vec2 GUIRenderPos = vec2(0, 0);

        imuiImage.skip_aspect_fitting = skip_aspect_fitting;

        if (center) {
            float diff_x = (float)graphics->window_dims[0] - getSizeX();
            float diff_y = (float)graphics->window_dims[1] - getSizeY();
            GUIRenderPos = vec2(diff_x / 2.0f, diff_y / 2.0f);
        } else {
            GUIRenderPos = drawOffset + drawDisplacement;
        }

        if (skip_aspect_fitting) {
            imuiImage.setRenderSize(vec2(getSizeX(), getSizeY()));

            imuiImage.setPosition(vec3(GUIRenderPos.x(), GUIRenderPos.y(), (float)getZOrdering()));
        } else {
            vec2 screenRenderPos = screenMetrics.GUIToScreen(GUIRenderPos);

            imuiImage.setRenderSize(vec2((float(getSizeX()) * screenMetrics.GUItoScreenXScale),
                                         (float(getSizeY()) * screenMetrics.GUItoScreenYScale)));

            imuiImage.setPosition(vec3(screenRenderPos.x(), screenRenderPos.y(), (float)getZOrdering()));
        }

        if (isColorEffected) {
            imuiImage.setColor(effectColor);
        } else {
            imuiImage.setColor(color);
        }

        imuiImage.setRotation(rotation);

        if (textureSize.x() != 0 && textureSize.y() != 0) {
            imuiImage.setRenderOffset(vec2(textureOffset.x(), textureOffset.y()),
                                      vec2(textureSize.x(), textureSize.y()));
        }

        // Add clipping (if we need it)
        if (shouldClip && currentClipSize.x() != UNDEFINEDSIZE && currentClipSize.y() != UNDEFINEDSIZE) {
            vec2 adjustedClipPos = screenMetrics.GUIToScreen(currentClipPos);
            imuiImage.setClipping(vec2(adjustedClipPos.x(),
                                       adjustedClipPos.y()),
                                  vec2(currentClipSize.x() * screenMetrics.GUItoScreenXScale,
                                       currentClipSize.y() * screenMetrics.GUItoScreenYScale));
        }

        if (owner != NULL) {
            owner->IMGUI_IMUIContext->queueImage(imuiImage);
        }
    }

    // Call the superclass to make sure any element specific rendering is done
    IMElement::render(drawOffset, currentClipPos, currentClipSize);
}

/*******************************************************************************************/
/**
 * @brief  Updates the element
 *
 * @param delta Number of millisecond elapsed since last update
 * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
 * @param guistate The state of the GUI at this update
 *
 */
void IMImage::update(uint64_t delta, vec2 drawOffset, GUIState& guistate) {
    IMElement::update(delta, drawOffset, guistate);
}

/*******************************************************************************************/
/**
 * @brief Sets the rotation for this image
 *
 * @param Rotation (in degrees)
 *
 */
void IMImage::setRotation(float _rotation) {
    rotation = _rotation;
    imuiImage.setRotation(rotation);
}

/*******************************************************************************************/
/**
 * @brief Gets the rotation for this image
 *
 * @returns current rotation (in degrees)
 *
 */
float IMImage::getRotation() {
    return rotation;
}

/*******************************************************************************************/
/**
 * @brief  Render only part of this image
 *
 * (all coordinates are based on the original size of the image)
 *
 * @param offset position of the upper lefthand coordinate to source the rendering from
 * @param size size of the rectangle to use for the rendering
 *
 */
void IMImage::setImageOffset(vec2 offset, vec2 size) {
    textureOffset = offset;
    textureSize = size;
}

/*******************************************************************************************/
/**
 * @brief  Remove all referenced object without releaseing references
 *
 */
void IMImage::clense() {
    IMElement::clense();
}

/*******************************************************************************************/
/**
 * @brief  Destructor
 *
 */
IMImage::~IMImage() {
    IMrefCountTracker.removeRefCountObject(getElementTypeName());
}
