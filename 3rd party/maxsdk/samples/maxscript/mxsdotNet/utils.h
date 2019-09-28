//**************************************************************************/
// Copyright (c) 1998-2006 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// AUTHOR: Larry.Minton / David Cunningham - created Oct 2, 2006
//***************************************************************************/
//

#pragma once

#pragma managed(push, off)

#include <string>
#include <unordered_map>

#pragma managed(pop)

namespace MXS_dotNet
{
	class dotNetBase;

#pragma managed(push, off)
	bool ShowMethodInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb, 
		bool showStaticOnly, bool showSpecial, bool showAttributes, bool declaredOnTypeOnly);
	bool ShowPropertyInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb, 
		bool showStaticOnly, bool showMethods, bool showAttributes, bool declaredOnTypeOnly);
	bool ShowEventInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb, 
		bool showStaticOnly, bool declaredOnTypeOnly);
	bool ShowFieldInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb, 
		bool showStaticOnly, bool showAttributes, bool declaredOnTypeOnly);
	/*
	// see comments in utils.cpp
	bool ShowInterfaceInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb);
	void GetInterfaces(System::Type ^type, Array* result);
	bool ShowAttributeInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb, bool declaredOnTypeOnly);
	void GetAttributes(System::Type ^type, Array* result);
	*/
	bool ShowConstructorInfo(System::Type ^type, System::Text::StringBuilder ^sb);
	void GetPropertyAndFieldNames(System::Type ^type, Array* propNames, 
		bool showStaticOnly, bool declaredOnTypeOnly);
	// This method converts an mxs value to a .net object. In MaxPlus, where we want to pass a native pointer (for example,
	// ::Mesh* mesh), due to SWIGing we end up having to declare the arg as void*, and swig outputs that as INT_PTR. Since
	// this loses all type info, for now we are passing the type as part of the arg name. So, in this case, we would declare
	// it as 'void* mesh_pointer'. For this to be handled, we are passing the parameter name and MXS_Value_To_Object will
	// parse the name to identify the desired type.
	// For a bit of type safety/generalization, we are also exposing a desired '::Mesh* mesh' as 'void* fpvalue_pointer'.
	// In this case, we create a new FPValue and set the FPValue via val->to_fpvalue. Since the lifetime of the FPValue
	// has to be beyond the invoke call, the caller to this method passes in a 'Tab<FPValue*>', this method adds the
	// created FPValue* (in any), and the caller is responsible for deleting the FPValue instances created.
	System::Object^ MXS_Value_To_Object(Value* val, 
										System::Type ^ type, 
										System::String ^ parameterName, 
										Tab<FPValue*> &backingFPValues);
	Value* Object_To_MXS_Value(System::Object^ val);

	void PrepArgList(array<System::Reflection::ParameterInfo^>^ candidateParamList, 
					Value**                 arg_list, 
					array<System::Object^>^ argArray, 
					int                     count,
					Tab<FPValue*>&			backingFPValues);
	// see MXS_Value_To_Object above
	int FindMethodAndPrepArgList(System::Collections::Generic::List<array<System::Reflection::ParameterInfo^>^>^ candidateParamLists, 
							Value**                 arg_list, 
							array<System::Object^>^ argArray,
							int                     count, 
							Array*                  userSpecifiedParamTypes,
							Tab<FPValue*>&			backingFPValues);
	bool isCompatible(array<System::Reflection::ParameterInfo^>^ candidateParamList,
					Value**   arg_list, 
					int       count);
	int CalcParamScore(Value *val, System::Type^ type, System::String ^ parameterName = nullptr);
	Value* CombineEnums(Value** arg_list, int count);
	bool CompareEnums(Value** arg_list, int count);
	Value* LoadAssembly(const MCHAR* assyDll, bool returnPassFail);
	// converts maxscript value to a dotNetObject value of type specified by argument 'type'
	Value* MXSValueToDotNetObjectValue(Value *val, dotNetBase *type);
	Value* CreateIconFromMaxMultiResIcon(const MSTR& iconName, int width, int height, bool enabled = true, bool on = false);
	// converts a .net object to a mxs value
	Value* DotNetObjectValueToMXSValue(dotNetBase *obj);
	// converts a .net array to a mxs value of given type
	Value* DotNetArrayToMXSValue(dotNetBase *obj, Value* type);

	System::Type^ ResolveTypeFromName(const MCHAR* pTypeString);

	// when calculating a param score (i.e., degree of match between supplied mxs value and param type), we use the following
	// scoring values
	static const int kParamScore_NoMatch = 0; 
	static const int kParamScore_PoorMatch = 3; 
	static const int kParamScore_GoodMatch = 6; 
	static const int kParamScore_ExactMatch = 9; 

#pragma managed(on)

	public ref class MaxPlusArgNameParsingConstants
	{
	public:
		// "_pointer" string used for type resolution parsing of argument name
		static System::String^ k_pointerString = gcnew System::String(_T("_pointer"));
		static const int k_pointerStringLen = 8; // and its length

		// strings corresponding to supported classes for type resolution parsing of argument name 
		static System::String^ k_fpvalueClassString = gcnew System::String(_T("fpvalue"));
		static System::String^ k_point3ClassString = gcnew System::String(_T("point3"));
		static System::String^ k_matrix3ClassString = gcnew System::String(_T("matrix3"));
		static System::String^ k_meshClassString = gcnew System::String(_T("mesh"));
		static System::String^ k_inodeClassString = gcnew System::String(_T("inode"));

		static System::String^ k_wstrClassString = gcnew System::String(_T("wstr"));
		static System::String^ k_point4ClassString = gcnew System::String(_T("point4"));
		static System::String^ k_point2ClassString = gcnew System::String(_T("point2"));
		static System::String^ k_rayClassString = gcnew System::String( _T("ray"));
		static System::String^ k_quatClassString = gcnew System::String(_T("quat"));
		static System::String^ k_angaxisClassString = gcnew System::String(_T("angaxis"));
		static System::String^ k_bitarrayClassString = gcnew System::String(_T("bitarray"));
		static System::String^ k_acolorClassString = gcnew System::String(_T("acolor"));
		static System::String^ k_intervalClassString = gcnew System::String(_T("interval"));
		static System::String^ k_colorClassString = gcnew System::String(_T("color"));
		static System::String^ k_box3ClassString = gcnew System::String(_T("box3"));

		static System::String^ k_referencemakerClassString = gcnew System::String(_T("referencemaker"));
		static System::String^ k_referencetargetClassString = gcnew System::String(_T("referencetarget"));
		static System::String^ k_mtlbaseClassString = gcnew System::String(_T("mtlbase"));
		static System::String^ k_mtlClassString = gcnew System::String(_T("mtl"));
		static System::String^ k_texmapClassString = gcnew System::String(_T("texmap"));
		static System::String^ k_beziershapeClassString = gcnew System::String(_T("beziershape"));

	};
#pragma managed(pop)

#pragma managed(push, off)
	namespace BuiltInTypeAlias
	{
		static std::unordered_map<std::wstring, const TCHAR*> *spBuiltInTypeMap;
		void Initialize();
		void Release();
		TSTR Convert(const MCHAR* pTypeString);
	}
#pragma managed(pop)
};
