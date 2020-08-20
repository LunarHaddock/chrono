// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Rainer Gericke
// =============================================================================
// Header for an helper class defining common methods for shape node classes
// =============================================================================

#include "chrono_vsg/shapes/ChVSGShapeFactory.h"

#include "chrono_thirdparty/stb/stb_image.h"
#include "chrono_thirdparty/filesystem/path.h"

using namespace chrono::vsg3d;

ChVSGShapeFactory::ChVSGShapeFactory() {}

vsg::ref_ptr<vsg::ShaderStage> ChVSGShapeFactory::readVertexShader(std::string filePath) {
    return vsg::ShaderStage::read(VK_SHADER_STAGE_VERTEX_BIT, "main", filePath);
}

vsg::ref_ptr<vsg::ShaderStage> ChVSGShapeFactory::readFragmentShader(std::string filePath) {
    return vsg::ShaderStage::read(VK_SHADER_STAGE_FRAGMENT_BIT, "main", filePath);
}

vsg::ref_ptr<vsg::vec4Array2D> ChVSGShapeFactory::createRGBATexture(
    std::string filePath) {  // read texture image file with stb_image

    vsg::ref_ptr<vsg::vec4Array2D> image;
    filesystem::path testPath(filePath);

    if (testPath.exists() && testPath.is_file()) {
        GetLog() << "texture file '" << filePath << " exists.\n";
        int texWidth = -1, texHeight = -1, texChannels;
        float* pixels = stbi_loadf(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        image = vsg::vec4Array2D::create(texWidth, texHeight, vsg::vec4(0, 0, 0, 0),
                                         vsg::Data::Layout{VK_FORMAT_R32G32B32A32_SFLOAT});
        if (pixels && texWidth > 0 && texHeight > 0) {
            int k = 0;
            for (int j = 0; j < texHeight; j++) {
                for (int i = 0; i < texWidth; i++) {
                    float r = pixels[k++];
                    float g = pixels[k++];
                    float b = pixels[k++];
                    float a = pixels[k++];
                    vsg::vec4 col(r, g, b, a);
                    image->set(i, j, col);
                }
            }
            // release now obsolete pixel buffer
            stbi_image_free(pixels);
        }
    } else {
        GetLog() << "texture file '" << filePath << " not found.\n";
        image = vsg::vec4Array2D::create(2, 2, vsg::vec4(1, 1, 0, 0), vsg::Data::Layout{VK_FORMAT_R32G32B32A32_SFLOAT});
        image->set(0, 0, vsg::vec4(0, 0, 1, 0));
        image->set(1, 1, vsg::vec4(0, 0, 1, 0));
    }
    return image;
}

//############################################ Textured Box #######################################################
vsg::ref_ptr<vsg::Node> ChVSGShapeFactory::createBoxTexturedNode(vsg::vec4 color,
                                                                 std::string texFilePath,
                                                                 vsg::ref_ptr<vsg::MatrixTransform> transform) {
    // set up search paths to SPIRV shaders and textures
    vsg::ref_ptr<vsg::ShaderStage> vertexShader =
        readVertexShader(GetChronoDataFile("vsg/shaders/vert_PushConstants.spv"));
    vsg::ref_ptr<vsg::ShaderStage> fragmentShader =
        readFragmentShader(GetChronoDataFile("vsg/shaders/frag_PushConstants.spv"));
    if (!vertexShader || !fragmentShader) {
        std::cout << "Could not create shaders." << std::endl;
        return {};
    }

    auto textureData = createRGBATexture(GetChronoDataFile(texFilePath));
    // set up graphics pipeline
    vsg::DescriptorSetLayoutBindings descriptorBindings{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
         nullptr}  // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
    };

    auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);
    vsg::DescriptorSetLayouts descriptorSetLayouts{descriptorSetLayout};

    vsg::PushConstantRanges pushConstantRanges{
        {VK_SHADER_STAGE_VERTEX_BIT, 0, 128}  // projection view, and model matrices, actual push constant calls
                                              // autoaatically provided by the VSG's DispatchTraversal
    };

    auto pipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

    vsg::VertexInputState::Bindings vertexBindingsDescriptions{
        VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // vertex data
        VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // colour data
        VkVertexInputBindingDescription{2, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX}   // tex coord data
    };

    vsg::VertexInputState::Attributes vertexAttributeDescriptions{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},  // vertex data
        VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},  // colour data
        VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0},     // tex coord data
    };

    vsg::GraphicsPipelineStates pipelineStates{
        vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
        vsg::InputAssemblyState::create(),
        vsg::RasterizationState::create(),
        vsg::MultisampleState::create(),
        vsg::ColorBlendState::create(),
        vsg::DepthStencilState::create()};

    auto graphicsPipeline =
        vsg::GraphicsPipeline::create(pipelineLayout, vsg::ShaderStages{vertexShader, fragmentShader}, pipelineStates);
    auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(graphicsPipeline);

    // create texture image and associated DescriptorSets and binding
    auto texture = vsg::DescriptorImage::create(vsg::Sampler::create(), textureData, 0, 0,
                                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{texture});
    auto bindDescriptorSets = vsg::BindDescriptorSets::create(
        VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->getPipelineLayout(), 0, vsg::DescriptorSets{descriptorSet});

    // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of
    // Descriptors to decorate the whole graph
    auto subgraph = vsg::StateGroup::create();
    subgraph->add(bindGraphicsPipeline);
    subgraph->add(bindDescriptorSets);

    // set up vertex and index arrays
    auto vertices = vsg::vec3Array::create(
        {{1, -1, -1},  {1, 1, -1},  {1, 1, 1},  {1, -1, 1},  {-1, 1, -1}, {-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1},
         {-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}, {1, 1, -1},  {-1, 1, -1},  {-1, 1, 1},  {1, 1, 1},
         {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1},  {1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}});

#if 0
    auto normals = vsg::vec3Array::create({{1, 0, 0},  {1, 0, 0},  {1, 0, 0},  {1, 0, 0},  {-1, 0, 0}, {-1, 0, 0},
                                           {-1, 0, 0}, {-1, 0, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0},
                                           {0, 1, 0},  {0, 1, 0},  {0, 1, 0},  {0, 1, 0},  {0, 0, 1},  {0, 0, 1},
                                           {0, 0, 1},  {0, 0, 1},  {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}});
#endif

    auto colors = vsg::vec3Array::create(vertices->size(), vsg::vec3(color.r, color.g, color.b));

    auto texcoords = vsg::vec2Array::create({{0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1},
                                             {0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1},
                                             {0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1}});

    auto indices = vsg::ushortArray::create({0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
                                             12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23});

    // VK_SHARING_MODE_EXCLUSIVE

    // setup geometry
    auto drawCommands = vsg::Commands::create();
    drawCommands->addChild(vsg::BindVertexBuffers::create(
        0, vsg::DataList{vertices, colors, texcoords}));  // shader doesn't support normals yet..
    drawCommands->addChild(vsg::BindIndexBuffer::create(indices));
    drawCommands->addChild(vsg::DrawIndexed::create(indices->size(), 1, 0, 0, 0));

    // add drawCommands to transform
    transform->addChild(drawCommands);
    subgraph->addChild(transform);

    // compile(subgraph);

    return subgraph;
}

