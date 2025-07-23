#include "Skybox.h"

#include <rlgl.h>
#include <raymath.h>

// https://www.raylib.com/examples/models/loader.html?name=models_skybox
static TextureCubemap GenTextureCubemap(Shader shader, Texture2D panorama, int size, int format)
{
    TextureCubemap cubemap = { 0 };

    rlDisableBackfaceCulling();     // Disable backface culling to render inside the cube

    // STEP 1: Setup framebuffer
    //------------------------------------------------------------------------------------------
    unsigned int rbo = rlLoadTextureDepth(size, size, true);
    cubemap.id = rlLoadTextureCubemap(0, size, format, 1);

    unsigned int fbo = rlLoadFramebuffer();
    rlFramebufferAttach(fbo, rbo, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
    rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X, 0);

    // Check if framebuffer is complete with attachments (valid)
    if (rlFramebufferComplete(fbo)) TraceLog(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", fbo);
    //------------------------------------------------------------------------------------------

    // STEP 2: Draw to framebuffer
    //------------------------------------------------------------------------------------------
    // NOTE: Shader is used to convert HDR equirectangular environment map to cubemap equivalent (6 faces)
    rlEnableShader(shader.id);

    // Define projection matrix and send it to shader
    Matrix matFboProjection = MatrixPerspective(90.0 * DEG2RAD, 1.0, rlGetCullDistanceNear(), rlGetCullDistanceFar());
    rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_PROJECTION], matFboProjection);

    // Define view matrix for every side of the cubemap
    Matrix fboViews[6] = {
        MatrixLookAt( { 0.0f, 0.0f, 0.0f },  { 1.0f,  0.0f,  0.0f },  { 0.0f, -1.0f,  0.0f }),
        MatrixLookAt( { 0.0f, 0.0f, 0.0f },  { -1.0f,  0.0f,  0.0f },  { 0.0f, -1.0f,  0.0f }),
        MatrixLookAt( { 0.0f, 0.0f, 0.0f },  { 0.0f,  1.0f,  0.0f },  { 0.0f,  0.0f,  1.0f }),
        MatrixLookAt( { 0.0f, 0.0f, 0.0f },  { 0.0f, -1.0f,  0.0f },  { 0.0f,  0.0f, -1.0f }),
        MatrixLookAt( { 0.0f, 0.0f, 0.0f },  { 0.0f,  0.0f,  1.0f },  { 0.0f, -1.0f,  0.0f }),
        MatrixLookAt( { 0.0f, 0.0f, 0.0f },  { 0.0f,  0.0f, -1.0f },  { 0.0f, -1.0f,  0.0f })
    };

    rlViewport(0, 0, size, size);   // Set viewport to current fbo dimensions

    // Activate and enable texture for drawing to cubemap faces
    rlActiveTextureSlot(0);
    rlEnableTexture(panorama.id);

    for (int i = 0; i < 6; i++)
    {
        // Set the view matrix for the current cube face
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_VIEW], fboViews[i]);

        // Select the current cubemap face attachment for the fbo
        // WARNING: This function by default enables->attach->disables fbo!!!
        rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X + i, 0);
        rlEnableFramebuffer(fbo);

        // Load and draw a cube, it uses the current enabled texture
        rlClearScreenBuffers();
        rlLoadDrawCube();

        // ALTERNATIVE: Try to use internal batch system to draw the cube instead of rlLoadDrawCube
        // for some reason this method does not work, maybe due to cube triangles definition? normals pointing out?
        // TODO: Investigate this issue...
        //rlSetTexture(panorama.id); // WARNING: It must be called after enabling current framebuffer if using internal batch system!
        //rlClearScreenBuffers();
        //DrawCubeV(Vector3Zero(), Vector3One(), WHITE);
        //rlDrawRenderBatchActive();
    }
    //------------------------------------------------------------------------------------------

    // STEP 3: Unload framebuffer and reset state
    //------------------------------------------------------------------------------------------
    rlDisableShader();          // Unbind shader
    rlDisableTexture();         // Unbind texture
    rlDisableFramebuffer();     // Unbind framebuffer
    rlUnloadFramebuffer(fbo);   // Unload framebuffer (and automatically attached depth texture/renderbuffer)

    // Reset viewport dimensions to default
    rlViewport(0, 0, rlGetFramebufferWidth(), rlGetFramebufferHeight());
    rlEnableBackfaceCulling();
    //------------------------------------------------------------------------------------------

    cubemap.width = size;
    cubemap.height = size;
    cubemap.mipmaps = 1;
    cubemap.format = format;

    return cubemap;
}

namespace SatHunter {
    
void Skybox::Load(std::string hdrFilename)
{
    m_Skybox = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));

    // Load skybox shader and set required locations
    m_Skybox.materials[0].shader = LoadShader("res/skybox.vs", "res/skybox.fs");

    Shader skyboxShader = m_Skybox.materials[0].shader;

    int cubemapValue = MATERIAL_MAP_CUBEMAP;
    SetShaderValue(skyboxShader, GetShaderLocation(skyboxShader, "environmentMap"), &cubemapValue, SHADER_UNIFORM_INT);

    int gammaValue = 0;
    SetShaderValue(skyboxShader, GetShaderLocation(skyboxShader, "doGamma"), &gammaValue, SHADER_UNIFORM_INT);

    int flippedValue = 1;
    SetShaderValue(skyboxShader, GetShaderLocation(skyboxShader, "vflipped"), &flippedValue, SHADER_UNIFORM_INT);

    // Load cubemap shader and setup required shader locations
    m_CubemapShader = LoadShader("res/cubemap.vs", "res/cubemap.fs");

    int eqMap = 0;
    SetShaderValue(m_CubemapShader, GetShaderLocation(m_CubemapShader, "equirectangularMap"), &eqMap, SHADER_UNIFORM_INT);

    Texture panorama = LoadTexture(hdrFilename.c_str());

    m_Skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture =
        GenTextureCubemap(m_CubemapShader, panorama, 1024, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    UnloadTexture(panorama);
}

void Skybox::Draw()
{
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
        DrawModel(m_Skybox, { 0, 0, 0 }, 10.0f, WHITE);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

}

