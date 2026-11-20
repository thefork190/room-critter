/*
 * Copyright 2017 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hello_ar_application.h"

#include <android/asset_manager.h>

#include <array>
#include <cmath>
#include "critter_motion.h"

#include "arcore_c_api.h"
#include "plane_renderer.h"
#include "util.h"

namespace hello_ar {
namespace {

const glm::vec3 kWhite = {255, 255, 255};

// Assumed distance from the device camera to the surface on which user will
// try to place objects. This value affects the apparent scale of objects
// while the tracking method of the Instant Placement point is
// SCREENSPACE_WITH_APPROXIMATE_DISTANCE. Values in the [0.2, 2.0] meter
// range are a good choice for most AR experiences. Use lower values for AR
// experiences where users are expected to place objects on surfaces close
// to the camera. Use larger values for experiences where the user will
// likely be standing and trying to place an object on the ground or floor
// in front of them.

void SetColor(float r, float g, float b, float a, float* color4f) {
  color4f[0] = r;
  color4f[1] = g;
  color4f[2] = b;
  color4f[3] = a;
}

}  // namespace

HelloArApplication::HelloArApplication(AAssetManager* asset_manager)
    : asset_manager_(asset_manager) {}

HelloArApplication::~HelloArApplication() {
  if (ar_session_ != nullptr) {
    Reset();
    ArFrame_destroy(ar_frame_);
    ArSession_destroy(ar_session_);
  }
}

void HelloArApplication::OnPause() {
  LOGI("OnPause()");
  if (ar_session_ != nullptr) {
    ArSession_pause(ar_session_);
  }
}

void HelloArApplication::OnResume(JNIEnv* env, void* context, void* activity) {
  LOGI("OnResume()");

  if (ar_session_ == nullptr) {
    ArInstallStatus install_status;
    // If install was not yet requested, that means that we are resuming the
    // activity first time because of explicit user interaction (such as
    // launching the application)
    bool user_requested_install = !install_requested_;

    // === ATTENTION!  ATTENTION!  ATTENTION! ===
    // This method can and will fail in user-facing situations.  Your
    // application must handle these cases at least somewhat gracefully.  See
    // HelloAR Java sample code for reasonable behavior.
    CHECKANDTHROW(
        ArCoreApk_requestInstall(env, activity, user_requested_install,
                                 &install_status) == AR_SUCCESS,
        env, "Please install Google Play Services for AR (ARCore).");

    switch (install_status) {
      case AR_INSTALL_STATUS_INSTALLED:
        break;
      case AR_INSTALL_STATUS_INSTALL_REQUESTED:
        install_requested_ = true;
        return;
    }

    // === ATTENTION!  ATTENTION!  ATTENTION! ===
    // This method can and will fail in user-facing situations.  Your
    // application must handle these cases at least somewhat gracefully.  See
    // HelloAR Java sample code for reasonable behavior.
    CHECKANDTHROW(ArSession_create(env, context, &ar_session_) == AR_SUCCESS,
                  env, "Failed to create AR session.");

    ConfigureSession();
    ArFrame_create(ar_session_, &ar_frame_);

    ArSession_setDisplayGeometry(ar_session_, display_rotation_, width_,
                                 height_);
  }

  const ArStatus status = ArSession_resume(ar_session_);
  CHECKANDTHROW(status == AR_SUCCESS, env, "Failed to resume AR session.");
}

void HelloArApplication::OnSurfaceCreated() {
  LOGI("OnSurfaceCreated()");

  depth_texture_.CreateOnGlThread();
  background_renderer_.InitializeGlContent(asset_manager_,
                                           depth_texture_.GetTextureId());
  point_cloud_renderer_.InitializeGlContent(asset_manager_);
  andy_renderer_.InitializeGlContent(asset_manager_, "models/critter.obj",
                                     "models/critter.png");
  andy_renderer_.SetMaterialProperty(.4f, .8f, .15f, 6.f);
  andy_renderer_.SetDepthTexture(depth_texture_.GetTextureId(),
                                 depth_texture_.GetWidth(),
                                 depth_texture_.GetHeight());
  plane_renderer_.InitializeGlContent(asset_manager_);
}

void HelloArApplication::OnDisplayGeometryChanged(int display_rotation,
                                                  int width, int height) {
  LOGI("OnSurfaceChanged(%d, %d)", width, height);
  glViewport(0, 0, width, height);
  display_rotation_ = display_rotation;
  width_ = width;
  height_ = height;
  if (ar_session_ != nullptr) {
    ArSession_setDisplayGeometry(ar_session_, display_rotation, width, height);
  }
}

void HelloArApplication::OnDrawFrame(bool depthColorVisualizationEnabled,
                                     bool useDepthForOcclusion) {
  // Render the scene.
  glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  if (ar_session_ == nullptr) return;

  ArSession_setCameraTextureName(ar_session_,
                                 background_renderer_.GetTextureId());

  // Update session to get current frame and render camera background.
  if (ArSession_update(ar_session_, ar_frame_) != AR_SUCCESS) {
    LOGE("HelloArApplication::OnDrawFrame ArSession_update error");
    status_ = 0;
    return;
  }

  andy_renderer_.SetDepthTexture(depth_texture_.GetTextureId(),
                                 depth_texture_.GetWidth(),
                                 depth_texture_.GetHeight());

  ArCamera* ar_camera;
  ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);

  int32_t geometry_changed = 0;
  ArFrame_getDisplayGeometryChanged(ar_session_, ar_frame_, &geometry_changed);
  if (geometry_changed != 0 || !calculate_uv_transform_) {
    // The UV Transform represents the transformation between screenspace in
    // normalized units and screenspace in units of pixels.  Having the size of
    // each pixel is necessary in the virtual object shader, to perform
    // kernel-based blur effects.
    calculate_uv_transform_ = true;
    glm::mat3 transform = GetTextureTransformMatrix(ar_session_, ar_frame_);
    andy_renderer_.SetUvTransformMatrix(transform);
  }

  glm::mat4 view_mat;
  glm::mat4 projection_mat;
  ArCamera_getViewMatrix(ar_session_, ar_camera, glm::value_ptr(view_mat));
  ArCamera_getProjectionMatrix(ar_session_, ar_camera,
                               /*near=*/0.1f, /*far=*/100.f,
                               glm::value_ptr(projection_mat));

  background_renderer_.Draw(ar_session_, ar_frame_,
                            depthColorVisualizationEnabled);

  ArTrackingState camera_tracking_state;
  ArCamera_getTrackingState(ar_session_, ar_camera, &camera_tracking_state);
  ArCamera_release(ar_camera);

  // If the camera isn't tracking don't bother rendering other objects.
  if (camera_tracking_state != AR_TRACKING_STATE_TRACKING) {
    status_ = 0;
    return;
  }

  int32_t is_depth_supported = 0;
  ArSession_isDepthModeSupported(ar_session_, AR_DEPTH_MODE_AUTOMATIC,
                                 &is_depth_supported);
  if (is_depth_supported) {
    depth_texture_.UpdateWithDepthImageOnGlThread(*ar_session_, *ar_frame_);
  }

  const bool depth_ready = is_depth_supported && depth_texture_.IsValid();
  status_ = !is_depth_supported ? 1 : !useDepthForOcclusion ? 2 : depth_ready ? 4 : 3;
  andy_renderer_.SetDepthTexture(depth_texture_.GetTextureId(),
                                depth_texture_.GetWidth(), depth_texture_.GetHeight());
  // Get light estimation value.
  ArLightEstimate* ar_light_estimate;
  ArLightEstimateState ar_light_estimate_state;
  ArLightEstimate_create(ar_session_, &ar_light_estimate);

  ArFrame_getLightEstimate(ar_session_, ar_frame_, ar_light_estimate);
  ArLightEstimate_getState(ar_session_, ar_light_estimate,
                           &ar_light_estimate_state);

  // Set light intensity to default. Intensity value ranges from 0.0f to 1.0f.
  // The first three components are color scaling factors.
  // The last one is the average pixel intensity in gamma space.
  float color_correction[4] = {1.f, 1.f, 1.f, 1.f};
  if (ar_light_estimate_state == AR_LIGHT_ESTIMATE_STATE_VALID) {
    ArLightEstimate_getColorCorrection(ar_session_, ar_light_estimate,
                                       color_correction);
  }

  ArLightEstimate_destroy(ar_light_estimate);
  ar_light_estimate = nullptr;

  // Update and render planes.
  ArTrackableList* plane_list = nullptr;
  ArTrackableList_create(ar_session_, &plane_list);
  CHECK(plane_list != nullptr);

  ArTrackableType plane_tracked_type = AR_TRACKABLE_PLANE;
  ArSession_getAllTrackables(ar_session_, plane_tracked_type, plane_list);

  int32_t plane_list_size = 0;
  ArTrackableList_getSize(ar_session_, plane_list, &plane_list_size);
  plane_count_ = plane_list_size;

  for (int i = 0; i < plane_list_size; ++i) {
    ArTrackable* ar_trackable = nullptr;
    ArTrackableList_acquireItem(ar_session_, plane_list, i, &ar_trackable);
    ArPlane* ar_plane = ArAsPlane(ar_trackable);
    ArTrackingState out_tracking_state;
    ArTrackable_getTrackingState(ar_session_, ar_trackable,
                                 &out_tracking_state);

    ArPlane* subsume_plane;
    ArPlane_acquireSubsumedBy(ar_session_, ar_plane, &subsume_plane);
    if (subsume_plane != nullptr) {
      ArTrackable_release(ArAsTrackable(subsume_plane));
      ArTrackable_release(ar_trackable);
      continue;
    }

    if (ArTrackingState::AR_TRACKING_STATE_TRACKING != out_tracking_state) {
      ArTrackable_release(ar_trackable);
      continue;
    }

    if (anchors_.empty()) plane_renderer_.Draw(projection_mat, view_mat, *ar_session_, *ar_plane);
    ArTrackable_release(ar_trackable);
  }

  ArTrackableList_destroy(plane_list);
  plane_list = nullptr;

  andy_renderer_.setUseDepthForOcclusion(asset_manager_, useDepthForOcclusion && depth_ready);

  // Render Andy objects.
  glm::mat4 model_mat(1.0f);
  for (auto& colored_anchor : anchors_) {
    ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
    ArAnchor_getTrackingState(ar_session_, colored_anchor.anchor,
                              &tracking_state);
    if (tracking_state == AR_TRACKING_STATE_TRACKING) {

      // Render object only if the tracking state is AR_TRACKING_STATE_TRACKING.
      util::GetTransformMatrixFromAnchor(*colored_anchor.anchor, ar_session_,
                                         &model_mat);
      int64_t now_ns = 0;
      ArFrame_getTimestamp(ar_session_, ar_frame_, &now_ns);
      if (!route_start_ns_) route_start_ns_ = now_ns;
      const float t = float(now_ns - route_start_ns_) * 1e-9f;
      // Same repeatable 0.7 m route for depth on/off comparisons; not navigation.
      const auto motion = room_critter::motion(t);
      model_mat = glm::translate(model_mat, glm::vec3(motion.x, motion.y, 0.f));
      model_mat = glm::rotate(model_mat, motion.heading, glm::vec3(0.f, 1.f, 0.f));
      andy_renderer_.Draw(projection_mat, view_mat, model_mat, color_correction,
                          colored_anchor.color);
    }
  }


}