//############################################ Textured Sphere #######################################################
vsg::ref_ptr<vsg::Node> ChVSGShapeFactory::createSphereTexturedNode(vsg::vec4 color,
                                                                    std::string texFilePath,
                                                                    vsg::ref_ptr<vsg::MatrixTransform> transform) {
    // set up search paths to SPIRV shaders and textures
    vsg::ref_ptr<vsg::ShaderStage> vertexShader =
        readVertexShader(GetChronoDataFile("vsg/shaders/vert_PushConstants.spv"));
    vsg::ref_ptr<vsg::ShaderStage> fragmentShader =
        readFragmentShader(GetChronoDataFile("vsg/shaders/frag_PushConstants.spv"));
    if (!vertexShader || !fragmentShader) {
        std::cout << "Could not create shaders." << std::endl;
        return {};
    }

    auto textureData = createRGBATexture(GetChronoDataFile(texFilePath));

    // set up graphics pipeline
    vsg::DescriptorSetLayoutBindings descriptorBindings{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
         nullptr}  // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
    };

    auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);
    vsg::DescriptorSetLayouts descriptorSetLayouts{descriptorSetLayout};

    vsg::PushConstantRanges pushConstantRanges{
        {VK_SHADER_STAGE_VERTEX_BIT, 0, 128}  // projection view, and model matrices, actual push constant calls
                                              // autoaatically provided by the VSG's DispatchTraversal
    };

    auto pipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

    vsg::VertexInputState::Bindings vertexBindingsDescriptions{
        VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // vertex data
        VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // colour data
        VkVertexInputBindingDescription{2, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX}   // tex coord data
    };

    vsg::VertexInputState::Attributes vertexAttributeDescriptions{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},  // vertex data
        VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},  // colour data
        VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0},     // tex coord data
    };

    vsg::GraphicsPipelineStates pipelineStates{
        vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
        vsg::InputAssemblyState::create(),
        vsg::RasterizationState::create(),
        vsg::MultisampleState::create(),
        vsg::ColorBlendState::create(),
        vsg::DepthStencilState::create()};

    auto graphicsPipeline =
        vsg::GraphicsPipeline::create(pipelineLayout, vsg::ShaderStages{vertexShader, fragmentShader}, pipelineStates);
    auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(graphicsPipeline);

    // create texture image and associated DescriptorSets and binding
    auto texture = vsg::DescriptorImage::create(vsg::Sampler::create(), textureData, 0, 0,
                                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{texture});
    auto bindDescriptorSets = vsg::BindDescriptorSets::create(
        VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->getPipelineLayout(), 0, vsg::DescriptorSets{descriptorSet});

    // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of
    // Descriptors to decorate the whole graph
    auto subgraph = vsg::StateGroup::create();
    subgraph->add(bindGraphicsPipeline);
    subgraph->add(bindDescriptorSets);

    // set up vertex and index arrays
    auto vertices = vsg::vec3Array::create(
        {{-0.818828, -0.445779, -0.361665}, {0.14171, -0.679507, -0.719853},    {-0.137781, -0.957064, 0.255038},
         {-0.732933, -0.187495, 0.653953},  {-0.821265, 0.565674, -0.074407},   {-0.280702, 0.261588, -0.92346},
         {0.821265, -0.565674, 0.074407},   {0.280702, -0.261588, 0.92346},     {-0.14171, 0.679507, 0.719853},
         {0.137781, 0.957064, -0.255038},   {0.732933, 0.187495, -0.653953},    {0.818828, 0.445779, 0.361665},
         {-0.562282, -0.824567, -0.062675}, {-0.398002, -0.661425, -0.635698},  {0.002309, -0.961951, -0.273211},
         {-0.081697, -0.245646, -0.965911}, {-0.646288, -0.108267, -0.755374},  {-0.912102, -0.372229, 0.171802},
         {-0.511795, -0.672753, 0.534293},  {-0.964021, 0.070469, -0.256317},   {-0.913535, 0.222286, 0.340651},
         {-0.647719, 0.486252, -0.586531},  {0.514104, -0.289196, -0.807504},   {0.265817, 0.263965, -0.92718},
         {0.401742, -0.895045, 0.193644},   {0.566025, -0.731903, -0.379385},   {-0.265817, -0.263965, 0.92718},
         {0.084008, -0.716309, 0.692708},   {-0.566025, 0.731903, 0.379385},    {-0.514104, 0.289196, 0.807504},
         {-0.084008, 0.716309, -0.692708},  {-0.401742, 0.895045, -0.193644},   {0.913535, -0.222286, -0.340651},
         {0.647719, -0.486252, 0.586531},   {0.081697, 0.245646, 0.965911},     {-0.002309, 0.961951, 0.273211},
         {0.511795, 0.672753, -0.534293},   {0.964021, -0.070469, 0.256317},    {0.912102, 0.372229, -0.171802},
         {0.646288, 0.108267, 0.755374},    {0.398002, 0.661425, 0.635698},     {0.562282, 0.824567, 0.062675},
         {-0.717878, -0.660305, -0.220565}, {-0.133216, -0.696995, -0.704593},  {-0.0704161, -0.997473, -0.00944551},
         {0.0311932, -0.480879, -0.876232}, {-0.481834, 0.0796943, -0.872631},  {-0.899709, -0.425187, -0.0986872},
         {-0.646989, -0.447143, 0.617631},  {-0.926695, -0.19508, -0.321217},   {-0.901721, 0.409569, 0.138389},
         {-0.482579, 0.388715, -0.784868},  {0.340882, -0.503516, -0.793896},   {0.519134, 0.234662, -0.821848},
         {0.137203, -0.962697, 0.233218},   {0.721092, -0.67446, -0.158523},    {-0.519134, -0.234662, 0.821848},
         {0.18957, -0.508296, 0.840059},    {-0.721092, 0.67446, 0.158523},     {-0.340882, 0.503516, 0.793896},
         {-0.18957, 0.508296, -0.840059},   {-0.137203, 0.962697, -0.233218},   {0.901721, -0.409569, -0.138389},
         {0.482579, -0.388715, 0.784868},   {-0.0311932, 0.480879, 0.876232},   {0.0704161, 0.997473, 0.00944551},
         {0.646989, 0.447143, -0.617631},   {0.927962, -0.330657, 0.171905},    {0.899709, 0.425187, 0.0986872},
         {0.481834, -0.0796943, 0.872631},  {0.133216, 0.696995, 0.704593},     {0.363881, 0.926063, -0.0999877},
         {-0.363882, -0.926063, 0.0999876}, {-0.632488, -0.575506, -0.518412},  {0.0748593, -0.853203, -0.516179},
         {-0.18837, 0.00828639, -0.982063}, {-0.761543, -0.287984, -0.580618},  {-0.855062, -0.290936, 0.429215},
         {-0.337639, -0.847153, 0.410282},  {-0.927962, 0.330657, -0.171905},   {-0.855807, 0.0180843, 0.516979},
         {-0.763554, 0.546774, -0.343545},  {0.64819, -0.052862, -0.759641},    {-0.00773751, 0.273174, -0.961934},
         {0.6357, -0.759258, 0.139329},     {0.36787, -0.733629, -0.571367},    {0.00773751, -0.273174, 0.961934},
         {-0.0279509, -0.869794, 0.492624}, {-0.36787, 0.733629, 0.571367},     {-0.64819, 0.052862, 0.759641},
         {0.0279509, 0.869794, -0.492624},  {-0.6357, 0.759258, -0.139329},     {0.855807, -0.0180843, -0.516979},
         {0.763554, -0.546774, 0.343545},   {0.18837, -0.00828639, 0.982063},   {-0.0748593, 0.853203, 0.516179},
         {0.337639, 0.847153, -0.410282},   {0.926695, 0.19508, 0.321217},      {0.855062, 0.290936, -0.429215},
         {0.761543, 0.287984, 0.580618},    {0.632488, 0.575506, 0.518412},     {0.717878, 0.660305, 0.220565},
         {0.775129, 0.629193, -0.0573709},  {0.748586, 0.549379, -0.371216},    {0.564676, 0.787188, -0.247944},
         {0.504851, 0.781231, 0.367156},    {0.294395, 0.939228, 0.176586},     {0.208028, 0.853459, 0.477841},
         {0.549015, 0.40465, 0.731328},     {0.252193, 0.476875, 0.842015},     {0.382725, 0.186062, 0.904932},
         {0.846588, 0.0198716, 0.531877},   {0.6803, -0.198719, 0.705481},      {0.847342, -0.292685, 0.443111},
         {0.986335, 0.158645, 0.0444327},   {0.98709, -0.153911, -0.044337},    {0.959793, 0.0788291, -0.269412},
         {0.408815, 0.492462, -0.768343},   {0.0955832, 0.515361, -0.851626},   {0.224902, 0.730274, -0.645073},
         {-0.212423, 0.976282, 0.0418314},  {-0.508786, 0.855337, 0.0976503},   {-0.298791, 0.890513, 0.34309},
         {-0.22733, 0.281183, 0.932339},    {-0.410029, 0.0132652, 0.911976},   {-0.0967976, -0.00963034, 0.995257},
         {0.384691, -0.632225, 0.672536},   {0.255374, -0.847139, 0.465983},    {0.551736, -0.726192, 0.410163},
         {0.777852, -0.501647, -0.378546},  {0.567858, -0.536823, -0.623985},   {0.750555, -0.268902, -0.603621},
         {-0.255374, 0.847139, -0.465983},  {-0.384691, 0.632225, -0.672536},   {-0.551736, 0.726192, -0.410163},
         {-0.567858, 0.536823, 0.623985},   {-0.777852, 0.501647, 0.378546},    {-0.750555, 0.268902, 0.603621},
         {-0.0955832, -0.515361, 0.851626}, {-0.408815, -0.492462, 0.768343},   {-0.224902, -0.730274, 0.645073},
         {0.508786, -0.855337, -0.0976503}, {0.212423, -0.976282, -0.0418314},  {0.298791, -0.890513, -0.34309},
         {0.410029, -0.0132652, -0.911976}, {0.22733, -0.281183, -0.932339},    {0.0967976, 0.00963034, -0.995257},
         {-0.6803, 0.198719, -0.705481},    {-0.846588, -0.0198716, -0.531877}, {-0.847342, 0.292685, -0.443111},
         {-0.98709, 0.153911, 0.044337},    {-0.986335, -0.158645, -0.0444327}, {-0.959793, -0.0788291, 0.269412},
         {-0.748586, -0.549379, 0.371216},  {-0.775129, -0.629193, 0.0573709},  {-0.564676, -0.787188, 0.247944},
         {-0.382725, -0.186062, -0.904932}, {-0.252193, -0.476875, -0.842015},  {-0.549015, -0.40465, -0.731328},
         {-0.294395, -0.939228, -0.176586}, {-0.504851, -0.781231, -0.367156},  {-0.208028, -0.853459, -0.477841}});

