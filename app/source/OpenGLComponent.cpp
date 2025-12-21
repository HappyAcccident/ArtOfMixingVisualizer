/*
  ==============================================================================

    OpenGLComponent.cpp
    Created: 24 Nov 2025 12:23:43pm
    Author:  nate

  ==============================================================================
*/

#include "ArtOfMixingVisualizer/OpenGLComponent.h"
#include <algorithm>
#include <numeric>

//==============================================================================
OpenGLComponent::OpenGLComponent() : forwardFFT(fftOrder)
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
    for (int n = 0; n < 5; n++)
    {
        if (nextFFTBlockReadys[n])
        {
            forwardFFT.performFrequencyOnlyForwardTransform(fftDatas[n].data());
            nextFFTBlockReadys[n] = false;
            analyzeFFT(fftDatas[n].data(), fftSize, 44100, n);
        }
    }
    std::cout << "[" << freqRanges[0].first << ", " << freqRanges[0].second << "]" << std::endl;
}

void OpenGLComponent::analyzeFFT(const float* fftData, int fftSize, float sampleRate, int instrument)
{
    std::vector<float> power(fftSize/2);
    float totalPower = 0.0f;
    for (int i = 0; i < fftSize/2; ++i)
    {
        power[i] = fftData[i] * fftData[i];
        totalPower += power[i];
    }

    if (totalPower <= 0.0f)
    {
        meanFreqs[instrument] = 0.f;
        freqRanges[instrument].first, freqRanges[instrument].second = 0.f;
    }

    std::vector<float> cdf(fftSize/2);
    float running = 0.0f;
    for (int i = 0; i < fftSize/2; ++i)
    {
        running += power[i] / totalPower;
        cdf[i] = running;
    }

    float lowerFreq = 0.0f;
    float upperFreq = 0.0f;
    for (int i = 0; i < fftSize/2; ++i)
    {
        float freq = i * sampleRate / fftSize;
        if (cdf[i] >= 0.25f && lowerFreq == 0.0f)
            lowerFreq = freq;
        if (cdf[i] >= 0.75f)
        {
            upperFreq = freq;
            break;
        }
    }

    float weightedSum = 0.0f;
    for (int i = 0; i < fftSize/2; ++i)
    {
        float freq = i * sampleRate / fftSize;
        weightedSum += freq * power[i];
    }
    float meanFreq = weightedSum / totalPower;

    meanFreqs[instrument] = meanFreq;
    freqRanges[instrument].first = lowerFreq;
    freqRanges[instrument].second = upperFreq;
}


//==============================================================================

void OpenGLComponent::pushNextSampleIntoFifos(float leftSample, float rightSample, int instrument) noexcept
{
    // if the fifo contains enough data, set a flag to say
    // that the next line should now be rendered..
    if (fifoIndexes[instrument] == fftSize) // [8]
    {
        if (!nextFFTBlockReadys[instrument]) // [9]
        {
            std::fill (fftDatas[instrument].begin(), fftDatas[instrument].end(), 0.0f);
            std::copy (monoFifos[instrument].begin(), monoFifos[instrument].end(), fftDatas[instrument].begin());
            nextFFTBlockReadys[instrument] = true;
        }
        fifoIndexes[instrument] = 0;
    }
    auto idx = (size_t) fifoIndexes[instrument];
    leftFifos[instrument][idx] = leftSample; // [9]
    rightFifos[instrument][idx] = rightSample;
    monoFifos[instrument][idx] = (leftSample + rightSample)/2;
    fifoIndexes[instrument]++;
}

float mapFrequencyToInterval(float freqHz)
{
    freqHz = juce::jlimit(20.0f, 20000.0f, freqHz);
    constexpr float minFreq = 20.0f;
    constexpr float maxFreq = 20000.0f;
    float logMin = std::log(minFreq);
    float logMax = std::log(maxFreq);
    float logFreq = std::log(freqHz);
    float t = (logFreq - logMin) / (logMax - logMin);
    return juce::jmap(t, 0.0f, 1.0f, -1.5f, 1.5f);
}

