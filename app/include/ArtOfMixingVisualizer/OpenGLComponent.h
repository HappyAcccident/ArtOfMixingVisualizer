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
#include <juce_dsp/juce_dsp.h>
#include <ArtOfMixingVisualizer/WavefrontObjFile.h>

static constexpr auto fftOrder = 10;
static constexpr auto fftSize = 1 << fftOrder;

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

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    juce::Matrix3D<float> getProjectionMatrix() const;
    juce::Matrix3D<float> getViewMatrix() const;
    juce::Matrix3D<float> getSphereModelMatrix(int n, float scale) const;
    juce::Matrix3D<float> getShadowModelMatrix(int n , float scale) const;
    void createShaders();

    void pushNextSampleIntoFifo(float sample, int instrument) noexcept;
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

    typedef juce::uint32 Index;

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

        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix, modelMatrix;

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

    struct TextureCoord  { float x, y;    };

    struct Shape
    {
        Shape(juce::File objFile, juce::Colour color);

        void draw (Attributes& glAttributes);
        void updateShape (const std::function<void(OpenGLComponent::Vertex&, OpenGLComponent::Vertex&)>& vertexFunction);

    protected:
        struct VertexBuffer
        {
            explicit VertexBuffer (WavefrontObjFile::Shape& aShape, juce::Colour color);

            ~VertexBuffer();

            void bind();
            void unbind();

            GLuint vertexBuffer, indexBuffer;
            juce::Array<Vertex> vertices;
            juce::Array<Index> indices;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VertexBuffer)
        };

        WavefrontObjFile shapeFile;
        juce::OwnedArray<VertexBuffer> referenceVertexBuffers;
        juce::OwnedArray<VertexBuffer> vertexBuffers;

        static void createVertexListFromMesh (const WavefrontObjFile::Mesh& mesh, juce::Array<Vertex>& vertexList, juce::Array<Index>& indexList, juce::Colour colour)
        {
            auto scale = 1.f;                                                  // [6]
            WavefrontObjFile::TextureCoord defaultTexCoord { 0.5f, 0.5f };
            WavefrontObjFile::Vertex defaultNormal { 0.5f, 0.5f, 0.5f };

            for (auto i = 0; i < mesh.vertices.size(); ++i)                     // [7]
            {
                const auto& v = mesh.vertices.getReference (i);
                const auto& n = i < mesh.normals.size() ? mesh.normals.getReference (i) : defaultNormal;
                const auto& tc = i < mesh.textureCoords.size() ? mesh.textureCoords.getReference (i) : defaultTexCoord;

                vertexList.add ({ { scale * v.x, scale * v.y, scale * v.z, },
                                { scale * n.x, scale * n.y, scale * n.z, },
                                { colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha() },
                                { tc.x, tc.y } });                                  // [8]
            }

            for (auto i = 0; i < mesh.indices.size(); ++i)
            {
                const auto& index = mesh.indices.getReference(i);

                indexList.add ((juce::uint32) index);
            }
        }
    };

    struct Sphere : Shape
    {
        Sphere(juce::Colour color, int id) : Shape(juce::File("C:/Users/nate/ArtOfMixing/app/resources/sphere.obj"), color),
                                             objID(id) 
        {}
        int objID;
        float depth = 0.f;
    };

    struct Shadow : Shape
    {
        Shadow() : Shape(juce::File("C:/Users/nate/ArtOfMixing/app/resources/circle.obj"), 
                         juce::Colours::transparentBlack.withAlpha(0.25f)) {}
        int yOrder = 0;
    };

    juce::File boxFile;
    juce::File sphereFile;
    juce::File circleFile;

    juce::File boxTex;

    juce::OpenGLTexture boxTexture;
    
    juce::String debug;

    juce::String vertexShader;
    juce::String fragmentShader;

    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<Shape> box;
    
    std::array<std::unique_ptr<Sphere>, 5> spheres;
    std::array<std::unique_ptr<Shadow>, 5> shadows;
    std::array<juce::Colour, 5> colors;
    std::array<std::pair<Sphere*, float*>, 5> renderOrder;

    std::unique_ptr<Attributes> attributes;
    std::unique_ptr<Uniforms> uniforms;

    int hz = 60;

    juce::dsp::FFT forwardFFT1;
    juce::dsp::FFT forwardFFT2;
    juce::dsp::FFT forwardFFT3;
    juce::dsp::FFT forwardFFT4;
    juce::dsp::FFT forwardFFT5;
    std::array<juce::dsp::FFT*, 5> forwardFFTs;
    std::array<std::array<float, fftSize>, 5> fifos;
    std::array<std::array<float, fftSize*2>, 5> fftDatas;
    std::array<int, 5> fifoIndexes = {0, 0, 0, 0, 0};
    std::array<bool, 5> nextFFTBlockReadys = {false, false, false, false, false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGLComponent);
};