#if 0
    auto normals =
        vsg::vec3Array::create({{-0.704, -0.704, -0.3736},   {-0.5442, -0.5442, -0.826},  {-0.5913, -0.5913, -0.0591},
                                {-0.2427, -0.2427, -0.1345}, {-0.1399, -0.1399, -0.4956}, {-0.3992, -0.3992, -0.8929},
                                {-0.9921, -0.9921, 0.0406},  {-0.4175, -0.4175, 0.7529},  {0.5305, 0.5305, 0.2595},
                                {0.5419, 0.5419, -0.7576},   {-0.5886, -0.5886, -0.6645}, {-0.8964, -0.8964, 0.4169},
                                {-0.0428, -0.0428, 0.8593},  {0.7926, 0.7926, 0.0514},    {0.4553, 0.4553, -0.8903},
                                {-0.2729, -0.2729, -0.0119}, {-0.2207, -0.2207, 0.782},   {0.5442, 0.5442, 0.826},
                                {0.9647, 0.9647, 0.0592},    {0.4597, 0.4597, -0.4587},   {0.5913, 0.5913, 0.0591},
                                {0.6671, 0.6671, -0.2296},   {0.852, 0.852, -0.2553},     {0.704, 0.704, 0.3736},
                                {0.8735, 0.8735, 0.3467},    {0.8231, 0.8231, 0.5562},    {0.425, 0.425, 0.6434},
                                {0.3623, 0.3623, 0.841},     {0.0644, 0.0644, 0.9298},    {0.1399, 0.1399, 0.4956},
                                {-0.16, -0.16, 0.5703},      {-0.3757, -0.3757, 0.3492},  {0.2427, 0.2427, 0.1345},
                                {0.0284, 0.0284, -0.0914},   {0.1111, 0.1111, -0.3833},   {0.4175, 0.4175, -0.7529},
                                {0.5898, 0.5898, -0.7687},   {0.8099, 0.8099, -0.5495},   {0.9921, 0.9921, -0.0406},
                                {0.9238, 0.9238, 0.1638},    {0.8506, 0.8506, 0.4564},    {0.3992, 0.3992, 0.8929},
                                {0.0967, 0.0967, 0.9636},    {-0.0806, -0.0806, 0.9967},  {-0.5419, -0.5419, 0.7576},
                                {-0.7485, -0.7485, 0.5256},  {-0.6969, -0.6969, 0.3248},  {-0.5305, -0.5305, -0.2595},
                                {-0.4437, -0.4437, -0.5451}, {-0.1465, -0.1465, -0.6309}, {0.8964, 0.8964, -0.4169},
                                {0.7485, 0.7485, -0.5256},   {0.6969, 0.6969, -0.3248},   {0.5886, 0.5886, 0.6645},
                                {0.4437, 0.4437, 0.5451},    {0.1465, 0.1465, 0.6309},    {-0.4553, -0.4553, 0.8903},
                                {-0.5898, -0.5898, 0.7687},  {-0.8099, -0.8099, 0.5495},  {-0.7926, -0.7926, -0.0514},
                                {-0.9238, -0.9238, -0.1638}, {-0.8506, -0.8506, -0.4564}, {0.0428, 0.0428, -0.8593},
                                {-0.0967, -0.0967, -0.9636}, {0.0806, 0.0806, -0.9967},   {0.2207, 0.2207, -0.782},
                                {0.16, 0.16, -0.5703},       {0.3757, 0.3757, -0.3492},   {0.2729, 0.2729, 0.0119},
                                {-0.0284, -0.0284, 0.0914},  {-0.1111, -0.1111, 0.3833},  {-0.4597, -0.4597, 0.4587},
                                {-0.6671, -0.6671, 0.2296},  {-0.852, -0.852, 0.2553},    {-0.0644, -0.0644, -0.9298},
                                {-0.3623, -0.3623, -0.841},  {-0.425, -0.425, -0.6434},   {-0.9647, -0.9647, -0.0592},
                                {-0.8735, -0.8735, -0.3467}, {-0.8231, -0.8231, -0.5562}, {0.7485, 0.7485, -0.5256},
                                {-0.7485, -0.7485, 0.5256},  {0.9921, 0.9921, -0.0407},   {-0.9921, -0.9921, 0.0407}});