bool HelloArApplication::IsDepthSupported() {
  if (!ar_session_) return false;
  int32_t is_supported = 0;
  ArSession_isDepthModeSupported(ar_session_, AR_DEPTH_MODE_AUTOMATIC,
                                 &is_supported);
  return is_supported;
}

void HelloArApplication::ConfigureSession() {
  const bool is_depth_supported = IsDepthSupported();

  ArConfig* ar_config = nullptr;
  ArConfig_create(ar_session_, &ar_config);
  ArConfig_setPlaneFindingMode(ar_session_, ar_config, AR_PLANE_FINDING_MODE_HORIZONTAL);
  if (is_depth_supported) {
    ArConfig_setDepthMode(ar_session_, ar_config, AR_DEPTH_MODE_AUTOMATIC);
  } else {
    ArConfig_setDepthMode(ar_session_, ar_config, AR_DEPTH_MODE_DISABLED);
  }

  if (is_instant_placement_enabled_) {
    ArConfig_setInstantPlacementMode(ar_session_, ar_config,
                                     AR_INSTANT_PLACEMENT_MODE_LOCAL_Y_UP);
  } else {
    ArConfig_setInstantPlacementMode(ar_session_, ar_config,
                                     AR_INSTANT_PLACEMENT_MODE_DISABLED);
  }
  CHECK(ar_config);
  CHECK(ArSession_configure(ar_session_, ar_config) == AR_SUCCESS);
  ArConfig_destroy(ar_config);
}

