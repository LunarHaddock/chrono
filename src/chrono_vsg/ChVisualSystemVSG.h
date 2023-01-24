// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2022 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Radu Serban, Rainer Gericke
// =============================================================================

#ifndef CH_VISUAL_SYSTEM_VSG_H
#define CH_VISUAL_SYSTEM_VSG_H

#include <iostream>
#include <string>

#include <vsg/all.h>
#include <vsgXchange/all.h>
#include <vsgImGui/RenderImGui.h>
#include <vsgImGui/SendEventsToImGui.h>
#include <vsgImGui/imgui.h>

#include "chrono/assets/ChVisualSystem.h"
#include "chrono/assets/ChVisualModel.h"
#include "chrono/physics/ChLoadContainer.h"

#include "chrono/assets/ChBoxShape.h"
#include "chrono/assets/ChSphereShape.h"
#include "chrono/assets/ChEllipsoidShape.h"
#include "chrono/assets/ChCylinderShape.h"
#include "chrono/assets/ChCapsuleShape.h"
#include "chrono/assets/ChBarrelShape.h"
#include "chrono/assets/ChConeShape.h"
#include "chrono/assets/ChTriangleMeshShape.h"
#include "chrono/assets/ChSurfaceShape.h"
#include "chrono/assets/ChObjFileShape.h"
#include "chrono/assets/ChLineShape.h"
#include "chrono/assets/ChPathShape.h"
#include "chrono/physics/ChParticleCloud.h"

#include "chrono_vsg/ChApiVSG.h"
#include "chrono_vsg/ChGuiComponentVSG.h"
#include "chrono_vsg/shapes/ShapeBuilder.h"

namespace chrono {
namespace vsg3d {

/// @addtogroup vsg_module
/// @{

/// VSG-based Chrono run-time visualization system.
class CH_VSG_API ChVisualSystemVSG : virtual public ChVisualSystem {
  public:
    ChVisualSystemVSG();
    ~ChVisualSystemVSG();

    /// Initialize the visualization system.
    virtual void Initialize();

    /// Process all visual assets in the associated ChSystem.
    /// This function is called by default by Initialize(), but can also be called later if further modifications to
    /// visualization assets occur.
    virtual void BindAll() override;

    /// Check if rendering is running.
    /// Returns `false` if the viewer was closed.
    virtual bool Run() override;

    /// Perform any necessary operations at the beginning of each rendering frame.
    virtual void BeginScene() override {}

    /// Draw all 3D shapes and GUI elements at the current frame.
    /// This function is typically called inside a loop such as
    /// <pre>
    ///    while(vis->Run()) {...}
    /// </pre>
    virtual void Render() override;

    /// Render COG frames for all bodies in the system.
    virtual void RenderCOGFrames(double axis_length = 1) override;

    void SetCOGFrameScale(double axis_length);
    void ToggleCOGFrameVisibility();

    /// End the scene draw at the end of each animation frame.
    virtual void EndScene() override {}

    /// Create a snapshot of the frame to be rendered and save it to the provided file.
    /// The file extension determines the image format.
    virtual void WriteImageToFile(const std::string& filename) override;

    // Terminate the VSG visualization.
    void Quit();

    void SetWindowSize(ChVector2<int> size);
    void SetWindowSize(int width, int height);
    void SetWindowPosition(ChVector2<int> pos);
    void SetWindowPosition(int from_left, int from_top);
    void SetWindowTitle(std::string title);
    void SetClearColor(ChColor color);
    void SetOutputScreen(int screenNum = 0);
    void SetFullscreen(bool yesno = false);
    void SetUseSkyBox(bool yesno);

    /// Draw the scene objects as wireframes.
    void SetWireFrameMode(bool mode = true) { m_wireframe = mode; }

    /// Set the camera up vector (default: Z).
    void SetCameraVertical(CameraVerticalDir upDir);

    /// Add a camera to the VSG scene.
    /// Note that currently only one camera is supported.
    virtual int AddCamera(const ChVector<>& pos, ChVector<> targ = VNULL) override;

    /// Set the location of the specified camera.
    virtual void SetCameraPosition(int id, const ChVector<>& pos) override;

    /// Set the target (look-at) point of the specified camera.
    virtual void SetCameraTarget(int id, const ChVector<>& target) override;

    /// Set the location of the current (active) camera.
    virtual void SetCameraPosition(const ChVector<>& pos) override;

    /// Set the target (look-at) point of the current (active) camera.
    virtual void SetCameraTarget(const ChVector<>& target) override;

    void SetLightIntensity(double intensity) { m_lightIntensity = ChClamp(intensity, 0.0, 1.0); }
    void SetLightDirection(double acimut, double elevation);
    void SetCameraAngleDeg(double angleDeg) { m_cameraAngleDeg = angleDeg; }
    void SetGuiFontSize(float theSize);
    void SetDecoGrid(double ustep, double vstep, int nu, int nv, ChCoordsys<> pos, ChColor col);
    int AddVisualModel(std::shared_ptr<ChVisualModel> model, const ChFrame<>& frame) override;
    int AddVisualModel(std::shared_ptr<ChVisualShape> model, const ChFrame<>& frame) override;
    void UpdateVisualModel(int id, const ChFrame<>& frame) override;
    void SetSystemSymbol(double size);
    void SetSystemSymbolPosition(ChVector<> pos);

