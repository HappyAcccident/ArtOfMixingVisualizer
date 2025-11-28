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
    teapotFile = juce::File("C:/Users/nate/ArtOfMixing/app/resources/teapot.obj");
    cubeFile = juce::File("C:/Users/nate/ArtOfMixing/app/resources/cube.obj");
    sphereFile = juce::File("C:/Users/nate/ArtOfMixing/app/resources/sphere.obj");
    setOpaque(true);
    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(true);
    openGLContext.setPixelFormat(pixelFormat);
    openGLContext.attachTo(*this);
    startTimerHz(60);
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
}

void OpenGLComponent::newOpenGLContextCreated()
{
    using namespace ::juce::gl;
    createShaders();

    teapotTex = juce::File("C:/Users/nate/ArtOfMixing/app/resources/teapotTex.png");
    cubeTex = juce::File("C:/Users/nate/ArtOfMixing/app/resources/cubeTex.png");

    auto loadTexture = [&](juce::File texFile, GLuint& textureID)
    {
        juce::Image img = juce::ImageFileFormat::loadFrom(texFile);
        if (img.isValid())
        {
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            juce::Image::BitmapData bitmap(img, juce::Image::BitmapData::readOnly);

            glTexImage2D(GL_TEXTURE_2D,
                        0,
                        GL_RGBA,
                        img.getWidth(),
                        img.getHeight(),
                        0,
                        GL_BGRA,
                        GL_UNSIGNED_BYTE,
                        bitmap.data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
            DBG("FAILED TO LOAD TEXTURE!");
        }
    };

    loadTexture(teapotTex, teapotTextureID);
    loadTexture(cubeTex, cubeTextureID);
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

    if (uniforms->projectionMatrix.get() != nullptr)                        // [6]
        uniforms->projectionMatrix->setMatrix4 (getProjectionMatrix().mat, 1, false);

    if (uniforms->viewMatrix.get() != nullptr)                              // [7]
        uniforms->viewMatrix->setMatrix4 (getViewMatrix().mat, 1, false);

    GLint useTextureLoc = glGetUniformLocation(shader->getProgramID(), "useTexture");

    glActiveTexture(GL_TEXTURE0);

    glUniform1i(useTextureLoc, 1);
    glBindTexture(GL_TEXTURE_2D, teapotTextureID);
    // teapot->draw(*attributes);

    glUniform1i(useTextureLoc, 0);
    // cube->draw(*attributes);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUniform1i(useTextureLoc, 0);
    sphere->draw(*attributes);
    glBindTexture(GL_TEXTURE_2D, 0);


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
    auto w = 1.0f / (0.5f + 0.1f);                                          // [1]
    auto h = w * getLocalBounds().toFloat().getAspectRatio (false);         // [2]
    return juce::Matrix3D<float>::fromFrustum (-w, w, -h, h, 4.0f, 30.0f);  // [3]
}

juce::Matrix3D<float> OpenGLComponent::getViewMatrix() const
{
    auto viewMatrix = juce::Matrix3D<float>::fromTranslation ({ 0.0f, 0.0f, -10.0f });  // [4]
    auto rotationMatrix = viewMatrix.rotation ({ 0.0f,
                                                 0.0f /* * std::sin ((float) getFrameCounter() * 0.01f) */,
                                                 0.0f });                        // [5]
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
                    gl_FragColor = vec4(0.95, 0.57, 0.03, 0.7);
                }
           })";
    std::unique_ptr<juce::OpenGLShaderProgram> newShader (new juce::OpenGLShaderProgram (openGLContext));   // [1]
    juce::String statusText;
    if (newShader->addVertexShader (juce::OpenGLHelpers::translateVertexShaderToV3 (vertexShader))          // [2]
          && newShader->addFragmentShader (juce::OpenGLHelpers::translateFragmentShaderToV3 (fragmentShader))
          && newShader->link())
    {
        teapot    .reset();
        cube      .reset();
        sphere    .reset();
        attributes.reset();
        uniforms  .reset();
        shader.reset (newShader.release());                                                                 // [3]
        shader->use();
        teapot    .reset (new Shape(teapotFile));
        cube      .reset (new Shape(cubeFile));
        sphere    .reset (new Sphere(10.f));
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

OpenGLComponent::Shape::Shape(juce::File objFile)
{
    if (shapeFile.load(objFile).wasOk())
        for (auto* s : shapeFile.shapes)
            vertexBuffers.add (new VertexBuffer (*s));
}

void OpenGLComponent::Shape::draw (Attributes& glAttributes)
{
    using namespace ::juce::gl;
    for (auto* vertexBuffer : vertexBuffers)
    {
        vertexBuffer->bind();
        glAttributes.enable();
        glDrawElements (GL_TRIANGLES, vertexBuffer->numIndices, GL_UNSIGNED_INT, nullptr);
        glAttributes.disable();
    }
}

//==============================================================================

OpenGLComponent::Shape::VertexBuffer::VertexBuffer(WavefrontObjFile::Shape& aShape)
{
    using namespace ::juce::gl;
    numIndices = aShape.mesh.indices.size();                                    // [1]
    glGenBuffers (1, &vertexBuffer);                                            // [2]
    glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
    juce::Array<Vertex> vertices;
    createVertexListFromMesh (aShape.mesh, vertices, juce::Colours::green);     // [3]
    glBufferData (GL_ARRAY_BUFFER,                                              // [4]
                  static_cast<GLsizeiptr> (static_cast<size_t> (vertices.size()) * sizeof (Vertex)),
                  vertices.getRawDataPointer(), GL_STATIC_DRAW);
    glGenBuffers (1, &indexBuffer);                                             // [5]
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER,
                  static_cast<GLsizeiptr> (static_cast<size_t> (numIndices) * sizeof (juce::uint32)),
                  aShape.mesh.indices.getRawDataPointer(), GL_STATIC_DRAW);
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
}

//==============================================================================

OpenGLComponent::Sphere::Sphere(float radius) : Shape(juce::File("C:/Users/nate/ArtOfMixing/app/resources/sphere.obj"))
{
    
}