#endif

    auto colors = vsg::vec3Array::create(vertices->size(), vsg::vec3(color.r, color.g, color.b));

    auto texcoords = vsg::vec2Array::create(
        {{0.0793455, 0.808896},  {0.282722, 0.877895},  {0.227244, 0.708956}, {0.0398594, 0.636555},
         {0.904004, 0.761853},   {0.880607, 0.937326},  {0.404004, 0.738147}, {0.380607, 0.562674},
         {0.782722, 0.622105},   {0.727244, 0.791044},  {0.539859, 0.863445}, {0.579345, 0.691104},
         {0.154749, 0.759982},   {0.163787, 0.859644},  {0.250382, 0.794043}, {0.1989, 0.958324},
         {0.0264165, 0.886272},  {0.0616677, 0.722521}, {0.146495, 0.660289}, {0.988387, 0.791255},
         {0.962012, 0.694676},   {0.897511, 0.849753},  {0.418448, 0.899591}, {0.624444, 0.938887},
         {0.317147, 0.718985},   {0.354769, 0.811932},  {0.124444, 0.561113}, {0.268581, 0.628209},
         {0.854769, 0.688068},   {0.918448, 0.600409},  {0.768581, 0.871791}, {0.817147, 0.781015},
         {0.462012, 0.805324},   {0.397511, 0.650247},  {0.6989, 0.541676},   {0.750382, 0.705957},
         {0.646495, 0.839711},   {0.488387, 0.708745},  {0.561668, 0.777479}, {0.526417, 0.613728},
         {0.663787, 0.640356},   {0.654748, 0.740018},  {0.118355, 0.785395}, {0.219943, 0.874435},
         {0.238783, 0.751503},   {0.260309, 0.919975},  {0.973912, 0.918794}, {0.0702628, 0.765732},
         {0.0962468, 0.644046},  {0.0330218, 0.802046}, {0.932145, 0.727904}, {0.89208, 0.893635},
         {0.344717, 0.895975},   {0.567567, 0.903528},  {0.272531, 0.712537}, {0.380316, 0.775337},
         {0.0675671, 0.596472},  {0.306814, 0.59126},   {0.880316, 0.724663}, {0.844717, 0.604025},
         {0.806814, 0.90874},    {0.772531, 0.787463},  {0.432145, 0.772096}, {0.39208, 0.606365},
         {0.760309, 0.580025},   {0.738783, 0.748497},  {0.596247, 0.855954}, {0.445521, 0.722504},
         {0.570263, 0.734268},   {0.473912, 0.581206},  {0.719943, 0.625565}, {0.690412, 0.76594},
         {0.190412, 0.73406},    {0.117498, 0.836738},  {0.263928, 0.836323}, {0.993003, 0.96981},
         {0.0575404, 0.848594},  {0.052197, 0.679395},  {0.189639, 0.682715}, {0.945521, 0.777496},
         {0.996637, 0.663528},   {0.901094, 0.805814},  {0.487049, 0.887313}, {0.754507, 0.955945},
         {0.36094, 0.727753},    {0.323975, 0.846793},  {0.254507, 0.544055}, {0.244887, 0.668019},
         {0.823975, 0.653207},   {0.987049, 0.612687},  {0.744887, 0.831981}, {0.86094, 0.772247},
         {0.496637, 0.836472},   {0.401094, 0.694186},  {0.493003, 0.53019},  {0.763928, 0.663677},
         {0.689639, 0.817285},   {0.533022, 0.697954},  {0.552197, 0.820605}, {0.55754, 0.651406},
         {0.617498, 0.663262},   {0.618355, 0.714605},  {0.60852, 0.759136},  {0.600763, 0.81053},
         {0.650964, 0.789878},   {0.65869, 0.690166},   {0.701658, 0.721747}, {0.711949, 0.67071},
         {0.601089, 0.61945},    {0.672578, 0.590685},  {0.572019, 0.569961}, {0.503735, 0.660743},
         {0.454769, 0.625365},   {0.447067, 0.676937},  {0.525382, 0.742926}, {0.475382, 0.757059},
         {0.513042, 0.793415},   {0.639729, 0.889459},  {0.720813, 0.912192}, {0.702452, 0.861587},
         {0.784098, 0.74334},    {0.835405, 0.734434},  {0.801522, 0.694263}, {0.858208, 0.558882},
         {0.994853, 0.567278},   {0.0157823, 0.515506}, {0.336998, 0.632603}, {0.296599, 0.67285},
         {0.353407, 0.682736},   {0.408837, 0.811788},  {0.37947, 0.857244},  {0.445247, 0.853138},
         {0.796599, 0.82715},    {0.836998, 0.867397},  {0.853406, 0.817264}, {0.87947, 0.642756},
         {0.908837, 0.688212},   {0.945247, 0.646862},  {0.220813, 0.587808}, {0.139729, 0.610541},
         {0.202452, 0.638413},   {0.335405, 0.765566},  {0.284098, 0.75666},  {0.301522, 0.805737},
         {0.494853, 0.932722},   {0.358208, 0.941118},  {0.515782, 0.984494}, {0.954768, 0.874635},
         {0.00373508, 0.839257}, {0.947067, 0.823063},  {0.975382, 0.742941}, {0.0253815, 0.757074},
         {0.0130423, 0.706585},  {0.100763, 0.68947},   {0.10852, 0.740864},  {0.150964, 0.710122},
         {0.0720189, 0.930039},  {0.172578, 0.909315},  {0.101089, 0.88055},  {0.201658, 0.778253},
         {0.15869, 0.809834},    {0.211949, 0.82929},   {0.022727, 0.118096}, {0.068182, 0.118096},
         {0.613636, 0.118096},   {0.65909, 0.118096},   {0.681818, 0.078731}, {0.65909, 0.039365},
         {0.636363, 0.078731},   {0.863635, 0.078731},  {0.840908, 0.118096}, {0.886363, 0.118096},
         {0.818181, 0.078731},   {0.772726, 0.078731},  {0.749999, 0.118096}, {0.795454, 0.118096},
         {0.613636, 0.039365},   {0.590909, 0.078731},  {0.636363, 0},        {0.568181, 0.118096},
         {0, 0.157461},          {0.022727, 0.196826},  {0.045454, 0.236191}, {0.068182, 0.275556},
         {0.090909, 0.314921},   {0.65909, 0.354286},   {0.681818, 0.393651}, {0.704545, 0.433017},
         {0.727272, 0.472382},   {0.840908, 0.354286},  {0.863635, 0.393651}, {0.886363, 0.433017},
         {0.90909, 0.472382},    {0.113636, 0.354286},  {0.136363, 0.393651}, {0.159091, 0.433017},
         {0.181818, 0.472382},   {0.295454, 0.354286},  {0.318182, 0.393651}, {0.340909, 0.433017},
         {0.363636, 0.472382},   {0.477272, 0.354286},  {0.5, 0.393651},      {0.522727, 0.433017},
         {0.545454, 0.472382}});

    auto indices = vsg::ushortArray::create(
        {0,   73,  42,  1,   43,  45,  0,   42,  47,  0,   47,  49,  0,   49,  76,  1,   45,  52,  2,   44,  54,  3,
         48,  56,  4,   50,  58,  5,   51,  60,  1,   52,  85,  2,   54,  87,  3,   56,  89,  4,   58,  91,  5,   60,
         83,  6,   62,  67,  7,   63,  69,  8,   64,  70,  9,   65,  71,  10,  66,  98,  38,  102, 68,  38,  103, 102,
         36,  96,  104, 41,  105, 101, 41,  106, 105, 35,  95,  107, 40,  108, 100, 40,  109, 108, 34,  94,  110, 39,
         111, 99,  39,  112, 111, 33,  93,  113, 37,  114, 97,  37,  115, 114, 32,  92,  116, 23,  117, 53,  23,  118,
         117, 30,  90,  119, 31,  120, 61,  31,  121, 120, 28,  88,  122, 29,  123, 59,  29,  124, 123, 26,  86,  125,
         27,  126, 57,  27,  127, 126, 24,  84,  128, 25,  129, 55,  25,  130, 129, 22,  82,  131, 30,  132, 90,  30,
         133, 132, 21,  81,  134, 28,  135, 88,  28,  136, 135, 20,  80,  137, 26,  138, 86,  26,  139, 138, 18,  78,
         140, 24,  141, 84,  24,  142, 141, 14,  74,  143, 22,  144, 82,  22,  145, 144, 15,  75,  146, 16,  147, 46,
         16,  148, 147, 19,  79,  149, 19,  150, 79,  19,  151, 150, 17,  77,  152, 17,  153, 77,  17,  154, 153, 12,
         72,  155, 15,  156, 75,  15,  157, 156, 13,  73,  158, 12,  159, 72,  12,  160, 159, 13,  43,  161, 161, 74,
         14,  161, 43,  74,  43,  1,   74,  159, 161, 14,  159, 160, 161, 160, 13,  161, 72,  44,  2,   72,  159, 44,
         159, 14,  44,  158, 76,  16,  158, 73,  76,  73,  0,   76,  156, 158, 16,  156, 157, 158, 157, 13,  158, 75,
         46,  5,   75,  156, 46,  156, 16,  46,  155, 78,  18,  155, 72,  78,  72,  2,   78,  153, 155, 18,  153, 154,
         155, 154, 12,  155, 77,  48,  3,   77,  153, 48,  153, 18,  48,  152, 80,  20,  152, 77,  80,  77,  3,   80,
         150, 152, 20,  150, 151, 152, 151, 17,  152, 79,  50,  4,   79,  150, 50,  150, 20,  50,  149, 81,  21,  149,
         79,  81,  79,  4,   81,  147, 149, 21,  147, 148, 149, 148, 19,  149, 46,  51,  5,   46,  147, 51,  147, 21,
         51,  146, 83,  23,  146, 75,  83,  75,  5,   83,  144, 146, 23,  144, 145, 146, 145, 15,  146, 82,  53,  10,
         82,  144, 53,  144, 23,  53,  143, 85,  25,  143, 74,  85,  74,  1,   85,  141, 143, 25,  141, 142, 143, 142,
         14,  143, 84,  55,  6,   84,  141, 55,  141, 25,  55,  140, 87,  27,  140, 78,  87,  78,  2,   87,  138, 140,
         27,  138, 139, 140, 139, 18,  140, 86,  57,  7,   86,  138, 57,  138, 27,  57,  137, 89,  29,  137, 80,  89,
         80,  3,   89,  135, 137, 29,  135, 136, 137, 136, 20,  137, 88,  59,  8,   88,  135, 59,  135, 29,  59,  134,
         91,  31,  134, 81,  91,  81,  4,   91,  132, 134, 31,  132, 133, 134, 133, 21,  134, 90,  61,  9,   90,  132,
         61,  132, 31,  61,  131, 92,  32,  131, 82,  92,  82,  10,  92,  129, 131, 32,  129, 130, 131, 130, 22,  131,
         55,  62,  6,   55,  129, 62,  129, 32,  62,  128, 93,  33,  128, 84,  93,  84,  6,   93,  126, 128, 33,  126,
         127, 128, 127, 24,  128, 57,  63,  7,   57,  126, 63,  126, 33,  63,  125, 94,  34,  125, 86,  94,  86,  7,
         94,  123, 125, 34,  123, 124, 125, 124, 26,  125, 59,  64,  8,   59,  123, 64,  123, 34,  64,  122, 95,  35,
         122, 88,  95,  88,  8,   95,  120, 122, 35,  120, 121, 122, 121, 28,  122, 61,  65,  9,   61,  120, 65,  120,
         35,  65,  119, 96,  36,  119, 90,  96,  90,  9,   96,  117, 119, 36,  117, 118, 119, 118, 30,  119, 53,  66,
         10,  53,  117, 66,  117, 36,  66,  116, 98,  38,  116, 92,  98,  92,  10,  98,  114, 116, 38,  114, 115, 116,
         115, 32,  116, 97,  68,  11,  97,  114, 68,  114, 38,  68,  113, 67,  37,  113, 93,  67,  93,  6,   67,  111,
         113, 37,  111, 112, 113, 112, 33,  113, 99,  97,  11,  99,  111, 97,  111, 37,  97,  110, 69,  39,  110, 94,
         69,  94,  7,   69,  108, 110, 39,  108, 109, 110, 109, 34,  110, 100, 99,  11,  100, 108, 99,  108, 39,  99,
         107, 70,  40,  107, 95,  70,  95,  8,   70,  105, 107, 40,  105, 106, 107, 106, 35,  107, 101, 100, 11,  101,
         105, 100, 105, 40,  100, 104, 71,  41,  104, 96,  71,  96,  9,   71,  102, 104, 41,  102, 103, 104, 103, 36,
         104, 68,  101, 11,  68,  102, 101, 102, 41,  101, 98,  103, 38,  98,  66,  103, 66,  36,  103, 71,  106, 41,
         71,  65,  106, 65,  35,  106, 70,  109, 40,  70,  64,  109, 64,  34,  109, 69,  112, 39,  69,  63,  112, 63,
         33,  112, 67,  115, 37,  67,  62,  115, 62,  32,  115, 83,  118, 23,  83,  60,  118, 60,  30,  118, 91,  121,
         31,  91,  58,  121, 58,  28,  121, 89,  124, 29,  89,  56,  124, 56,  26,  124, 87,  127, 27,  87,  54,  127,
         54,  24,  127, 85,  130, 25,  85,  52,  130, 52,  22,  130, 60,  133, 30,  60,  51,  133, 51,  21,  133, 58,
         136, 28,  58,  50,  136, 50,  20,  136, 56,  139, 26,  56,  48,  139, 48,  18,  139, 54,  142, 24,  54,  44,
         142, 44,  14,  142, 52,  145, 22,  52,  45,  145, 45,  15,  145, 76,  148, 16,  76,  49,  148, 49,  19,  148,
         49,  151, 19,  49,  47,  151, 47,  17,  151, 47,  154, 17,  47,  42,  154, 42,  12,  154, 45,  157, 15,  45,
         43,  157, 43,  13,  157, 42,  160, 12,  42,  73,  160, 73,  13,  160});

    // VK_SHARING_MODE_EXCLUSIVE

    // setup geometry
    auto drawCommands = vsg::Commands::create();
    drawCommands->addChild(vsg::BindVertexBuffers::create(
        0, vsg::DataList{vertices, colors, texcoords}));  // shader doesn't support normals yet..
    drawCommands->addChild(vsg::BindIndexBuffer::create(indices));
    drawCommands->addChild(vsg::DrawIndexed::create(indices->size(), 1, 0, 0, 0));

    // add drawCommands to transform
    transform->addChild(drawCommands);
    subgraph->addChild(transform);

    return subgraph;
}

