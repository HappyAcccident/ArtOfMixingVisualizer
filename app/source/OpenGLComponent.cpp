/*
  ==============================================================================

    OpenGLComponent.cpp
    Created: 24 Nov 2025 12:23:43pm
    Author:  nate

  ==============================================================================
*/

#include "ArtOfMixingVisualizer/OpenGLComponent.h"

//==============================================================================
OpenGLComponent::OpenGLComponent()
{
    juce::OpenGLPixelFormat pixelFormat;
    pixelFormat.depthBufferBits = 24; // 24-bit depth buffer
    boxFile = juce::File("C:/Users/nate/ArtOfMixing/app/resources/box.obj");
    sphereFile = juce::File("C:/Users/nate/ArtOfMixing/app/resources/sphere.obj");
    circleFile = juce::File("C:/Users/nate/ArtOfMixing/app/resources/circle.obj");
    setOpaque(true);
    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(true);
    openGLContext.setPixelFormat(pixelFormat);
    openGLContext.attachTo(*this);
}

OpenGLComponent::~OpenGLComponent()
{
    openGLContext.detach();
}

void OpenGLComponent::paint (juce::Graphics& g)
{

}

void OpenGLComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}

void OpenGLComponent::timerCallback()
{
    frameCounter++;

    float scale = 1.f;
    auto transformOneX = [this, scale](float& refX, float& posX) {posX = scale*refX + cos((float) frameCounter/hz);};
    auto transformOneY = [this, scale](float& refY, float& posY) {posY = scale*refY + abs(sin((float) 10*frameCounter/hz)) - 0.725;};
    auto transformOneZ = [this, scale](float& refZ, float& posZ) {posZ = scale*refZ + 2 * sin((float) frameCounter/hz);};
    juce::Array<std::function<void(float&, float&)>> transformationOne = {transformOneX, transformOneY, transformOneZ};

    auto transformTwoX = [this, scale](float& refX, float& posX) {posX = scale*refX + cos((float) frameCounter/hz + juce::MathConstants<float>::twoPi/5);};
    auto transformTwoY = [this, scale](float& refY, float& posY) {posY = scale*refY + abs(sin((float) 10*frameCounter/hz)) - 0.725;};
    auto transformTwoZ = [this, scale](float& refZ, float& posZ) {posZ = scale*refZ + 2 * sin((float) frameCounter/hz + juce::MathConstants<float>::twoPi/5);};
    juce::Array<std::function<void(float&, float&)>> transformationTwo = {transformTwoX, transformTwoY, transformTwoZ};

    auto transformThreeX = [this, scale](float& refX, float& posX) {posX = scale*refX + cos((float) frameCounter/hz + 2*juce::MathConstants<float>::twoPi/5);};
    auto transformThreeY = [this, scale](float& refY, float& posY) {posY = scale*refY + abs(sin((float) 10*frameCounter/hz)) - 0.725;};
    auto transformThreeZ = [this, scale](float& refZ, float& posZ) {posZ = scale*refZ + 2 * sin((float) frameCounter/hz + 2*juce::MathConstants<float>::twoPi/5);};
    juce::Array<std::function<void(float&, float&)>> transformationThree = {transformThreeX, transformThreeY, transformThreeZ};

    auto transformFourX = [this, scale](float& refX, float& posX) {posX = scale*refX + cos((float) frameCounter/hz + 3*juce::MathConstants<float>::twoPi/5);};
    auto transformFourY = [this, scale](float& refY, float& posY) {posY = scale*refY + abs(sin((float) 10*frameCounter/hz)) - 0.725;};
    auto transformFourZ = [this, scale](float& refZ, float& posZ) {posZ = scale*refZ + 2 * sin((float) frameCounter/hz + 3*juce::MathConstants<float>::twoPi/5);};
    juce::Array<std::function<void(float&, float&)>> transformationFour = {transformFourX, transformFourY, transformFourZ};

    auto transformFiveX = [this, scale](float& refX, float& posX) {posX = scale*refX + cos((float) frameCounter/hz + 4*juce::MathConstants<float>::twoPi/5);};
    auto transformFiveY = [this, scale](float& refY, float& posY) {posY = scale*refY + abs(sin((float) 10*frameCounter/hz)) - 0.725;};
    auto transformFiveZ = [this, scale](float& refZ, float& posZ) {posZ = scale*refZ + 2 * sin((float) frameCounter/hz + 4*juce::MathConstants<float>::twoPi/5);};
    juce::Array<std::function<void(float&, float&)>> transformationFive = {transformFiveX, transformFiveY, transformFiveZ};

    juce::Array<juce::Array<std::function<void(float&, float&)>>> transformations = {transformationOne, 
                                                                                     transformationTwo,
                                                                                     transformationThree,
                                                                                     transformationFour,
                                                                                     transformationFive};

    for (int n = 0; n < 5; n++)
    {
        instruments[n]->updateSphereAndShadow(transformations[n]);
    }

    // std::cout << "[" << renderOrder[0].second << ", "
    //                  << renderOrder[1].second << ", "
    //                  << renderOrder[2].second << ", "
    //                  << renderOrder[3].second << ", "
    //                  << renderOrder[4].second << "]"
    //                  << std::endl;
}

