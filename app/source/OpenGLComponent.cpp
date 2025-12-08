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
    cubeFile = juce::File("C:/Users/nate/ArtOfMixing/app/resources/cube.obj");
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

    auto transformX = [this](float& refX, float& posX) 
    {
        posX = refX;
        posX += cos((float) frameCounter * 0.01f);
    };
    auto transformY = [this](float& refY, float& posY) 
    {
        posY = refY;
        posY += abs(sin((float) frameCounter * 0.1f)) - 0.725;
    };
    auto transformZ = [this](float& refZ, float& posZ) 
    {
        posZ = refZ;
        posZ += 2 * sin((float) frameCounter * 0.01f);
    };

    juce::Array<std::function<void(float&, float&)>> transformations = {transformX, transformY, transformZ};

    auto transformation = [this](float& ref, float& pos)
    {
        pos = ref * 0.5 * (1.f - sin((float) frameCounter * 0.01f));
    };

    instrumentOne->updateSphereAndShadow(transformations);
}

void OpenGLComponent::newOpenGLContextCreated()
{
    using namespace ::juce::gl;
    createShaders();
    startTimerHz(60);

    cubeTex = juce::File("C:/Users/nate/ArtOfMixing/app/resources/cubeTex.png");
    juce::Image cubeImage = juce::ImageFileFormat::loadFrom(cubeTex);
    cubeTexture.loadImage(cubeImage);
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

    cubeTexture.bind();
    cube->draw(*attributes);
    cubeTexture.unbind();
    
    glUniform1i(useTextureLoc, 0);
    
    instrumentOne->draw(*attributes);

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
        cube      .reset();
        sphere    .reset();
        circle    .reset();
        instrumentOne.reset();
        attributes.reset();
        uniforms  .reset();
        shader.reset (newShader.release());                                                                 // [3]
        shader->use();
        cube      .reset (new Shape(cubeFile, juce::Colours::transparentWhite));
        sphere    .reset (new Shape(sphereFile, juce::Colours::orange.withAlpha(0.65f)));       
        circle    .reset (new Shape(circleFile, juce::Colours::transparentBlack.withAlpha(0.25f)));
        instrumentOne.reset(new SphereAndShadow(sphere.get(), circle.get()));
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

OpenGLComponent::SphereAndShadow::SphereAndShadow(Shape* sphere, 
                                                  Shape* shadow)
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
        transformations[2](referenceVertex.position[2], vertex.position[2]);
    };

    sphere->updateShape(sphereFunction);
    shadow->updateShape(shadowFunction);
}

void OpenGLComponent::SphereAndShadow::draw(OpenGLComponent::Attributes &glAttributes)
{
    sphere->draw(glAttributes);
    shadow->draw(glAttributes);
}
