#version 330 core

layout (location = 0) out vec3 fColor;

uniform sampler2D u_diffuseMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_vertexMap;
uniform sampler2D u_depthMap;

uniform mat4 viewMatrix;
uniform int u_numLights;
uniform float u_zNear;
uniform float u_zFar;

uniform vec2 u_screenSize;

const int g_maxLights = 50;

in vec2 vTexCoord;

layout (std140) uniform LightPos
{
  vec4 u_lightPos[g_maxLights];
};

const int g_sampleCount = 16;
const int g_radius = 8;

const vec2 g_poissonSamples[] = vec2[](
                                vec2( -0.94201624,  -0.39906216 ),
                                vec2(  0.94558609,  -0.76890725 ),
                                vec2( -0.094184101, -0.92938870 ),
                                vec2(  0.34495938,   0.29387760 ),
                                vec2( -0.91588581,   0.45771432 ),
                                vec2( -0.81544232,  -0.87912464 ),
                                vec2( -0.38277543,   0.27676845 ),
                                vec2(  0.97484398,   0.75648379 ),
                                vec2(  0.44323325,  -0.97511554 ),
                                vec2(  0.53742981,  -0.47373420 ),
                                vec2( -0.26496911,  -0.41893023 ),
                                vec2(  0.79197514,   0.19090188 ),
                                vec2( -0.24188840,   0.99706507 ),
                                vec2( -0.81409955,   0.91437590 ),
                                vec2(  0.19984126,   0.78641367 ),
                                vec2(  0.14383161,  -0.14100790 )
                               );


float linearizeDepth(vec2 texcoord)
{
  float z = 2.0 * u_zNear/ (u_zFar + u_zNear - texture2D(u_depthMap, texcoord).x * (u_zFar - u_zNear));
  return z;
}

void AlchemyAO()
{
}
                               
void main()
{
  vec3 normal_camera = (texture(u_normalMap, vTexCoord)).xyz;
  
  if(length(normal_camera) == 0)
    fColor = vec3(0.8);
  else {
    float occ_factor = 0;
    vec3 vertex_camera = (texture(u_vertexMap, vTexCoord)).xyz;
        
    /*for(int i = 0; i < g_sampleCount; i++) {
      //float depth = texture(u_depthMap, vTexCoord).r;
      vec2 sampleTexCoord = vTexCoord + (g_poissonSamples[i] * g_radius / u_screenSize.x);
      vec3 samplePos = texture(u_vertexMap, sampleTexCoord).xyz;
      
      vec3 V = samplePos - vertex_camera;
      float d = length(V);
      V = normalize(V);
      
      occ_factor += max(0.0, dot(normal_camera, V)) / (1.0 + d);
    }*/
    
    vec3 t = normalize(texture(u_vertexMap, vec2(vTexCoord.x + 1 / u_screenSize.x, vTexCoord.y)).xyz - vertex_camera);
    vec3 b = normalize(texture(u_vertexMap, vec2(vTexCoord.x, vTexCoord.y + 1 / u_screenSize.y)).xyz - vertex_camera);
    mat3 tbn = mat3(t, b, normal_camera);
    
    for(int i = 0; i < g_sampleCount; i++) {
      vec2 sampleTexCoord = vTexCoord + (g_poissonSamples[i] * g_radius / u_screenSize.x);
      vec3 samplePos = tbn * texture(u_vertexMap, sampleTexCoord).xyz;
      
      vec3 V = (samplePos - vertex_camera);
      float d = length(V);
      V = normalize(V);
      
      occ_factor += max(0.0, dot(normal_camera, V)) / (1.0 + d);
    }
    
    fColor = vec3(1.0 - (1 * occ_factor / g_sampleCount));
  }
 }