void OpenGLComponent::newOpenGLContextCreated()
{
    using namespace ::juce::gl;
    createShaders();
    startTimerHz(hz);

    boxTex = juce::File("C:/Users/nate/ArtOfMixing/app/resources/boxTex.png");
    juce::Image boxImage = juce::ImageFileFormat::loadFrom(boxTex);
    boxTexture.loadImage(boxImage);
}

void OpenGLComponent::renderOpenGL()
{
    using namespace ::juce::gl;

    jassert (juce::OpenGLHelpers::isContextActive());

    auto desktopScale = (float) openGLContext.getRenderingScale();          // [1]
    juce::OpenGLHelpers::clear(juce::Colours::black);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glViewport (0,
                0,
                juce::roundToInt (desktopScale * (float) getWidth()),
                juce::roundToInt (desktopScale * (float) getHeight()));     // [4]

    shader->use();                                                          // [5]
    
    GLint samplerLoc = glGetUniformLocation(shader->getProgramID(), "gSampler");
    if (samplerLoc >= 0)
        glUniform1i(samplerLoc, 0);

    GLint useTextureLoc = glGetUniformLocation(shader->getProgramID(), "useTexture");
    if (useTextureLoc >= 0)
        glUniform1i(useTextureLoc, 1);
    
    if (uniforms->projectionMatrix.get() != nullptr)                        // [6]
        uniforms->projectionMatrix->setMatrix4 (getProjectionMatrix().mat, 1, false);

    if (uniforms->viewMatrix.get() != nullptr)                              // [7]
        uniforms->viewMatrix->setMatrix4 (getViewMatrix().mat, 1, false);

    boxTexture.bind();
    box->draw(*attributes);
    boxTexture.unbind();
    
    glUniform1i(useTextureLoc, 0);
    std::sort(renderOrder.begin(), renderOrder.end(), [](auto &left, auto &right) {return *left.second < *right.second;});
    for (auto& sphere : renderOrder)
    {
        sphere.first->draw(*attributes);
    }
    for (int n = 0; n < 5; n++)
    {
        shadows[n]->draw(*attributes);
    }

    // Reset the element buffers so child Components draw correctly
    glBindBuffer (GL_ARRAY_BUFFER, 0);                                      // [9]
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
}

void OpenGLComponent::openGLContextClosing()
{

}

//==============================================================================

juce::Matrix3D<float> OpenGLComponent::getProjectionMatrix() const
{
    auto w = 1.0f;                                          // [1]
    auto h = w * getLocalBounds().toFloat().getAspectRatio (false);         // [2]
    return juce::Matrix3D<float>::fromFrustum (-w, w, -h, h, 4.0f, 30.0f);  // [3]
}

juce::Matrix3D<float> OpenGLComponent::getViewMatrix() const
{
    auto viewMatrix = juce::Matrix3D<float>::fromTranslation ({ 0.0f, 0.0f, -10.0f });  // [4]
    auto rotationMatrix = viewMatrix.rotation ({ 0.0f,
                                                 0.0f /* * (float) getFrameCounter() * 0.01f */,
                                                 0.0f /* * (float) getFrameCounter() * 0.01f */});                        // [5]
    return viewMatrix * rotationMatrix;                                           // [6]
}

