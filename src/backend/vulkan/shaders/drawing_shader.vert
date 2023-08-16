#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTexCoord;

layout(push_constant) uniform uPushConstant
{
	vec2 xyScale;
	vec2 xyOffset;
} pc;

void main() {
    gl_Position = vec4(inPosition * pc.xyScale + pc.xyOffset, 0, 1);
    outColor = inColor;
    outTexCoord = inTexCoord;
}