    /// Add a user-defined GUI component.
    /// This function must be called before Initialize().
    void AddGuiComponent(std::shared_ptr<ChGuiComponentVSG> gc);

    /// Add a colorbar as a GUI element.
    /// This function must be called before Initialize().
    void AddGuiColorbar(const std::string& title, double min_val, double max_val);

    /// Toggle GUI visibility for all GUI components.
    void ToggleGuiVisibility() { m_show_gui = !m_show_gui; }

    /// Return bollean indicating whether or not GUI are visible.
    bool IsGuiVisible() const { return m_show_gui; }

  protected:
    virtual void UpdateFromMBS();

    bool m_initialized = false;
    int m_screen_num = -1;
    bool m_use_fullscreen = false;

    vsg::ref_ptr<vsg::Window> m_window;
    vsg::ref_ptr<vsg::Viewer> m_viewer;  ///< high-level VSG rendering manager
    vsg::ref_ptr<vsg::RenderGraph> m_renderGraph;

    bool m_show_gui;                                        ///< flag to toggle global GUI visibility
    std::vector<std::shared_ptr<ChGuiComponentVSG>> m_gui;  ///< list of all additional GUI components

    vsg::dvec3 m_vsg_cameraEye = vsg::dvec3(-10.0, 0.0, 0.0);
    vsg::dvec3 m_vsg_cameraTarget = vsg::dvec3(0.0, 0.0, 0.0);
    vsg::ref_ptr<vsg::LookAt> m_lookAt;
    vsg::ref_ptr<vsg::Camera> m_vsg_camera;
    bool m_camera_trackball;  ///< create a camera trackball control?

    vsg::dvec3 m_system_symbol_position = vsg::dvec3(0.0, 0.0, 0.0);
    vsg::dvec3 m_system_symbol_size = vsg::dvec3(1.0, 1.0, 1.0);
    //
    //  m_scene +- skybox, lights +- m_bodyScene
    //                            |
    //                            +- m_cogScene
    //                            |
    //                            +- m_linkScene
    //                            |
    //                            +- m_particleScene
    //                            |
    //                            +- m_decoScene
    //                            |
    //                            +- m_symbolScene
    //                            |
    //                            +- m_deformableScene
    vsg::ref_ptr<vsg::Group> m_scene;
    vsg::ref_ptr<vsg::Group> m_bodyScene;
    vsg::ref_ptr<vsg::Group> m_linkScene;
    vsg::ref_ptr<vsg::Group> m_particleScene;
    vsg::ref_ptr<vsg::Group> m_decoScene;
    vsg::ref_ptr<vsg::Group> m_symbolScene;
    vsg::ref_ptr<vsg::Group> m_deformableScene;

    vsg::ref_ptr<vsg::Switch> m_cogScene;

    vsg::ref_ptr<ShapeBuilder> m_shapeBuilder;
    vsg::ref_ptr<vsg::Builder> m_vsgBuilder;

    vsg::ref_ptr<vsg::Options> m_options;  ///< I/O related options for vsg::read/write calls

    bool m_wireframe;  ///< draw as wireframes

    bool m_capture_image;         ///< export current frame to image file
    std::string m_imageFilename;  ///< name of file to export current frame

    size_t m_num_vsgVertexList = 0;
    bool m_allowVertexTransfer = false;
    bool m_allowNormalsTransfer = false;
    bool m_allowColorsTransfer = false;
    std::vector<vsg::ref_ptr<vsg::vec3Array>> m_vsgVerticesList;
    std::vector<vsg::ref_ptr<vsg::vec3Array>> m_vsgNormalsList;
    std::vector<vsg::ref_ptr<vsg::vec4Array>> m_vsgColorsList;
    std::shared_ptr<ChTriangleMeshShape> m_mbsMesh;

  private:
    /// Utility function to populate a VSG group with shape groups (from the given visual model).
    /// The visual model may or may not be associated with a Chrono physics item.
    void PopulateGroup(vsg::ref_ptr<vsg::Group> group,
                       std::shared_ptr<ChVisualModel> model,
                       std::shared_ptr<ChPhysicsItem> phitem);

    std::map<std::size_t, vsg::ref_ptr<vsg::Node>> m_objCache;
    std::hash<std::string> m_stringHash;
    int m_windowWidth = 800;
    int m_windowHeight = 600;
    int m_windowX = 0;
    int m_windowY = 0;
    std::string m_windowTitle;
    ChColor m_clearColor;

    int m_numThreads = 16;
    vsg::ref_ptr<vsg::OperationThreads> m_loadThreads;

    vsg::ref_ptr<vsg::Group> m_particlePattern;  ///< cache for particle shape

    bool m_useSkybox;
    std::string m_skyboxPath;

    vsg::dvec3 m_cameraUpVector;
    bool m_yup;
    double m_cameraAngleDeg = 30.0;

    double m_lightIntensity = 1.0;
    double m_elevation = 0;
    double m_acimut = 0;
    // bool m_do_image_export = false;
    float m_guiFontSize = 20.0f;

    bool m_show_cog;     ///< flag to toggle COG vidibility
    double m_cog_scale;  ///< current COG frame scale

    unsigned int m_frame_number;  ///< current number of rendered frames
    double m_start_time;          ///< wallclock time at first render

    friend class ChBaseGuiComponentVSG;
};

/// @} vsg_module

}  // namespace vsg3d
}  // namespace chrono

#endif