void OpenGLComponent::createShaders()
{
    vertexShader = R"(
        attribute vec4 position;
        attribute vec4 sourceColour;
        attribute vec2 textureCoordIn;
        uniform mat4 projectionMatrix;
        uniform mat4 viewMatrix;
        varying vec4 destinationColour;
        varying vec2 textureCoordOut;
        void main()
        {
            destinationColour = sourceColour;
            textureCoordOut = textureCoordIn;
            gl_Position = projectionMatrix * viewMatrix * position;
        })";
    fragmentShader =
       #if JUCE_OPENGL_ES
        R"(varying lowp vec4 destinationColour;
           varying lowp vec2 textureCoordOut;)"
       #else
        R"(varying vec4 destinationColour;
           varying vec2 textureCoordOut;
           uniform bool useTexture;
           uniform sampler2D gSampler;)"
       #endif
        R"(
           void main()
           {)"
       #if JUCE_OPENGL_ES
        R"(    lowp vec4 colour = destinationColour;)"
       #else
        R"(    vec4 colour = destinationColour;)"
       #endif
        R"(    
                if (useTexture)
                {
                    gl_FragColor = texture2D(gSampler, textureCoordOut);
                }
                else
                {
                    gl_FragColor = colour;
                }
           })";
    std::unique_ptr<juce::OpenGLShaderProgram> newShader (new juce::OpenGLShaderProgram (openGLContext));   // [1]
    juce::String statusText;
    if (newShader->addVertexShader (juce::OpenGLHelpers::translateVertexShaderToV3 (vertexShader))          // [2]
          && newShader->addFragmentShader (juce::OpenGLHelpers::translateFragmentShaderToV3 (fragmentShader))
          && newShader->link())
    {
        box      .reset();
        box      .reset (new Shape(boxFile, juce::Colours::transparentWhite));

        for (int n = 0; n < 5; n++)
        {
            spheres[n].reset();
            shadows[n].reset();
            instruments[n].reset();
            spheres[n].reset(new Sphere(colors[n]));
            spheres[n].get()->depth = 0.f;
            shadows[n].reset(new Shadow());
            shadows[n].get()->yOrder = n;
            instruments[n].reset(new SphereAndShadow(spheres[n].get(), shadows[n].get()));
            renderOrder[n].first = spheres[n].get();
            renderOrder[n].second = &renderOrder[n].first->depth;
        }
        attributes.reset();
        uniforms  .reset();
        shader.reset (newShader.release());                                                                 // [3]
        shader->use();      
        attributes.reset (new Attributes (*shader));
        uniforms  .reset (new Uniforms (*shader));
        statusText = "GLSL: v" + juce::String (juce::OpenGLShaderProgram::getLanguageVersion(), 2);
    }
    else
    {
        statusText = newShader->getLastError();                                                             // [4]
    }


}

//==============================================================================

OpenGLComponent::Attributes::Attributes (juce::OpenGLShaderProgram& shaderProgram)
{
    position      .reset (createAttribute (shaderProgram, "position"));
    normal        .reset (createAttribute (shaderProgram, "normal"));
    sourceColour  .reset (createAttribute (shaderProgram, "sourceColour"));
    textureCoordIn.reset (createAttribute (shaderProgram, "textureCoordIn"));
}

void OpenGLComponent::Attributes::enable()
{
    using namespace ::juce::gl;
    if (position.get() != nullptr)
    {
        glVertexAttribPointer (position->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), nullptr);
        glEnableVertexAttribArray (position->attributeID);
    }
    if (normal.get() != nullptr)
    {
        glVertexAttribPointer (normal->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), (GLvoid*) (sizeof (float) * 3));
        glEnableVertexAttribArray (normal->attributeID);
    }
    if (sourceColour.get() != nullptr)
    {
        glVertexAttribPointer (sourceColour->attributeID, 4, GL_FLOAT, GL_FALSE, sizeof (Vertex), (GLvoid*) (sizeof (float) * 6));
        glEnableVertexAttribArray (sourceColour->attributeID);
    }
    if (textureCoordIn.get() != nullptr)
    {
        glVertexAttribPointer (textureCoordIn->attributeID, 2, GL_FLOAT, GL_FALSE, sizeof (Vertex), (GLvoid*) (sizeof (float) * 10));
        glEnableVertexAttribArray (textureCoordIn->attributeID);
    }
}

void OpenGLComponent::Attributes::disable()
{
    using namespace ::juce::gl;
    if (position.get() != nullptr)       glDisableVertexAttribArray (position->attributeID);
    if (normal.get() != nullptr)         glDisableVertexAttribArray (normal->attributeID);
    if (sourceColour.get() != nullptr)   glDisableVertexAttribArray (sourceColour->attributeID);
    if (textureCoordIn.get() != nullptr) glDisableVertexAttribArray (textureCoordIn->attributeID);
}