//############################################ Textured Cylinder #######################################################
vsg::ref_ptr<vsg::Node> ChVSGShapeFactory::createCylinderTexturedNode(vsg::vec4 color,
                                                                      std::string texFilePath,
                                                                      vsg::ref_ptr<vsg::MatrixTransform> transform) {
    vsg::ref_ptr<vsg::ShaderStage> vertexShader =
        readVertexShader(GetChronoDataFile("vsg/shaders/vert_PushConstants.spv"));
    vsg::ref_ptr<vsg::ShaderStage> fragmentShader =
        readFragmentShader(GetChronoDataFile("vsg/shaders/frag_PushConstants.spv"));
    if (!vertexShader || !fragmentShader) {
        std::cout << "Could not create shaders." << std::endl;
        return {};
    }

    auto textureData = createRGBATexture(GetChronoDataFile(texFilePath));

    // set up graphics pipeline
    vsg::DescriptorSetLayoutBindings descriptorBindings{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
         nullptr}  // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
    };

    auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);
    vsg::DescriptorSetLayouts descriptorSetLayouts{descriptorSetLayout};

    vsg::PushConstantRanges pushConstantRanges{
        {VK_SHADER_STAGE_VERTEX_BIT, 0, 128}  // projection view, and model matrices, actual push constant calls
                                              // autoaatically provided by the VSG's DispatchTraversal
    };

    auto pipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

    vsg::VertexInputState::Bindings vertexBindingsDescriptions{
        VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // vertex data
        VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // colour data
        VkVertexInputBindingDescription{2, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX}   // tex coord data
    };

    vsg::VertexInputState::Attributes vertexAttributeDescriptions{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},  // vertex data
        VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},  // colour data
        VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0},     // tex coord data
    };

    vsg::GraphicsPipelineStates pipelineStates{
        vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
        vsg::InputAssemblyState::create(),
        vsg::RasterizationState::create(),
        vsg::MultisampleState::create(),
        vsg::ColorBlendState::create(),
        vsg::DepthStencilState::create()};

    auto graphicsPipeline =
        vsg::GraphicsPipeline::create(pipelineLayout, vsg::ShaderStages{vertexShader, fragmentShader}, pipelineStates);
    auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(graphicsPipeline);

    // create texture image and associated DescriptorSets and binding
    auto texture = vsg::DescriptorImage::create(vsg::Sampler::create(), textureData, 0, 0,
                                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{texture});
    auto bindDescriptorSets = vsg::BindDescriptorSets::create(
        VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->getPipelineLayout(), 0, vsg::DescriptorSets{descriptorSet});

    // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of Descriptors
    // to decorate the whole graph
    auto subgraph = vsg::StateGroup::create();
    subgraph->add(bindGraphicsPipeline);
    subgraph->add(bindDescriptorSets);

    // set up vertex and index arrays
    auto vertices = vsg::vec3Array::create({{1, 0, -1},
                                            {1, 0, 1},
                                            {0.980785, -0.19509, -1},
                                            {0.980785, -0.19509, 1},
                                            {0.92388, -0.382683, -1},
                                            {0.92388, -0.382683, 1},
                                            {0.83147, -0.55557, -1},
                                            {0.83147, -0.55557, 1},
                                            {0.707107, -0.707107, -1},
                                            {0.707107, -0.707107, 1},
                                            {0.55557, -0.83147, -1},
                                            {0.55557, -0.83147, 1},
                                            {0.382683, -0.923879, -1},
                                            {0.382683, -0.923879, 1},
                                            {0.19509, -0.980785, -1},
                                            {0.19509, -0.980785, 1},
                                            {0, -1, -1},
                                            {0, -1, 1},
                                            {-0.19509, -0.980785, -1},
                                            {-0.19509, -0.980785, 1},
                                            {-0.382683, -0.923879, -1},
                                            {-0.382683, -0.923879, 1},
                                            {-0.55557, -0.83147, -1},
                                            {-0.55557, -0.83147, 1},
                                            {-0.707107, -0.707107, -1},
                                            {-0.707107, -0.707107, 1},
                                            {-0.83147, -0.55557, -1},
                                            {-0.83147, -0.55557, 1},
                                            {-0.92388, -0.382683, -1},
                                            {-0.92388, -0.382683, 1},
                                            {-0.980785, -0.19509, -1},
                                            {-0.980785, -0.19509, 1},
                                            {-1, 0, -1},
                                            {-1, 0, 1},
                                            {-0.980785, 0.195091, -1},
                                            {-0.980785, 0.195091, 1},
                                            {-0.923879, 0.382684, -1},
                                            {-0.923879, 0.382684, 1},
                                            {-0.831469, 0.555571, -1},
                                            {-0.831469, 0.555571, 1},
                                            {-0.707106, 0.707107, -1},
                                            {-0.707106, 0.707107, 1},
                                            {-0.55557, 0.83147, -1},
                                            {-0.55557, 0.83147, 1},
                                            {-0.382683, 0.92388, -1},
                                            {-0.382683, 0.92388, 1},
                                            {-0.195089, 0.980785, -1},
                                            {-0.195089, 0.980785, 1},
                                            {1e-06, 1, -1},
                                            {1e-06, 1, 1},
                                            {0.195091, 0.980785, -1},
                                            {0.195091, 0.980785, 1},
                                            {0.382684, 0.923879, -1},
                                            {0.382684, 0.923879, 1},
                                            {0.555571, 0.831469, -1},
                                            {0.555571, 0.831469, 1},
                                            {0.707108, 0.707106, -1},
                                            {0.707108, 0.707106, 1},
                                            {0.83147, 0.555569, -1},
                                            {0.83147, 0.555569, 1},
                                            {0.92388, 0.382682, -1},
                                            {0.92388, 0.382682, 1},
                                            {0.980786, 0.195089, -1},
                                            {0.980786, 0.195089, 1}});

    auto colors = vsg::vec3Array::create(vertices->size(), vsg::vec3(color.r, color.g, color.b));

    auto texcoords = vsg::vec2Array::create({{1, 1},
                                             {0.96875, 0.5},
                                             {1, 0.5},
                                             {0.96875, 1},
                                             {0.9375, 0.5},
                                             {0.9375, 1},
                                             {0.90625, 0.5},
                                             {0.90625, 1},
                                             {0.875, 0.5},
                                             {0.875, 1},
                                             {0.84375, 0.5},
                                             {0.84375, 1},
                                             {0.8125, 0.5},
                                             {0.8125, 1},
                                             {0.78125, 0.5},
                                             {0.78125, 1},
                                             {0.75, 0.5},
                                             {0.75, 1},
                                             {0.71875, 0.5},
                                             {0.71875, 1},
                                             {0.6875, 0.5},
                                             {0.6875, 1},
                                             {0.65625, 0.5},
                                             {0.65625, 1},
                                             {0.625, 0.5},
                                             {0.625, 1},
                                             {0.59375, 0.5},
                                             {0.59375, 1},
                                             {0.5625, 0.5},
                                             {0.5625, 1},
                                             {0.53125, 0.5},
                                             {0.53125, 1},
                                             {0.5, 0.5},
                                             {0.5, 1},
                                             {0.46875, 0.5},
                                             {0.46875, 1},
                                             {0.4375, 0.5},
                                             {0.4375, 1},
                                             {0.40625, 0.5},
                                             {0.40625, 1},
                                             {0.375, 0.5},
                                             {0.375, 1},
                                             {0.34375, 0.5},
                                             {0.34375, 1},
                                             {0.3125, 0.5},
                                             {0.3125, 1},
                                             {0.28125, 0.5},
                                             {0.28125, 1},
                                             {0.25, 0.5},
                                             {0.25, 1},
                                             {0.21875, 0.5},
                                             {0.21875, 1},
                                             {0.1875, 0.5},
                                             {0.1875, 1},
                                             {0.15625, 0.5},
                                             {0.15625, 1},
                                             {0.125, 0.5},
                                             {0.125, 1},
                                             {0.09375, 0.5},
                                             {0.09375, 1},
                                             {0.0625, 0.5},
                                             {0.028269, 0.341844},
                                             {0.158156, 0.028269},
                                             {0.471731, 0.158156},
                                             {0.0625, 1},
                                             {0.03125, 0.5},
                                             {0.03125, 1},
                                             {0, 0.5},
                                             {0.796822, 0.014612},
                                             {0.514611, 0.203179},
                                             {0.703179, 0.485389},
                                             {0.341844, 0.471731},
                                             {0.296822, 0.485388},
                                             {0.25, 0.49},
                                             {0.203179, 0.485389},
                                             {0.158156, 0.471731},
                                             {0.116663, 0.449553},
                                             {0.080295, 0.419706},
                                             {0.050447, 0.383337},
                                             {0.014612, 0.296822},
                                             {0.01, 0.25},
                                             {0.014611, 0.203179},
                                             {0.028269, 0.158156},
                                             {0.050447, 0.116663},
                                             {0.080294, 0.080294},
                                             {0.116663, 0.050447},
                                             {0.203178, 0.014612},
                                             {0.25, 0.01},
                                             {0.296822, 0.014612},
                                             {0.341844, 0.028269},
                                             {0.383337, 0.050447},
                                             {0.419706, 0.080294},
                                             {0.449553, 0.116663},
                                             {0.485388, 0.203178},
                                             {0.49, 0.25},
                                             {0.485388, 0.296822},
                                             {0.471731, 0.341844},
                                             {0.449553, 0.383337},
                                             {0.419706, 0.419706},
                                             {0.383337, 0.449553},
                                             {0, 1},
                                             {0.75, 0.49},
                                             {0.796822, 0.485388},
                                             {0.841844, 0.471731},
                                             {0.883337, 0.449553},
                                             {0.919706, 0.419706},
                                             {0.949553, 0.383337},
                                             {0.971731, 0.341844},
                                             {0.985388, 0.296822},
                                             {0.99, 0.25},
                                             {0.985388, 0.203178},
                                             {0.971731, 0.158156},
                                             {0.949553, 0.116663},
                                             {0.919706, 0.080294},
                                             {0.883337, 0.050447},
                                             {0.841844, 0.028269},
                                             {0.75, 0.01},
                                             {0.703178, 0.014612},
                                             {0.658156, 0.028269},
                                             {0.616663, 0.050447},
                                             {0.580294, 0.080294},
                                             {0.550447, 0.116663},
                                             {0.528269, 0.158156},
                                             {0.51, 0.25},
                                             {0.514612, 0.296822},
                                             {0.528269, 0.341844},
                                             {0.550447, 0.383337},
                                             {0.580295, 0.419706},
                                             {0.616663, 0.449553},
                                             {0.658156, 0.471731}});

    auto indices = vsg::ushortArray::create(
        {1,  2,  0,  3,  4,  2,  5,  6,  4,  7,  8,  6,  9,  10, 8,  11, 12, 10, 13, 14, 12, 15, 16, 14, 17, 18, 16,
         19, 20, 18, 21, 22, 20, 23, 24, 22, 25, 26, 24, 27, 28, 26, 29, 30, 28, 31, 32, 30, 33, 34, 32, 35, 36, 34,
         37, 38, 36, 39, 40, 38, 41, 42, 40, 43, 44, 42, 45, 46, 44, 47, 48, 46, 49, 50, 48, 51, 52, 50, 53, 54, 52,
         55, 56, 54, 57, 58, 56, 59, 60, 58, 53, 37, 21, 61, 62, 60, 63, 0,  62, 30, 46, 62, 1,  3,  2,  3,  5,  4,
         5,  7,  6,  7,  9,  8,  9,  11, 10, 11, 13, 12, 13, 15, 14, 15, 17, 16, 17, 19, 18, 19, 21, 20, 21, 23, 22,
         23, 25, 24, 25, 27, 26, 27, 29, 28, 29, 31, 30, 31, 33, 32, 33, 35, 34, 35, 37, 36, 37, 39, 38, 39, 41, 40,
         41, 43, 42, 43, 45, 44, 45, 47, 46, 47, 49, 48, 49, 51, 50, 51, 53, 52, 53, 55, 54, 55, 57, 56, 57, 59, 58,
         59, 61, 60, 5,  3,  1,  1,  63, 5,  63, 61, 5,  61, 59, 57, 57, 55, 61, 55, 53, 61, 53, 51, 49, 49, 47, 53,
         47, 45, 53, 45, 43, 37, 43, 41, 37, 41, 39, 37, 37, 35, 33, 33, 31, 29, 29, 27, 25, 25, 23, 21, 21, 19, 17,
         17, 15, 21, 15, 13, 21, 13, 11, 5,  11, 9,  5,  9,  7,  5,  37, 33, 21, 33, 29, 21, 29, 25, 21, 5,  61, 53,
         53, 45, 37, 21, 13, 5,  5,  53, 21, 61, 63, 62, 63, 1,  0,  62, 0,  2,  2,  4,  6,  6,  8,  10, 10, 12, 6,
         12, 14, 6,  14, 16, 18, 18, 20, 14, 20, 22, 14, 22, 24, 26, 26, 28, 30, 30, 32, 34, 34, 36, 30, 36, 38, 30,
         38, 40, 42, 42, 44, 46, 46, 48, 50, 50, 52, 46, 52, 54, 46, 54, 56, 58, 58, 60, 62, 62, 2,  6,  22, 26, 30,
         38, 42, 46, 54, 58, 46, 58, 62, 46, 62, 6,  14, 14, 22, 30, 30, 38, 46, 62, 14, 30});

    // VK_SHARING_MODE_EXCLUSIVE

    // setup geometry
    auto drawCommands = vsg::Commands::create();
    drawCommands->addChild(vsg::BindVertexBuffers::create(
        0, vsg::DataList{vertices, colors, texcoords}));  // shader doesn't support normals yet..
    drawCommands->addChild(vsg::BindIndexBuffer::create(indices));
    drawCommands->addChild(vsg::DrawIndexed::create(indices->size(), 1, 0, 0, 0));

    // add drawCommands to transform
    transform->addChild(drawCommands);
    subgraph->addChild(transform);

    return subgraph;
}

