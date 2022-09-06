#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inVertColor;
layout(push_constant) uniform uPushConstant
{
	vec2 xyScale;
	vec2 xyOffset;
	vec2 uvScale;
	vec2 uvOffset;
} pc;

layout(location = 0) out vec2 fragTexCoord;

void main() {
	gl_Position = vec4(inPosition * pc.xyScale + pc.xyOffset, 0, 1);
	fragTexCoord = vec2(inTexCoord * pc.uvScale + pc.uvOffset);
}