//==============================================================================

OpenGLComponent::Uniforms::Uniforms(juce::OpenGLShaderProgram& shaderProgram)
{
    projectionMatrix.reset (createUniform (shaderProgram, "projectionMatrix"));
    viewMatrix      .reset (createUniform (shaderProgram, "viewMatrix"));
}

//==============================================================================

OpenGLComponent::Shape::Shape(juce::File objFile, juce::Colour color)
{
    if (shapeFile.load(objFile).wasOk())
        for (auto* s : shapeFile.shapes)
        {
            vertexBuffers.add (new VertexBuffer (*s, color));
            referenceVertexBuffers.add (new VertexBuffer(*s, color));
        }
}

void OpenGLComponent::Shape::draw (Attributes& glAttributes)
{
    using namespace ::juce::gl;
    for (auto* vertexBuffer : vertexBuffers)
    {
        vertexBuffer->bind();
        glAttributes.enable();
        glDrawElements (GL_TRIANGLES, vertexBuffer->indices.size(), GL_UNSIGNED_INT, nullptr);
        glAttributes.disable();
        vertexBuffer->unbind();
    }
}

void OpenGLComponent::Shape::updateShape(const std::function<void(OpenGLComponent::Vertex&, OpenGLComponent::Vertex&)>& vertexFunction)
{
    for (int vB = 0; vB < referenceVertexBuffers.size(); vB++)
    {
        for (int v = 0; v < referenceVertexBuffers.getUnchecked(vB)->vertices.size(); v++)
        {
            vertexFunction(referenceVertexBuffers.getUnchecked(vB)->vertices.getReference(v), 
                           vertexBuffers.getUnchecked(vB)->vertices.getReference(v));
        }
    }
}

//==============================================================================

OpenGLComponent::Shape::VertexBuffer::VertexBuffer(WavefrontObjFile::Shape& aShape, juce::Colour color)
{
    using namespace ::juce::gl;
    glGenBuffers (1, &vertexBuffer);
    glGenBuffers (1, &indexBuffer);                                            // [2]
    createVertexListFromMesh (aShape.mesh, vertices, indices, color);     // [3]
}

OpenGLComponent::Shape::VertexBuffer::~VertexBuffer()
{
    using namespace ::juce::gl;
    glDeleteBuffers (1, &vertexBuffer);
    glDeleteBuffers (1, &indexBuffer);
}

void OpenGLComponent::Shape::VertexBuffer::bind()
{
    using namespace ::juce::gl;
    glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData (GL_ARRAY_BUFFER,                                              // [4]
                  static_cast<GLsizeiptr> (static_cast<size_t> (vertices.size()) * sizeof (Vertex)),
                  vertices.getRawDataPointer(), GL_STATIC_DRAW);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER,
                  static_cast<GLsizeiptr> (static_cast<size_t> (indices.size()) * sizeof (juce::uint32)),
                  indices.getRawDataPointer(), GL_STATIC_DRAW);
}

void OpenGLComponent::Shape::VertexBuffer::unbind()
{
    using namespace ::juce::gl;
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
}

//==============================================================================

OpenGLComponent::SphereAndShadow::SphereAndShadow(Sphere* sphere, 
                                                  Shadow* shadow)
                                                  : sphere(sphere), 
                                                    shadow(shadow)
{   
}

void OpenGLComponent::SphereAndShadow::updateSphereAndShadow(const juce::Array<std::function<void(float&, float&)>>& transformations)
{
    auto sphereFunction = [this, &transformations](OpenGLComponent::Vertex& referenceVertex, OpenGLComponent::Vertex& vertex)
    {
        for (int n = 0; n < 3; n++)
            transformations[n](referenceVertex.position[n], vertex.position[n]);
    };

    auto shadowFunction = [this, &transformations](OpenGLComponent::Vertex& referenceVertex, OpenGLComponent::Vertex& vertex)
    {
        transformations[0](referenceVertex.position[0], vertex.position[0]);
        vertex.position[1] = referenceVertex.position[1] + shadow->yOrder * 0.01;
        transformations[2](referenceVertex.position[2], vertex.position[2]);
    };

    float originRef = 0.f;
    float originPos = 0.f;
    transformations[2](originRef, originPos);
    sphere->depth = originPos;
    // std::cout << sphere->depth << std::endl;

    sphere->updateShape(sphereFunction);
    shadow->updateShape(shadowFunction);
}