//++++++++++++++++++++++++++++++++++++++++++++ Phong Shaded Box +++++++++++++++++++++++++++++++++++++++++++++++++
vsg::ref_ptr<vsg::Node> ChVSGShapeFactory::createBoxPhongNode(vsg::vec4 color,
                                                              vsg::ref_ptr<vsg::MatrixTransform> transform) {
    // set up search paths to SPIRV shaders and textures
    vsg::ref_ptr<vsg::ShaderStage> vertexShader = readVertexShader(GetChronoDataFile("vsg/shaders/vert_Phong.spv"));
    vsg::ref_ptr<vsg::ShaderStage> fragmentShader = readFragmentShader(GetChronoDataFile("vsg/shaders/frag_Phong.spv"));
    if (!vertexShader || !fragmentShader) {
        std::cout << "Could not create shaders." << std::endl;
        return {};
    }

    // auto textureData = createRGBATexture(GetChronoDataFile(texFilePath));
    auto textureData =
        vsg::vec4Array2D::create(2, 2, vsg::vec4(0, 0, 0, 0), vsg::Data::Layout{VK_FORMAT_R32G32B32A32_SFLOAT});
    // set up graphics pipeline
    vsg::DescriptorSetLayoutBindings descriptorBindings{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
         nullptr}  // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
    };

    auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);
    vsg::DescriptorSetLayouts descriptorSetLayouts{descriptorSetLayout};

    vsg::PushConstantRanges pushConstantRanges{
        {VK_SHADER_STAGE_VERTEX_BIT, 0, 128}  // projection view, and model matrices, actual push constant calls
                                              // autoatically provided by the VSG's DispatchTraversal
    };

    auto pipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

    vsg::VertexInputState::Bindings vertexBindingsDescriptions{
        VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // vertex data
        VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // normal data
        VkVertexInputBindingDescription{2, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX},  // color data
        VkVertexInputBindingDescription{3, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX}   // texture data
    };

    vsg::VertexInputState::Attributes vertexAttributeDescriptions{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},  // vertex data
        VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},  // normal data
        VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0},  // color data
        VkVertexInputAttributeDescription{3, 3, VK_FORMAT_R32G32_SFLOAT, 0},     // color data
    };

    vsg::GraphicsPipelineStates pipelineStates{
        vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
        vsg::InputAssemblyState::create(),
        vsg::RasterizationState::create(),
        vsg::MultisampleState::create(),
        vsg::ColorBlendState::create(),
        vsg::DepthStencilState::create()};

    auto graphicsPipeline =
        vsg::GraphicsPipeline::create(pipelineLayout, vsg::ShaderStages{vertexShader, fragmentShader}, pipelineStates);
    auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(graphicsPipeline);

    // create texture image and associated DescriptorSets and binding
    auto texture = vsg::DescriptorImage::create(vsg::Sampler::create(), textureData, 0, 0,
                                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{texture});
    auto bindDescriptorSets = vsg::BindDescriptorSets::create(
        VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->getPipelineLayout(), 0, vsg::DescriptorSets{descriptorSet});

    // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of
    // Descriptors to decorate the whole graph
    auto subgraph = vsg::StateGroup::create();
    subgraph->add(bindGraphicsPipeline);
    // subgraph->add(bindDescriptorSets);

    // set up vertex and index arrays
    auto vertices = vsg::vec3Array::create(
        {{1, -1, -1},  {1, 1, -1},  {1, 1, 1},  {1, -1, 1},  {-1, 1, -1}, {-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1},
         {-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}, {1, 1, -1},  {-1, 1, -1},  {-1, 1, 1},  {1, 1, 1},
         {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1},  {1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}});