float mapIntervalToRange(float lowerFreqHz, float upperFreqHz)
{
    lowerFreqHz = juce::jlimit(20.0f, 20000.0f, lowerFreqHz);
    upperFreqHz = juce::jlimit(20.0f, 20000.0f, upperFreqHz);
    constexpr float minFreq = 20.0f;
    constexpr float maxFreq = 20000.0f;
    float logMin = std::log(minFreq);
    float logMax = std::log(maxFreq);
    float lowerLogFreq = std::log(lowerFreqHz);
    float upperLogFreq = std::log(upperFreqHz);
    float lowerT = (lowerFreqHz - logMin) / (logMax - logMin);
    float upperT = (upperFreqHz - logMin) / (logMax - logMin);
    return juce::jmap(upperT - lowerT, 0.0f, 1.0f, -1.5f, 1.5f);
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
    
    auto identity = juce::Matrix3D<float>::Matrix3D();
    if (uniforms->modelMatrix.get() != nullptr)
        uniforms->modelMatrix->setMatrix4 (identity.mat, 1, false);

    boxTexture.bind();
    box->draw(*attributes);
    boxTexture.unbind();
    
    glUniform1i(useTextureLoc, 0);

    float scale = 0.5f;
    for (int n = 0; n < 5; n++)
    {
        spheres[n].get()->depth = getSphereModelMatrix(n, scale).mat[14];
    }
    std::sort(renderOrder.begin(), renderOrder.end(), [](auto &left, auto &right) {return *left.second < *right.second;});
    for (auto& sphere : renderOrder)
    {
        if (uniforms->modelMatrix.get() != nullptr)
            uniforms->modelMatrix->setMatrix4 (getSphereModelMatrix(sphere.first->objID, scale).mat, 1, false);
        sphere.first->draw(*attributes);
    }
    for (int n = 0; n < 5; n++)
    {
        if (uniforms->modelMatrix.get() != nullptr)
            uniforms->modelMatrix->setMatrix4 (getShadowModelMatrix(n, scale).mat, 1 , false);
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
                                                 0.0f /* * (float) frameCounter * 0.01f */,
                                                 0.0f /* * (float) frameCounter * 0.01f */});                        // [5]
    return viewMatrix * rotationMatrix;                                           // [6]
}

juce::Matrix3D<float> OpenGLComponent::getSphereModelMatrix(int n, float scale) const
{
    auto scaleMatrix = juce::Matrix3D<float>::Matrix3D({scale, 0.f,   0.f,   0.f,      
                                                        0.f,   scale, 0.f,   0.f,
                                                        0.f,   0.f,   scale, 0.f,
                                                        0.f,   0.f,   0.f,   1.f});
    auto translationMatrix = juce::Matrix3D<float>::fromTranslation({0.f,
                                                                     mapFrequencyToInterval(meanFreqs[n]),
                                                                     0.f});
    return translationMatrix * scaleMatrix;
}

juce::Matrix3D<float> OpenGLComponent::getShadowModelMatrix(int n, float scale) const
{
    auto scaleMatrix = juce::Matrix3D<float>::Matrix3D({scale, 0.f,   0.f,   0.f,      
                                                        0.f,   1.f,   0.f,   0.f,
                                                        0.f,   0.f,   scale, 0.f,
                                                        0.f,   0.f,   0.f,   1.f});
    auto translationMatrix = juce::Matrix3D<float>::fromTranslation({0.f,
                                                                     0.005f*n,
                                                                     0.f});
    return translationMatrix * scaleMatrix;
}

void OpenGLComponent::createShaders()
{
    vertexShader = R"(
        attribute vec4 position;
        attribute vec4 sourceColour;
        attribute vec2 textureCoordIn;
        uniform mat4 projectionMatrix;
        uniform mat4 viewMatrix;
        uniform mat4 modelMatrix;
        varying vec4 destinationColour;
        varying vec2 textureCoordOut;
        void main()
        {
            destinationColour = sourceColour;
            textureCoordOut = textureCoordIn;
            gl_Position = projectionMatrix * viewMatrix * modelMatrix * position;
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
            colors[n] = juce::Colour::fromHSV(float(n)/5, 0.85f, 0.85f, 0.5f);
            spheres[n].reset();
            shadows[n].reset();
            spheres[n].reset(new Sphere(colors[n], n));
            spheres[n].get();
            shadows[n].reset(new Shadow());
            shadows[n].get();
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
    modelMatrix     .reset (createUniform (shaderProgram, "modelMatrix"));
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