void HelloArApplication::OnSettingsChange(bool is_instant_placement_enabled) {
  is_instant_placement_enabled_ = false;

  if (ar_session_ != nullptr) {
    ConfigureSession();
  }
}

void HelloArApplication::Reset() {
  for (auto& item : anchors_) {
    ArAnchor_detach(ar_session_, item.anchor);
    ArAnchor_release(item.anchor);
    ArTrackable_release(item.trackable);
  }
  anchors_.clear();
  route_start_ns_ = 0;
}

void HelloArApplication::OnTouched(float x, float y) {
  if (!ar_session_ || !ar_frame_ || status_ == 0) return;
  ArHitResultList* hits = nullptr;
  ArHitResultList_create(ar_session_, &hits);
  ArFrame_hitTest(ar_session_, ar_frame_, x, y, hits);
  int32_t count = 0;
  ArHitResultList_getSize(ar_session_, hits, &count);
  for (int i = 0; i < count; ++i) {
    ArHitResult* hit = nullptr;
    ArHitResult_create(ar_session_, &hit);
    ArHitResultList_getItem(ar_session_, hits, i, hit);
    ArTrackable* trackable = nullptr;
    ArHitResult_acquireTrackable(ar_session_, hit, &trackable);
    ArTrackableType type;
    ArTrackable_getType(ar_session_, trackable, &type);
    bool accepted = false;
    if (type == AR_TRACKABLE_PLANE) {
      ArPlaneType plane_type;
      ArPlane_getType(ar_session_, ArAsPlane(trackable), &plane_type);
      ArPose* pose = nullptr;
      ArPose_create(ar_session_, nullptr, &pose);
      ArHitResult_getHitPose(ar_session_, hit, pose);
      int32_t inside = 0;
      ArPlane_isPoseInPolygon(ar_session_, ArAsPlane(trackable), pose, &inside);
      ArPose_destroy(pose);
      if (inside && plane_type == AR_PLANE_HORIZONTAL_UPWARD_FACING) {
        ArAnchor* anchor = nullptr;
        if (ArHitResult_acquireNewAnchor(ar_session_, hit, &anchor) == AR_SUCCESS) {
          Reset();
          ColoredAnchor item{};
          item.anchor = anchor;
          item.trackable = trackable;
          SetColor(255, 255, 255, 255, item.color);
          anchors_.push_back(item);
          accepted = true;
        }
      }
    }
    if (!accepted) ArTrackable_release(trackable);
    ArHitResult_destroy(hit);
    if (accepted) break;
  }
  ArHitResultList_destroy(hits);
}