#if 1
    auto normals = vsg::vec3Array::create({{1, 0, 0},  {1, 0, 0},  {1, 0, 0},  {1, 0, 0},  {-1, 0, 0}, {-1, 0, 0},
                                           {-1, 0, 0}, {-1, 0, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0},
                                           {0, 1, 0},  {0, 1, 0},  {0, 1, 0},  {0, 1, 0},  {0, 0, 1},  {0, 0, 1},
                                           {0, 0, 1},  {0, 0, 1},  {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}});
#endif

    auto colors = vsg::vec3Array::create(vertices->size(), vsg::vec3(color.r, color.g, color.b));

#if 1
    auto texcoords = vsg::vec2Array::create({{0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1},
                                             {0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1},
                                             {0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1}});
#endif

    auto indices = vsg::ushortArray::create({0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
                                             12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23});

    // VK_SHARING_MODE_EXCLUSIVE

    // setup geometry
    auto drawCommands = vsg::Commands::create();
    drawCommands->addChild(vsg::BindVertexBuffers::create(0, vsg::DataList{vertices, normals, colors, texcoords}));
    drawCommands->addChild(vsg::BindIndexBuffer::create(indices));
    drawCommands->addChild(vsg::DrawIndexed::create(indices->size(), 1, 0, 0, 0));

    // add drawCommands to transform
    transform->addChild(drawCommands);
    subgraph->addChild(transform);

    // compile(subgraph);

    return subgraph;
}