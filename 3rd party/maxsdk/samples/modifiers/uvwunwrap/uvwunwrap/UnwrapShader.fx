//**************************************************************************/
// Copyright 2015 Autodesk, Inc.
// All rights reserved.
// 
// This computer source code and related instructions and comments are the 
// unpublished confidential and proprietary information of Autodesk, Inc. 
// and are protected under Federal copyright and state trade secret law.
// They may not be disclosed to, copied or used by any third party without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
matrix worldViewProjection : WorldViewProjection;
float2 ViewSize : ViewportPixelSize;
StructuredBuffer<uint> gSelectionAttribute;
float3 gSelectedEdgeColor = float3(1.0,0.0,0.0);
float3 gSeamEdgePreviewColor = float3(0.0,1.0,1.0);
float3 gSeamEdgeColor = float3(0.0,0.0,1.0);
float3 gOpenEdgeColor = float3(0.0,1.0,0.0);
float3 gOpenEdgeSelectedColor = float3(1.0,0.1,0.9);
float gEdgeThickness = 1.0;
bool  gShowSelectedEdges = true;
extern float PcPriority : DepthPriority;
float PriorityAmplifier = 1.1f;

struct VS_IN
{
	float3 Pos : POSITION;
};

struct VS_OUT
{
	float4 VPos : SV_POSITION;
	float2 PosScreen : TEXCOORD0;
};

struct PS_INPUT
{
	float4 HPos : SV_POSITION;
	float3 fColor : COLOR;
};

float2 projToScreen(in float4 pos)
{
	return float2(ViewSize.x*0.5*((pos.x/pos.w) + 1),
					ViewSize.y*0.5*(1-(pos.y/pos.w)));
}

float4 screenToProj(in float2 screenVert,in float z,in float w)
{
	return float4((screenVert.x*2.0/ViewSize.x - 1)*w,
					(1-(screenVert.y*2.0/ViewSize.y))*w,
					z,
					w);
}

VS_OUT vs_main(VS_IN appIn)
{
	VS_OUT vsOut;
	vsOut.VPos = mul(float4(appIn.Pos.xyz, 1.0f), worldViewProjection);
	vsOut.VPos.z -= abs(PcPriority * PriorityAmplifier) * sign(vsOut.VPos.w);
	vsOut.PosScreen = projToScreen(vsOut.VPos);
	return vsOut;
}

[maxvertexcount(4)]
void gs_main( line VS_OUT input[2], uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> outStream)
{
	if(gSelectionAttribute[primID] == 0)
	{
		return;
	}

	PS_INPUT gsOut;
	if(gSelectionAttribute[primID] == 1)
	{
		gsOut.fColor = gSeamEdgePreviewColor;
	}
	else if(gSelectionAttribute[primID] == 2)
	{
		gsOut.fColor = gOpenEdgeColor;
	}
	else if(gSelectionAttribute[primID] == 3)
	{
		gsOut.fColor = gOpenEdgeSelectedColor;
	}
	else if(gSelectionAttribute[primID] == 4)
	{
		gsOut.fColor = gSeamEdgeColor;
	}
	else if(gSelectionAttribute[primID] == 5)
	{
		if(gShowSelectedEdges)
		{
			gsOut.fColor = gSelectedEdgeColor;
		}
		else
		{
			return;
		}
	}

	float2 dir = input[0].PosScreen - input[1].PosScreen;
	float2 perpend = float2(-dir.y, dir.x);
	perpend = normalize(perpend);
	
	float2 screenVert = input[0].PosScreen - perpend * gEdgeThickness;
	gsOut.HPos = screenToProj(screenVert,input[0].VPos.z,input[0].VPos.w);
	outStream.Append(gsOut);
	
	screenVert = input[1].PosScreen - perpend * gEdgeThickness;
	gsOut.HPos = screenToProj(screenVert,input[1].VPos.z,input[1].VPos.w);
	outStream.Append(gsOut);

	screenVert = input[0].PosScreen + perpend * gEdgeThickness;
	gsOut.HPos = screenToProj(screenVert,input[0].VPos.z,input[0].VPos.w);
	outStream.Append(gsOut);

	screenVert = input[1].PosScreen + perpend * gEdgeThickness;
	gsOut.HPos = screenToProj(screenVert,input[1].VPos.z,input[1].VPos.w);
	outStream.Append(gsOut);
	
	outStream.RestartStrip();
}

float4 ps_main(PS_INPUT gsIn) : SV_TARGET
{
	return float4(gsIn.fColor, 1.0f);
}

technique11 main
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_5_0, vs_main() ) );
        SetGeometryShader( CompileShader( gs_5_0, gs_main()));
        SetPixelShader( CompileShader( ps_5_0, ps_main() ) );
	}
}