void HelloArApplication::UpdateAnchorColor(ColoredAnchor* colored_anchor) {
  ArTrackable* ar_trackable = colored_anchor->trackable;
  float* color = colored_anchor->color;

  ArTrackableType ar_trackable_type;
  ArTrackable_getType(ar_session_, ar_trackable, &ar_trackable_type);

  if (ar_trackable_type == AR_TRACKABLE_POINT) {
    SetColor(66.0f, 133.0f, 244.0f, 255.0f, color);
    return;
  }

  if (ar_trackable_type == AR_TRACKABLE_PLANE) {
    SetColor(139.0f, 195.0f, 74.0f, 255.0f, color);
    return;
  }

  if (ar_trackable_type == AR_TRACKABLE_DEPTH_POINT) {
    SetColor(199.0f, 8.0f, 65.0f, 255.0f, color);
    return;
  }

  if (ar_trackable_type == AR_TRACKABLE_INSTANT_PLACEMENT_POINT) {
    ArInstantPlacementPoint* ar_instant_placement_point =
        ArAsInstantPlacementPoint(ar_trackable);
    ArInstantPlacementPointTrackingMethod tracking_method;
    ArInstantPlacementPoint_getTrackingMethod(
        ar_session_, ar_instant_placement_point, &tracking_method);
    if (tracking_method ==
        AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING) {
      SetColor(255.0f, 255.0f, 137.0f, 255.0f, color);
      return;
    } else if (
        tracking_method ==
        AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE) {  // NOLINT
      SetColor(255.0f, 255.0f, 255.0f, 255.0f, color);
      return;
    }
  }

  // Fallback color
  SetColor(0.0f, 0.0f, 0.0f, 0.0f, color);
}

// This method returns a transformation matrix that when applied to screen space
// uvs makes them match correctly with the quad texture coords used to render
// the camera feed. It takes into account device orientation.
glm::mat3 HelloArApplication::GetTextureTransformMatrix(
    const ArSession* session, const ArFrame* frame) {
  float frameTransform[6];
  float uvTransform[9];
  // XY pairs of coordinates in NDC space that constitute the origin and points
  // along the two principal axes.
  const float ndcBasis[6] = {0, 0, 1, 0, 0, 1};
  ArFrame_transformCoordinates2d(
      session, frame, AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES, 3,
      ndcBasis, AR_COORDINATES_2D_TEXTURE_NORMALIZED, frameTransform);

  // Convert the transformed points into an affine transform and transpose it.
  float ndcOriginX = frameTransform[0];
  float ndcOriginY = frameTransform[1];
  uvTransform[0] = frameTransform[2] - ndcOriginX;
  uvTransform[1] = frameTransform[3] - ndcOriginY;
  uvTransform[2] = 0;
  uvTransform[3] = frameTransform[4] - ndcOriginX;
  uvTransform[4] = frameTransform[5] - ndcOriginY;
  uvTransform[5] = 0;
  uvTransform[6] = ndcOriginX;
  uvTransform[7] = ndcOriginY;
  uvTransform[8] = 1;

  return glm::make_mat3(uvTransform);
}
}  // namespace hello_ar
