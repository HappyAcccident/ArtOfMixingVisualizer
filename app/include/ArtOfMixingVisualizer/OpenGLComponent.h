/*
  ==============================================================================

    OpenGLComponent.h
    Created: 24 Nov 2025 12:23:43pm
    Author:  nate

  ==============================================================================
*/

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_opengl/juce_opengl.h>
#include <ArtOfMixingVisualizer/WavefrontObjFile.h>

//==============================================================================
/*
*/
class OpenGLComponent  : public juce::Component,
                         public juce::OpenGLRenderer,
                         public juce::Timer
{
public:
    OpenGLComponent();
    ~OpenGLComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    int getFrameCounter() const {return frameCounter;};

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    juce::Matrix3D<float> getProjectionMatrix() const;
    juce::Matrix3D<float> getViewMatrix() const;
    void createShaders();

private:
    int frameCounter = 0;

    juce::OpenGLContext openGLContext;

    struct Vertex
    {
        float position[3];
        float normal[3];
        float colour[4];
        float texCoord[2];
    };

    struct Attributes
    {
        explicit Attributes (juce::OpenGLShaderProgram& shaderProgram);

        void enable();

        void disable();

        std::unique_ptr<juce::OpenGLShaderProgram::Attribute> position, normal, sourceColour, textureCoordIn;

    private:
        static juce::OpenGLShaderProgram::Attribute* createAttribute (juce::OpenGLShaderProgram& shader,
                                                                      const juce::String& attributeName)
        {
            using namespace ::juce::gl;

            if (glGetAttribLocation (shader.getProgramID(), attributeName.toRawUTF8()) < 0)
                return nullptr;

            return new juce::OpenGLShaderProgram::Attribute (shader, attributeName.toRawUTF8());
        }
    };

    struct Uniforms
    {
        explicit Uniforms (juce::OpenGLShaderProgram& shaderProgram);

        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix;

    private:
        static juce::OpenGLShaderProgram::Uniform* createUniform (juce::OpenGLShaderProgram& shaderProgram,
                                                                  const juce::String& uniformName)
        {
            using namespace ::juce::gl;

            if (glGetUniformLocation (shaderProgram.getProgramID(), uniformName.toRawUTF8()) < 0)
                return nullptr;

            return new juce::OpenGLShaderProgram::Uniform (shaderProgram, uniformName.toRawUTF8());
        }
    };

    struct Shape
    {
        Shape(juce::File objFile);

        void draw(Attributes& glAttributes);

        void bind();

    public:
        struct VertexBuffer
        {
            explicit VertexBuffer (WavefrontObjFile::Shape& aShape);

            ~VertexBuffer();

            void bind();

            GLuint vertexBuffer, indexBuffer;
            juce::Array<Vertex> vertices;
            WavefrontObjFile::Index* indicesPtr;
            int numIndices;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VertexBuffer)
        };

        WavefrontObjFile shapeFile;
        juce::OwnedArray<VertexBuffer> vertexBuffers;

        static void createVertexListFromMesh (const WavefrontObjFile::Mesh& mesh, juce::Array<Vertex>& list, juce::Colour colour)
        {
            auto scale = 0.2f;                                                  // [6]
            WavefrontObjFile::TextureCoord defaultTexCoord { 0.5f, 0.5f };
            WavefrontObjFile::Vertex defaultNormal { 0.5f, 0.5f, 0.5f };

            for (auto i = 0; i < mesh.vertices.size(); ++i)                     // [7]
            {
                const auto& v = mesh.vertices.getReference (i);
                const auto& n = i < mesh.normals.size() ? mesh.normals.getReference (i) : defaultNormal;
                const auto& tc = i < mesh.textureCoords.size() ? mesh.textureCoords.getReference (i) : defaultTexCoord;

                list.add ({ { scale * v.x, scale * v.y, scale * v.z, },
                            { scale * n.x, scale * n.y, scale * n.z, },
                            { colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha() },
                            { tc.x, tc.y } });                                  // [8]
            }
        }
    };

    struct Sphere : public Shape
    {
        Sphere(float radius);
        
    private:
        float radius;
    };

    juce::File teapotFile = juce::File("C:/Users/nate/ArtOfMixing/app/resources/teapot.obj");
    juce::File cubeFile = juce::File("C:/Users/nate/ArtOfMixing/app/resources/cube.obj");
    juce::File sphereFile = juce::File("C:/Users/nate/ArtOfMixing/app/resources/sphere.obj");

    juce::File teapotTex;
    juce::File cubeTex;
    
    juce::String debug;

    juce::String vertexShader;
    juce::String fragmentShader;

    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<Shape> teapot;
    std::unique_ptr<Shape> cube;
    std::unique_ptr<Shape> sphere;
    std::unique_ptr<Attributes> attributes;
    std::unique_ptr<Uniforms> uniforms;
    GLuint teapotTextureID = 0;
    GLuint cubeTextureID = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGLComponent)
};
