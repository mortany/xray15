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
// DESCRIPTION: utils.cpp  : MAXScript dotNet common utility code
// AUTHOR: Larry.Minton - created Jan.1.2006
//***************************************************************************/
//
#pragma unmanaged

#include "MaxIncludes.h"
#include "dotNetControl.h"
#include "utils.h"
#include "resource.h"

#include <maxscript\macros\define_external_functions.h>
#	include "namedefs.h"

#ifdef ScripterExport
	#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

#ifndef PI
#define PI  ((float)3.1415926535)
#endif
#ifndef DEG_TO_RAD
#define DEG_TO_RAD (PI/(float)180.0)
#endif

// 
#pragma managed

#using <ManagedServices.dll>

using namespace MXS_dotNet;

/* -------------------- MNETStr (MSTR converter) --------------- */
//
MSTR MXS_dotNet::MNETStr::ToMSTR(System::String^ pString)
{
	cli::pin_ptr<const System::Char> pChar = PtrToStringChars( pString );
	const wchar_t *psz = pChar;

	return MSTR::FromUTF16(psz);
}

MSTR MXS_dotNet::MNETStr::ToMSTR(System::Exception^ e, bool InnerException)
{
	if( InnerException )
	{
		while( e->InnerException ) e = e->InnerException;
	}

	return ToMSTR(e->Message);
}

/* --------------------- MXS_dotNet functions ------------------ */
//

public ref class MethodInfoSorter: public System::Collections::IComparer
{
private:
	// Does case insensitive compare on MethodInfo names, then on # args
	virtual int Compare( System::Object^ x, System::Object^ y ) sealed = System::Collections::IComparer::Compare
	{
		if (x == y) return 0;
		if (x == nullptr) return -1;
		if (y == nullptr) return 1;
		using namespace System::Reflection;
		MethodInfo ^l_pMethod1 = dynamic_cast<MethodInfo^>(x);
		MethodInfo ^l_pMethod2 = dynamic_cast<MethodInfo^>(y);
		int res = System::String::Compare(l_pMethod1->Name, l_pMethod2->Name, false);
		if (res == 0)
			res = l_pMethod1->GetParameters()->Length - l_pMethod2->GetParameters()->Length;
		return res;
	}
};

bool MXS_dotNet::ShowMethodInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb, 
						   bool showStaticOnly, bool showSpecial, bool showAttributes, bool declaredOnTypeOnly)
{
	using namespace System;
	using namespace System::Reflection;

	BindingFlags flags = (BindingFlags)( BindingFlags::Static | BindingFlags::Public);
	if (!showStaticOnly)
		flags = (BindingFlags)(flags | BindingFlags::Instance);
	if (declaredOnTypeOnly)
		flags = (BindingFlags)(flags | BindingFlags::DeclaredOnly);
	else
		flags = (BindingFlags)(flags | BindingFlags::FlattenHierarchy);

	array<MethodInfo^> ^l_pMethods = type->GetMethods(flags);
	if (sb) // if not outputting, no need to sort methods
	{
		if (!showSpecial) // null out methods we aren't interested in
		{
			for(int m=0; m < l_pMethods->Length; m++)
			{
				MethodInfo ^l_pMethod = l_pMethods[m];
				if ((l_pMethod->Attributes & MethodAttributes::SpecialName) == MethodAttributes::SpecialName)
					l_pMethods[m] = nullptr;
			}
		}
		System::Collections::IComparer^ myComparer = gcnew MethodInfoSorter;
		System::Array::Sort(l_pMethods, myComparer);
	}

	bool res = false;
	for(int m=0; m < l_pMethods->Length; m++)
	{
		MethodInfo ^l_pMethod = l_pMethods[m];
		if (l_pMethod == nullptr) continue;
		if (!pattern || max_name_match(MNETStr::ToMSTR(l_pMethod->Name), pattern))
		{
			res = true;
			if (!sb) 
				return res; // signifies method was found
			sb->Append(_T("  ."));
			if (l_pMethod->IsStatic)
				sb->Append(_T("[static]"));
			if (l_pMethod->ReturnType == System::Void::typeid)
			{
				sb->AppendFormat(
					_T("{0}"),
					l_pMethod->Name);
			}
			else
			{
				sb->AppendFormat(
					_T("<{0}>{1}"),
					l_pMethod->ReturnType, l_pMethod->Name);
			}

			array<ParameterInfo^> ^l_pParams = l_pMethod->GetParameters();
			if (l_pParams->Length == 0)
				sb->Append(_T("()"));

			for(int p=0; p < l_pParams->Length; p++)
			{
				ParameterInfo ^l_pParam = l_pParams[p];
				sb->Append(_T(" "));
				if (l_pParam->IsIn)
					sb->Append(_T("[in]"));
				if (l_pParam->IsOut)
					sb->Append(_T("[out]"));
				System::String ^ l_pParamTypeName = l_pParam->ParameterType->FullName;
				if (!l_pParamTypeName)
					l_pParamTypeName = l_pParam->ParameterType->Name;
				sb->AppendFormat(
					_T("<{0}>{1}"),
					l_pParamTypeName, l_pParam->Name);
			}
			if (showAttributes)
			{
				sb->AppendFormat(
					_T(" [Attributes: {0}]"),
					l_pMethod->Attributes.ToString());
			}
			sb->Append(_T("\n"));
		}
	}
	return res;
}

public ref class PropertyInfoSorter: public System::Collections::IComparer
{
private:
	// Does case insensitive compare on PropertyInfo names
	virtual int Compare( System::Object^ x, System::Object^ y ) sealed = System::Collections::IComparer::Compare
	{
		if (x == y) return 0;
		if (x == nullptr) return -1;
		if (y == nullptr) return 1;
		using namespace System::Reflection;
		PropertyInfo ^l_pProp1 = dynamic_cast<PropertyInfo^>(x);
		PropertyInfo ^l_pProp2 = dynamic_cast<PropertyInfo^>(y);
		return System::String::Compare(l_pProp1->Name, l_pProp2->Name, false);
	}
};

bool MXS_dotNet::ShowPropertyInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb, 
							 bool showStaticOnly, bool showMethods, bool showAttributes, bool declaredOnTypeOnly)
{
	using namespace System;
	using namespace System::Reflection;

	BindingFlags flags = (BindingFlags)( BindingFlags::Static | BindingFlags::Public);
	if (!showStaticOnly)
		flags = (BindingFlags)(flags | BindingFlags::Instance);
	if (declaredOnTypeOnly)
		flags = (BindingFlags)(flags | BindingFlags::DeclaredOnly);
	else
		flags = (BindingFlags)(flags | BindingFlags::FlattenHierarchy);

	array<PropertyInfo^>^ l_pProps = type->GetProperties(flags);
	if (sb) // if not outputting, no need to sort properties
	{
		System::Collections::IComparer^ myComparer = gcnew PropertyInfoSorter;
		System::Array::Sort(l_pProps, myComparer);
	}

	bool res = false;
	for(int m=0; m < l_pProps->Length; m++)
	{
		PropertyInfo ^l_pProp = l_pProps[m];
		if (!pattern || max_name_match(MNETStr::ToMSTR(l_pProp->Name), pattern))
		{
			res = true;
			if (!sb) 
				return res; // signifies property was found
			sb->AppendFormat(
				_T("  .{0}"),
				l_pProp->Name);
			MethodInfo ^l_pAccessor;
			if (l_pProp->CanRead)
			{
				l_pAccessor = l_pProp->GetGetMethod();
				if (l_pAccessor)
				{
					array<ParameterInfo^> ^l_pParams = l_pAccessor->GetParameters();
					if (l_pParams->Length != 0)
					{
						sb->Append(_T("["));
						bool first = true;
						for (int i = 0; i < l_pParams->Length; i++)
						{
							if (!first)
								sb->Append(_T(", "));
							first = false;
							ParameterInfo^ l_pParam = l_pParams[i];
							System::String ^ l_pParamTypeName = l_pParam->ParameterType->FullName;
							if (!l_pParamTypeName)
								l_pParamTypeName = l_pParam->ParameterType->Name;
							sb->AppendFormat(
								_T("<{0}>{1}"),
								l_pParamTypeName, l_pParam->Name);
						}
						sb->Append(_T("]"));
					}
				}
			}
			else if (l_pProp->CanWrite)
			{
				l_pAccessor = l_pProp->GetSetMethod();
				if (l_pAccessor)
				{
					array<ParameterInfo^> ^l_pParams = l_pAccessor->GetParameters();
					if (l_pParams->Length != 1)
					{
						sb->Append(_T("["));
						bool first = true;
						for (int i = 0; i < l_pParams->Length-1; i++)
						{
							if (!first)
								sb->Append(_T(", "));
							first = false;
							ParameterInfo^ l_pParam = l_pParams[i];
							System::String ^ l_pParamTypeName = l_pParam->ParameterType->FullName;
							if (!l_pParamTypeName)
								l_pParamTypeName = l_pParam->ParameterType->Name;
							sb->AppendFormat(
								_T("<{0}>{1}"),
								l_pParamTypeName, l_pParam->Name);
						}
						sb->Append(_T("]"));
					}
				}
			}

			sb->AppendFormat(
				_T(" : <{0}>"),
				l_pProp->PropertyType);
			if (!l_pProp->CanRead)
				sb->Append(_T(", write-only"));
			if (!l_pProp->CanWrite)
				sb->Append(_T(", read-only"));
			if (l_pAccessor && l_pAccessor->IsStatic)
				sb->Append(_T(", static"));
			if (showAttributes)
			{
				sb->AppendFormat(
					_T(", Attributes: {0}"),
					l_pProp->Attributes.ToString());
			}
			// debug testing here for multiple accessors, accessors that take additional args
			{
				array<MethodInfo^> ^l_pAccessors = l_pProp->GetAccessors();
				int nGetAccessors = 0;
				int nSetAccessors = 0;
				for (int i = 0; i < l_pAccessors->Length; i++)
				{
					MethodInfo ^l_pAccessor = l_pAccessors[i];
					if (l_pAccessor->ReturnType == System::Void::typeid)
						nSetAccessors++;
					else
						nGetAccessors++;
				}
				MethodInfo ^l_pAccessor = l_pProp->GetGetMethod();
				int i = (l_pAccessor != nullptr) ? 1 : 0;
				DbgAssert(i == nGetAccessors);
				if (l_pAccessor)
				{
					array<ParameterInfo^> ^l_pParams = l_pAccessor->GetParameters();
					DbgAssert(l_pParams->Length <= 1);
				}
				l_pAccessor = l_pProp->GetSetMethod();
				i = (l_pAccessor != nullptr) ? 1 : 0;
				DbgAssert(i == nSetAccessors);
				if (l_pAccessor)
				{
					array<ParameterInfo^> ^l_pParams = l_pAccessor->GetParameters();
					DbgAssert(l_pParams->Length <= 2);
				}
			}
			if (showMethods)
			{
				MethodInfo ^l_pAccessor = l_pProp->GetGetMethod();
				if (l_pAccessor)
				{
					sb->AppendFormat(
						_T(", get method: {0}, Attributes: {1}"), 
						l_pAccessor->Name, l_pAccessor->Attributes.ToString());
				}
				l_pAccessor = l_pProp->GetSetMethod();
				if (l_pAccessor)
				{
					sb->AppendFormat(
						_T(", set method: {0}, Attributes: {1}"), 
						l_pAccessor->Name, l_pAccessor->Attributes.ToString());
				}

				array<MethodInfo^> ^l_pAccessors = l_pProp->GetAccessors();
				sb->AppendFormat(_T("\n    Accessor methods:"));
				for(int p=0; p < l_pAccessors->Length; p++)
				{
					MethodInfo ^l_pAccessor = l_pAccessors[p];
					sb->AppendFormat(
						_T("\n      Name: {0}, Attributes: {1}"), 
						l_pAccessor->Name, l_pAccessor->Attributes.ToString());
				}
			}
			sb->Append(_T("\n"));
		}
	}
	return res;
}

public ref class EventInfoSorter: public System::Collections::IComparer
{
private:
	// Does case insensitive compare on EventInfo names
	virtual int Compare( System::Object^ x, System::Object^ y ) sealed = System::Collections::IComparer::Compare
	{
		if (x == y) return 0;
		if (x == nullptr) return -1;
		if (y == nullptr) return 1;
		using namespace System::Reflection;
		EventInfo ^l_pEvent1 = dynamic_cast<EventInfo^>(x);
		EventInfo ^l_pEvent2 = dynamic_cast<EventInfo^>(y);
		return System::String::Compare(l_pEvent1->Name, l_pEvent2->Name, false);
	}
};

bool MXS_dotNet::ShowEventInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb, 
							   bool showStaticOnly, bool declaredOnTypeOnly)
{
	using namespace System;
	using namespace System::Reflection;

	// EventInfo doesn't include an IsStatic member, so we need to collect the events in 2 passes - 
	// one for statics, one for instances

	BindingFlags flags = (BindingFlags)( BindingFlags::Static | BindingFlags::Public);
	if (declaredOnTypeOnly)
		flags = (BindingFlags)(flags | BindingFlags::DeclaredOnly);
	else
		flags = (BindingFlags)(flags | BindingFlags::FlattenHierarchy);

	array<EventInfo^> ^l_pEvents = type->GetEvents(flags);
	int numStaticEvents = l_pEvents->Length;

	if (!showStaticOnly)
	{
		flags = (BindingFlags)( BindingFlags::Instance | BindingFlags::Public);
		if (declaredOnTypeOnly)
			flags = (BindingFlags)(flags | BindingFlags::DeclaredOnly);
		else
			flags = (BindingFlags)(flags | BindingFlags::FlattenHierarchy);
		array<EventInfo^> ^l_pEvents_nonstatic = type->GetEvents(flags);
		if (l_pEvents_nonstatic->Length != 0)
		{
			System::Array::Resize(l_pEvents, l_pEvents_nonstatic->Length + numStaticEvents);
			l_pEvents_nonstatic->CopyTo(l_pEvents, numStaticEvents);
		}
	}

	if (sb) // if not outputting, no need to sort events
	{
		System::Collections::IComparer^ myComparer = gcnew EventInfoSorter;
		System::Array::Sort(l_pEvents, myComparer);
	}

	bool res = false;
	bool isControl = type->IsSubclassOf(System::Windows::Forms::Control::typeid);
	for(int e=0; e < l_pEvents->Length; e++)
	{
		EventInfo^ l_pEvent = l_pEvents[e];
		if (!pattern || max_name_match(MNETStr::ToMSTR(l_pEvent->Name), pattern))
		{
			res = true;
			if (!sb) 
				return res; // signifies event was found

			if (e < numStaticEvents)
				sb->Append(_T("   [static] "));
			else
				sb->Append(_T("   "));

			if (isControl)
				sb->AppendFormat(
					_T("on <control_name> {0}"),
					l_pEvent->Name);
			else
				sb->AppendFormat(
				_T("{0}"),
				l_pEvent->Name);


			Type ^l_pEventHandlerType = l_pEvent->EventHandlerType;
			Type ^l_pDelegateReturnType = l_pEventHandlerType->GetMethod(_T("Invoke"))->ReturnType;

			array<ParameterInfo^> ^l_pParams = l_pEventHandlerType->GetMethod(_T("Invoke"))->GetParameters();
			for (int i = isControl ? 1 : 0; i < l_pParams->Length; i++)
			{
				ParameterInfo ^l_pParam = l_pParams[i];
				sb->Append(_T(" "));
				if (l_pParam->IsIn)
					sb->Append(_T("[in]"));
				if (l_pParam->IsOut)
					sb->Append(_T("[out]"));
				System::String ^ l_pParamTypeName = l_pParam->ParameterType->FullName;
				if (!l_pParamTypeName)
					l_pParamTypeName = l_pParam->ParameterType->Name;
				sb->AppendFormat(
					_T("<{0}>{1}"),
					l_pParamTypeName, l_pParam->Name);
			}

			if (isControl)
				sb->Append(_T(" do ( ... )"));
			else
				sb->Append(_T(" = ( ... )"));
			if (l_pEvent->Attributes != EventAttributes::None)
			{
				sb->AppendFormat(
					_T(" [Attributes: {0}]"),
					l_pEvent->Attributes.ToString());
			}
			if (l_pDelegateReturnType != System::Void::typeid)
			{
				System::String ^ l_pReturnTypeName = l_pDelegateReturnType->FullName;
				if (!l_pReturnTypeName)
					l_pReturnTypeName = l_pDelegateReturnType->Name;
				sb->AppendFormat(
					_T(" [ return type: <{0}> ]"),
					l_pReturnTypeName);
			}
			sb->Append(_T("\n"));
		}
	}
	return res;
}

public ref class FieldInfoSorter: public System::Collections::IComparer
{
private:
	// Does case insensitive compare on FieldInfo names
	virtual int Compare( System::Object^ x, System::Object^ y ) sealed = System::Collections::IComparer::Compare
	{
		if (x == y) return 0;
		if (x == nullptr) return -1;
		if (y == nullptr) return 1;
		using namespace System::Reflection;
		FieldInfo ^l_pField1 = dynamic_cast<FieldInfo^>(x);
		FieldInfo ^l_pField2 = dynamic_cast<FieldInfo^>(y);
		return System::String::Compare(l_pField1->Name, l_pField2->Name, false);
	}
};

bool MXS_dotNet::ShowFieldInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb, 
						  bool showStaticOnly, bool showAttributes, bool declaredOnTypeOnly)
{
	using namespace System;
	using namespace System::Reflection;

	BindingFlags flags = (BindingFlags)( BindingFlags::Static | BindingFlags::Public);
	if (!showStaticOnly)
		flags = (BindingFlags)(flags | BindingFlags::Instance);
	if (declaredOnTypeOnly)
		flags = (BindingFlags)(flags | BindingFlags::DeclaredOnly);
	else
		flags = (BindingFlags)(flags | BindingFlags::FlattenHierarchy);

	array<FieldInfo^> ^l_pFields = type->GetFields(flags);
	if (sb) // if not outputting, no need to sort fields
	{
		System::Collections::IComparer^ myComparer = gcnew FieldInfoSorter;
		System::Array::Sort(l_pFields, myComparer);
	}

	bool res = false;
	for(int f=0; f < l_pFields->Length; f++)
	{
		FieldInfo^ l_pField = l_pFields[f];
		if (!pattern || max_name_match(MNETStr::ToMSTR(l_pField->Name), pattern))
		{
			res = true;
			if (!sb) 
				return res; // signifies property was found
			sb->AppendFormat(
				_T("  .{0} : <{1}>"),
				l_pField->Name, l_pField->FieldType);
			if (l_pField->IsInitOnly || l_pField->IsLiteral)
				sb->Append(_T(", read-only"));
			if (l_pField->IsStatic)
				sb->Append(_T(", static"));
			if (showAttributes)
			{
				sb->AppendFormat(
					_T(", Attributes: {0}"),
					l_pField->Attributes.ToString());
			}
			sb->Append(_T("\n"));
		}
	}
	return res;
}

/*
// too late to add to r9. This implementation of GetInterfaces returns the System::Type 
// associated with the interfaces. This isn't what is desired. What we want to do is return
// the object, but make it look like it has been cast to the interface type. For example:
//   myString = dotNetObject "System.String" "AAA"
//   showMethods myString -- shows all methods of String
//   newString = myString.Copy()
//   iclonable = getInterface myString "IClonable"
//   showMethods iclonable -- shows just that methods of IClonable interface
//   clonedString = iclonable.Clone()
// to do this means modifying DotNetObjectManaged to also store the Type it was created as - if nullptr
// use the type of the object
bool MXS_dotNet::ShowInterfaceInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb)
{
	using namespace System;
	using namespace System::Reflection;

	array<Type^> ^l_pInterfaces = type->GetInterfaces();
	bool res = false;
	for(int f=0; f < l_pInterfaces->Length; f++)
	{
		Type^ l_pInterface = l_pInterfaces[f];
		if (!pattern || max_name_match(MNETStr::ToMSTR(l_pInterface->Name), pattern))
		{
			res = true;
			if (!sb) 
				return res; // signifies interface was found
			sb->AppendFormat(
				_T("  .{0}"),
				l_pInterface->Name);
			sb->Append(_T("\n"));
		}
	}
	return res;
}

void MXS_dotNet::GetInterfaces(System::Type ^type, Array* result)
{
	using namespace System;
	using namespace System::Reflection;

	array<Type^> ^l_pInterfaces = type->GetInterfaces();
	for(int f=0; f < l_pInterfaces->Length; f++)
	{
		Type^ l_pInterface = l_pInterfaces[f];
		result->append(DotNetObjectWrapper::intern(l_pInterface));
	}
	return;
}

bool MXS_dotNet::ShowAttributeInfo(System::Type ^type, const MCHAR* pattern, System::Text::StringBuilder ^sb,
								   bool declaredOnTypeOnly)
{
	using namespace System;
	using namespace System::Reflection;

	array<System::Object^> ^l_pAttributes = type->GetCustomAttributes(!declaredOnTypeOnly);
	bool res = false;
	for(int f=0; f < l_pAttributes->Length; f++)
	{
		System::Object^ l_pAttribute = l_pAttributes[f];
		if (!pattern || max_name_match(MNETStr::ToMSTR(l_pAttribute->GetType()->Name), pattern))
		{
			res = true;
			if (!sb) 
				return res; // signifies interface was found
			sb->AppendFormat(
				_T("  .{0}"),
				l_pAttribute->GetType()->Name);
			sb->Append(_T("\n"));
		}
	}
	return res;
}

void MXS_dotNet::GetAttributes(System::Type ^type, Array* result)
{
	using namespace System;
	using namespace System::Reflection;

	array<System::Object^> ^l_pAttributes = type->GetCustomAttributes(true);
	for(int f=0; f < l_pAttributes->Length; f++)
	{
		System::Object^ l_pAttribute = l_pAttributes[f];
		result->append(DotNetObjectWrapper::intern(l_pAttribute));
	}
	return;
}
*/

public ref class ConstructorInfoSorter: public System::Collections::IComparer
{
private:
	// Does case insensitive compare on MethodInfo names, then on # args
	virtual int Compare( System::Object^ x, System::Object^ y ) sealed = System::Collections::IComparer::Compare
	{
		if (x == y) return 0;
		if (x == nullptr) return -1;
		if (y == nullptr) return 1;
		using namespace System::Reflection;
		ConstructorInfo ^l_pConstructInfo1 = dynamic_cast<ConstructorInfo^>(x);
		ConstructorInfo ^l_pConstructInfo2 = dynamic_cast<ConstructorInfo^>(y);
		int res = System::String::Compare(l_pConstructInfo1->Name, l_pConstructInfo2->Name, false);
		if (res == 0)
			res = l_pConstructInfo1->GetParameters()->Length - l_pConstructInfo2->GetParameters()->Length;
		return res;
	}
};

bool MXS_dotNet::ShowConstructorInfo(System::Type ^type, System::Text::StringBuilder ^sb)
{
	using namespace System;
	using namespace System::Reflection;

	array<ConstructorInfo^> ^l_pConstructors = type->GetConstructors();
	if (sb) // if not outputting, no need to sort constructors
	{
		System::Collections::IComparer^ myComparer = gcnew ConstructorInfoSorter;
		System::Array::Sort(l_pConstructors, myComparer);
	}

	bool res = false;
	for(int c=0; c < l_pConstructors->Length; c++)
	{
		ConstructorInfo^ l_pConstructor = l_pConstructors[c];
		{
			res = true;
			if (!sb) 
				return res; // signifies property was found

			System::String ^ l_pTypeName = type->FullName;
			if (!l_pTypeName)
				l_pTypeName = type->Name;
			sb->AppendFormat(
				_T("  {0}"),
				l_pTypeName);

			array<ParameterInfo^> ^l_pParams = l_pConstructor->GetParameters();
			if (l_pParams->Length == 0)
				sb->Append(_T("()"));

			for(int p=0; p < l_pParams->Length; p++)
			{
				ParameterInfo ^l_pParam = l_pParams[p];
				sb->Append(_T(" "));
				if (l_pParam->IsIn)
					sb->Append(_T("[in]"));
				if (l_pParam->IsOut)
					sb->Append(_T("[out]"));
				System::String ^ l_pParamTypeName = l_pParam->ParameterType->FullName;
				if (!l_pParamTypeName)
					l_pParamTypeName = l_pParam->ParameterType->Name;
				sb->AppendFormat(
					_T("<{0}>{1}"),
					l_pParamTypeName, l_pParam->Name);
			}
			sb->Append(_T("\n"));
		}
	}
	return res;
}

void MXS_dotNet::GetPropertyAndFieldNames(System::Type ^type, Array* propNames, 
										  bool showStaticOnly, bool declaredOnTypeOnly)
{
	using namespace System;
	using namespace System::Reflection;

	BindingFlags flags = (BindingFlags)( BindingFlags::Static | BindingFlags::Public);
	if (!showStaticOnly)
		flags = (BindingFlags)(flags | BindingFlags::Instance);
	if (declaredOnTypeOnly)
		flags = (BindingFlags)(flags | BindingFlags::DeclaredOnly);
	else
		flags = (BindingFlags)(flags | BindingFlags::FlattenHierarchy);

	array<PropertyInfo^>^ l_pProps = type->GetProperties(flags);
	System::Collections::IComparer^ myComparer = gcnew PropertyInfoSorter;
	System::Array::Sort(l_pProps, myComparer);
	for(int m=0; m < l_pProps->Length; m++)
	{
		PropertyInfo ^l_pProp = l_pProps[m];
		propNames->append(Name::intern(MNETStr::ToMSTR(l_pProp->Name)));
	}
	array<FieldInfo^> ^l_pFields = type->GetFields(flags);
	myComparer = gcnew FieldInfoSorter;
	System::Array::Sort(l_pFields, myComparer);
	for(int f=0; f < l_pFields->Length; f++)
	{
		FieldInfo^ l_pField = l_pFields[f];
		propNames->append(Name::intern(MNETStr::ToMSTR(l_pField->Name)));
	}
}

void CalculateArrayDimensions(Value* val, int rank, System::Collections::Generic::List<int> ^ dimensions)
{
	if (rank == 0)
		return;
	if (is_array(val))
	{
		Array* l_theArray = (Array*)val;
		int l_size = l_theArray->size;
		dimensions->Add(l_size);
		if (l_size != 0)
			CalculateArrayDimensions(l_theArray->data[0], rank - 1, dimensions);
	}
	else if (is_point2(val))
		dimensions->Add(2);
	else if (is_point3(val))
		dimensions->Add(3);
	else if (is_point4(val))
		dimensions->Add(4);
	else if (is_quat(val))
		dimensions->Add(4);
	else if (is_eulerangles(val))
		dimensions->Add(3);
	else if (is_matrix3(val))
	{
		dimensions->Add(4);
		if (rank > 1)
			dimensions->Add(4);
	}
	else
		dimensions->Add(1);
}

System::Object^ MXS_dotNet::MXS_Value_To_Object(Value* val, System::Type ^ type, System::String ^ parameterName, Tab<FPValue*> &backingFPValues)
{
	if (type->IsArray && is_array(val))
	{
		System::Type^  l_pEleType  = type->GetElementType();
		int            l_rank = type->GetArrayRank();
		int            l_rank_m1 = l_rank - 1;
		System::Collections::Generic::List<int> ^ l_dimensions = gcnew System::Collections::Generic::List<int>;
		CalculateArrayDimensions(val, l_rank, l_dimensions);
		// Make sure have dimension value for each array rank, in case fewer mxs array values than needed for the rank size.
		// Will get a array mismatch error later, but this will safely get us to that point.
		while (l_dimensions->Count < l_rank)
			l_dimensions->Add(1);
		Array*         l_theArray  = (Array*)val;
		System::Array^ l_pResArray = System::Array::CreateInstance(l_pEleType, l_dimensions->ToArray());
		if (l_rank > 1)
			l_pEleType = l_pEleType->MakeArrayType(l_rank - 1);
		for (int i = 0; i < l_theArray->size; i++)
		{
			System::Object^ l_ptr = MXS_dotNet::MXS_Value_To_Object(l_theArray->data[i], l_pEleType, parameterName, backingFPValues);
			if (l_rank > 1)
			{
				// do an n-dimensional copy from a n-dimensional source array to a (n+1)-dimensional target array
				// where first dimension of the target reflect the mxs array index (for loop variable 'i' above)
				System::Array^ l_pValArray = dynamic_cast<System::Array^>(l_ptr);
				array<int> ^ target_indices = gcnew array<int>(l_rank);
				array<int> ^ source_indices = gcnew array<int>(l_rank_m1);
				array<int> ^ limits = gcnew array<int>(l_rank_m1); // # elements for each dimension in source
				target_indices[0] = i;
				for (int j = 0; j < l_rank_m1; j++)
				{
					limits[j] = l_dimensions[j + 1];
					if (limits[j] != l_pValArray->GetLength(j))
					{
						TSTR msg;
						msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_ARRAY_SIZE_GOT_ARRAY_SIZE), limits[j], l_pValArray->GetLength(j));
						throw RuntimeError(msg);
					}
					target_indices[j + 1] = source_indices[j] = 0;
				}

				// fun stuff here!  For a 2x3 array will copy values in the following order:
				// [0,0], [1,0], [0,1], [1,1], [0,2], [1,2]
				int l_index = 0;
				while (l_index < l_rank_m1)
				{
					l_index = 0;
					System::Object^ l_val = l_pValArray->GetValue(source_indices);
					l_pResArray->SetValue(l_val, target_indices);
					target_indices[l_index + 1]++;
					source_indices[l_index]++;
					while ((l_index < l_rank_m1) && (source_indices[l_index] >= limits[l_index]))
					{
						target_indices[l_index + 1] = 0;
						source_indices[l_index] = 0;
						l_index++;
						if (l_index < l_rank_m1)
						{
							target_indices[l_index + 1]++;
							source_indices[l_index]++;
						}
					}
				}
			}
			else
			{
				// simplified handling of 1 dimensional array
				l_pResArray->SetValue(l_ptr, i);
			}
		}
		return l_pResArray;
	}

	// for 'byref' and 'byptr' types, get the underlying type. But don't lose whether we are
	// dealing with an Array type.
	System::Type^ l_pOrigType = type;
	if (type->HasElementType)
	{
		type = type->GetElementType();
		if (l_pOrigType->IsArray)
		{
			type = type->MakeArrayType(l_pOrigType->GetArrayRank());
		}
	}

	// special pointer cases! Want to handle the maxplus classes.
	// For maxplus, the type will be System::IntPtr::typeid (exposed in maxplus as void*). Since
	// that is all we know, we require the parameter name to be of the form XXXX_pointer. We strip
	// off the '_pointer' and see if the remainder matches one of the types we support. 
	// What a completely F'ed up hack this is....
	// In the future, we want to be able to handle both the esphere and maxplus classes. esphere
	// exposes methods/ctors using native 3ds Max classes, which we can't pass via invoke. So
	// look at having a way to register with mxs specialized classes implementing 
	// MXS_Value_To_Object, Object_To_MXS_Value, and CalcParamScore. These could be implemented
	// in a dlx plugin that is linked against esphere or maxplus (or whatever else out there
	// wants to be able to handle 3ds Max classes through mxs).
	// This would work for esphere, but currently not maxplus. The maxplus classes don't show
	// 3ds Max native classes as args, rather some mangled swig class name. Would need to figure
	// out a way to expose with non-mangled names too.

	// handle maxplus interface - type will be IntPtr, need parameterName to determine its class
	if (type == System::IntPtr::typeid && parameterName != nullptr && parameterName->EndsWith(MaxPlusArgNameParsingConstants::k_pointerString))
	{
		System::String ^ classname = parameterName->Substring(0, parameterName->Length-MaxPlusArgNameParsingConstants::k_pointerStringLen);
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_fpvalueClassString))
		{
			FPValue* fpValue = new FPValue;  // we delete this after done with IntPtr created below
			try { val->to_fpvalue(*fpValue); }
			catch (...) { delete fpValue; throw; }
			System::IntPtr^ res = gcnew System::IntPtr((INT_PTR)fpValue);
			backingFPValues.Append(1, &fpValue);
			return res;
		}

		if (val == &undefined &&
			( classname->Equals(MaxPlusArgNameParsingConstants::k_referencemakerClassString) || 
			  classname->Equals(MaxPlusArgNameParsingConstants::k_referencetargetClassString)  || 
			  classname->Equals(MaxPlusArgNameParsingConstants::k_mtlbaseClassString) || 
			  classname->Equals(MaxPlusArgNameParsingConstants::k_mtlClassString) || 
			  classname->Equals(MaxPlusArgNameParsingConstants::k_texmapClassString) ||
			  classname->Equals(MaxPlusArgNameParsingConstants::k_meshClassString) ||
			  classname->Equals(MaxPlusArgNameParsingConstants::k_beziershapeClassString) ||
			  classname->Equals(MaxPlusArgNameParsingConstants::k_inodeClassString)
			)
		   )
		{
			System::IntPtr^ res = gcnew System::IntPtr(0);
			return res;
		}

		if (classname->Equals(MaxPlusArgNameParsingConstants::k_meshClassString) && is_mesh(val))
		{
			Mesh* mesh = val->to_mesh();
			System::IntPtr^ res = gcnew System::IntPtr((INT_PTR)mesh);
			return res;
		}
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_beziershapeClassString) && is_beziershape(val))
		{
			BezierShape* beziershape = (val->to_beziershape());
			System::IntPtr^ res = gcnew System::IntPtr((INT_PTR)beziershape);
			return res;
		}
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_inodeClassString) && is_node(val))
		{
			INode* node = val->to_node();
			System::IntPtr^ res = gcnew System::IntPtr((INT_PTR)node);
			return res;
		}
		if ( (classname->Equals(MaxPlusArgNameParsingConstants::k_referencemakerClassString) && val->derives_from_MAXWrapper()) || 
		     (classname->Equals(MaxPlusArgNameParsingConstants::k_referencetargetClassString) && val->derives_from_MAXWrapper()) || 
		     (classname->Equals(MaxPlusArgNameParsingConstants::k_mtlbaseClassString) && (val->is_kind_of(class_tag(MAXMaterial)) || val->is_kind_of(class_tag(MAXTexture)))) || 
		     (classname->Equals(MaxPlusArgNameParsingConstants::k_mtlClassString) && val->is_kind_of(class_tag(MAXMaterial))) || 
		     (classname->Equals(MaxPlusArgNameParsingConstants::k_texmapClassString) && val->is_kind_of(class_tag(MAXTexture)))
		   )
		{
			FPValue* fpValue = new FPValue;  // we delete this after done with IntPtr created below
			try { val->to_fpvalue(*fpValue); }
			catch (...) { delete fpValue; throw; }
			backingFPValues.Append(1, &fpValue);
			System::IntPtr^ res = gcnew System::IntPtr((INT_PTR)fpValue->p);
			return res;
		}
		if ( (classname->Equals(MaxPlusArgNameParsingConstants::k_matrix3ClassString) && is_matrix3(val)) ||
		     (classname->Equals(MaxPlusArgNameParsingConstants::k_point3ClassString) && is_point3(val)) ||
		     (classname->Equals(MaxPlusArgNameParsingConstants::k_point4ClassString) && is_point4(val)) ||
		     (classname->Equals(MaxPlusArgNameParsingConstants::k_point2ClassString) && is_point2(val)) ||
		     (classname->Equals(MaxPlusArgNameParsingConstants::k_rayClassString) && is_ray(val)) ||
			 (classname->Equals(MaxPlusArgNameParsingConstants::k_quatClassString) && is_quat(val)) ||
			 (classname->Equals(MaxPlusArgNameParsingConstants::k_angaxisClassString) && is_angaxis(val)) ||
			 (classname->Equals(MaxPlusArgNameParsingConstants::k_bitarrayClassString) && is_bitarray(val)) ||
			 (classname->Equals(MaxPlusArgNameParsingConstants::k_intervalClassString) && is_interval(val)) ||
			 (classname->Equals(MaxPlusArgNameParsingConstants::k_box3ClassString) && is_box3(val))
		   )
		{
			// FPValue will provide lifetime control on the value passed as INT_PTR below
			FPValue* fpValue = new FPValue;  // we delete this after done with IntPtr created below
			try { val->to_fpvalue(*fpValue); }
			catch (...) { delete fpValue; throw; }
			backingFPValues.Append(1, &fpValue);
			System::IntPtr^ res = gcnew System::IntPtr((INT_PTR)fpValue->p);
			return res;
		}
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_wstrClassString) && (is_name(val) || is_string(val)))
		{
			// FPValue will provide lifetime control on the value passed as INT_PTR below
			FPValue* fpValue = new FPValue;  // we delete this after done with IntPtr created below
			backingFPValues.Append(1, &fpValue);
			fpValue->tstr = new MSTR(val->to_string()); 
			fpValue->type = TYPE_TSTR;
			System::IntPtr^ res = gcnew System::IntPtr((INT_PTR)fpValue->tstr);
			return res;
		}
		if ( (classname->Equals(MaxPlusArgNameParsingConstants::k_acolorClassString) && is_color(val)) ||
			 (classname->Equals(MaxPlusArgNameParsingConstants::k_colorClassString) && is_color(val))
		   )
		{
			// FPValue will provide lifetime control on the value passed as INT_PTR below
			FPValue* fpValue = new FPValue;  // we delete this after done with IntPtr created below
			backingFPValues.Append(1, &fpValue);
			if (classname->Equals(MaxPlusArgNameParsingConstants::k_colorClassString))
			{
				fpValue->clr = new Color (((ColorValue*)val)->to_color()); 
				fpValue->type = TYPE_COLOR_BV;
			}
			else
			{
				fpValue->aclr = new AColor (val->to_acolor()); 
				fpValue->type = TYPE_FRGBA_BV;
			}
			System::IntPtr^ res = gcnew System::IntPtr((INT_PTR)fpValue->clr);
			return res;
		}
	}

	// special case to handle point2/3/4, quat, and eulerangle as float[], and matrix3 as float[,]
	if (is_point2(val) || is_point3(val) || is_point4(val) || is_quat(val) || is_eulerangles(val) || is_matrix3(val))
	{
		System::Type^  l_pEleType = type->GetElementType();
		if (type->IsArray && (l_pEleType == System::Single::typeid || l_pEleType == System::Double::typeid))
		{
			int l_rank = type->GetArrayRank();
			float *data = nullptr;
			int nVals = 0;
			Point2 p2;
			Point3 p3;
			Point4 p4;
			Quat q;
			Matrix3 m;

			if (is_point2(val))
			{
				p2 = val->to_point2();
				data = p2;
				nVals = 2;
			}
			else if (is_point3(val))
			{
				p3 = val->to_point3();
				data = p3;
				nVals = 3;
			}
			else if (is_point4(val))
			{
				p4 = val->to_point4();
				data = p4;
				nVals = 4;
			}
			else if (is_quat(val))
			{
				q = val->to_quat();
				data = q;
				nVals = 4;
			}
			else if (is_eulerangles(val))
			{
				data = val->to_eulerangles();
				nVals = 3;
			}
			else if (is_matrix3(val))
			{
				m = val->to_matrix3();
				// uncomment following for a Y up matrix
				// m.RotateX(90.0 * DEG_TO_RAD);
				// m.Scale(Point3(1.0f, -1.0f, 1.0f), TRUE);
				array<int> ^ size = gcnew array<int>(2){ 4,4 };
				System::Array^ l_pResArray = System::Array::CreateInstance(l_pEleType, size);
				MRow *pM = m.GetAddr();
				for (int i = 0; i < 4; i++)
					for (int j = 0; j < 3; j++) 
						l_pResArray->SetValue(pM[i][j], j, i);
				for (int i = 0; i < 3; i++)
					l_pResArray->SetValue((float)0., 3, i);
				l_pResArray->SetValue((float)1., 3, 3);
				return l_pResArray;
			}
			System::Array^ l_pResArray = System::Array::CreateInstance(l_pEleType, nVals);
			for (int i = 0; i < nVals; i++)
			{
				l_pResArray->SetValue(data[i], i);
			}
			return l_pResArray;
		}
	}

	if (is_dotNetMXSValue(val) && type == System::Object::typeid)
	{
		DotNetMXSValue *l_wrapper = ((dotNetMXSValue*)val)->m_pDotNetMXSValue;
		return l_wrapper->GetWeakProxy();
	}
	else if (is_dotNetObject(val) || is_dotNetControl(val))
	{
		DotNetObjectWrapper* l_wrapper = NULL;
		if (is_dotNetObject(val)) 
		{
			l_wrapper = ((dotNetObject*)val)->GetDotNetObjectWrapper();
		}
		else 
		{
			l_wrapper = ((dotNetControl*)val)->GetDotNetObjectWrapper();
		}
		if (l_wrapper)
		{
			System::Object ^ l_pObj = l_wrapper->GetObject();
			if (l_pObj && type->IsAssignableFrom(l_pObj->GetType()))
			{
				return l_pObj;
			}
		}
	}
	else if (is_dotNetClass(val))
	{
		DotNetObjectWrapper *l_wrapper = ((dotNetClass*)val)->GetDotNetObjectWrapper();
		System::Type ^l_pTheType = l_wrapper->GetType();
		if (l_pTheType && type->IsAssignableFrom(l_pTheType->GetType()))
		{
			return l_pTheType;
		}
	}
	else if (type == System::Byte::typeid)
	{
		System::Byte^ res = gcnew System::Byte(val->to_int());
		return res;
	}
	else if	(type == System::SByte::typeid)
	{
		System::SByte^ res = gcnew System::SByte(val->to_int());
		return res;
	}
	else if (type == System::Decimal::typeid)
	{
		System::Decimal^ res = gcnew System::Decimal(val->to_float());
		return res;
	}
	else if (type == System::Int16::typeid)
	{
		System::Int16^ res = gcnew System::Int16(val->to_int());
		return res;
	}
	else if (type == System::UInt16::typeid)
	{
		System::UInt16^ res = gcnew System::UInt16(val->to_int());
		return res;
	}
	else if (type == System::Int32::typeid)
	{
		return val->to_int();
	}
	else if (type == System::UInt32::typeid)
	{
		System::UInt32^ res = gcnew System::UInt32(val->to_int());
		return res;
	}
	else if (type == System::Int64::typeid)
	{	
		return val->to_int64();
	}
	else if (type == System::UInt64::typeid)
	{
		System::UInt64^ res = gcnew System::UInt64(val->to_int());
		return res;
	}
	else if (type == System::IntPtr::typeid)
	{
		System::IntPtr^ res = gcnew System::IntPtr(val->to_intptr());
		return res;
	}
	else if (type == System::UIntPtr::typeid)
	{
		System::UIntPtr^ res = gcnew System::UIntPtr((UINT_PTR)val->to_intptr());
		return res;
	}
	else if (type == System::Single::typeid)
	{
		return val->to_float();
	}
	else if (type == System::Double::typeid)
	{
		return  val->to_double();
	}
	else if (type == System::Boolean::typeid)
	{
		return (val->to_bool() != FALSE);
	}
	else if (type == System::String::typeid)
	{
		const MCHAR* string = val->to_string();
		return System::String::Intern(gcnew System::String(string));
	}
	else if (l_pOrigType->IsArray && type == System::Char::typeid->MakeArrayType(l_pOrigType->GetArrayRank()))
	{
		CString cString(val->to_string());
		int len = cString.GetLength();
		array<System::Char> ^l_pCharArray = gcnew array<System::Char>(len);
		for (int i = 0; i < len; i++)
		{
			l_pCharArray[i] = cString[i];
		}
		return l_pCharArray;
	}
	else if (type == System::Char::typeid)
	{
		System::String^ myString = gcnew System::String(val->to_string());
		if (myString->Length != 0)
		{
			System::Char^ mychar = myString[0]; // This just expects one character, not an array of them
			return mychar;
		}
		else
		{
			throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_CHAR_ZERO_LEN_STRING));
		}
	}
	else if (val == &undefined)
	{
		return nullptr;
	}
	else if (type == System::Object::typeid)
	{
		if (is_array(val))
		{
			Array *theArray = (Array*)val;
			array<System::Object^> ^resArray = gcnew array<System::Object^>(theArray->size);
			for (int i = 0; i < theArray->size; i++)
			{
				resArray[i] = MXS_Value_To_Object(theArray->data[i], type, parameterName, backingFPValues);
			}
			return resArray;
		}
		else if (is_integer(val))
		{
			return val->to_int();
		}
		else if (is_integerptr(val))
		{
			return val->to_intptr();
		}
		else if (is_integer64(val)) 
		{
			return val->to_int64();
		}
		else if (is_float(val))
		{
			return val->to_float();
		}
		else if (is_double(val))
		{
			return  val->to_double();
		}
		else if (is_bool(val))
		{
			return (val == &true_value);
		}
		else if (is_string(val))
		{
			const MCHAR* string = val->to_string();
			return gcnew System::String(string);
		}
	}

	System::String ^ l_pTypeName = type->FullName;
	if (!l_pTypeName)
		l_pTypeName = type->Name;
	throw ConversionError(val, MNETStr::ToMSTR(l_pTypeName));
}

Value* MXS_dotNet::Object_To_MXS_Value(System::Object^ val)
{
	if (!val)
		return &undefined;
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);

	System::Array ^ l_pValArray = dynamic_cast<System::Array^>(val);
	if (l_pValArray)
	{
		one_typed_value_local_tls(Array* result);
		System::Type^ l_pValType = l_pValArray->GetType();
		int l_rank = l_pValArray->Rank;

		if (l_rank > 1)
		{
			// do an n-dimensional copy from a n-dimensional source array to a ragged target array
			array<int> ^ limits = gcnew array<int>(l_rank); // # elements for each dimension in source
			array<int> ^ indices = gcnew array<int>(l_rank);
			for (int i = 0; i < l_rank; i++)
			{
				limits[i] = l_pValArray->GetLength(i);
				indices[i] = 0;
			}
			// fun stuff here!  Key to understanding is that the data values are always stored
			// in the final leaf arrays of the target.
			// For a 2x3 array will copy values in the following order:
			// [0,0],[0,1],[0,2],[1,0],[1,1],[1,2]
			// All the mxs Array instances we create will be held at some level under top level result Array,
			// so that is the only Array instance we need to gc protect.
			vl.result = new Array(limits[0]);
			// need to keep track of the current mxs Array instance for each dimension
			array<Array*> ^ arrays = gcnew array<Array*>(l_rank);
			arrays[0] = vl.result;
			for (int i = 1; i < l_rank; i++)
				arrays[i] = nullptr;

			int l_rank_m1 = l_rank - 1;
			// so from the notes above, data values will be stored in Array held by arrays[l_rank_m1]
			while (true)
			{
				// if final array is null, need to create new Array instances at one or more levels
				// where each Array instance is a member of the Array for the previous level.
				if (!arrays[l_rank_m1])
				{
					for (int i = 1; i < l_rank; i++)
					{
						if (!arrays[i])
						{
							arrays[i] = new Array(limits[i]);
							arrays[i - 1]->append(arrays[i]);
						}
					}
				}
				// and put the value in the final array
				System::Object^ l_pEle = l_pValArray->GetValue(indices);
				arrays[l_rank_m1]->append(MXS_dotNet::Object_To_MXS_Value(l_pEle));
				// bump the index value. If > limit set to 0, clear this array, pop
				// out one level, bump its index value. Repeat as necessary
				indices[l_rank_m1]++;
				int l_index = l_rank_m1;
				while ((l_index >= 0) && (indices[l_index] >= limits[l_index]))
				{
					indices[l_index] = 0;
					arrays[l_index] = nullptr;
					l_index--;
					// if l_index is -1, we have popped out through all the levels and we are done!
					if (l_index == -1)
						return_value_tls(vl.result);
					indices[l_index]++;
				}
			}
		}
		// simplified handling of 1 dimensional array
		vl.result = new Array(l_pValArray->Length);
		for (int i = 0; i < l_pValArray->Length; i++)
		{
			System::Object ^ l_pEle = l_pValArray->GetValue(i);
			vl.result->append(MXS_dotNet::Object_To_MXS_Value(l_pEle));
		}
		return_value_tls(vl.result);

	}

	one_value_local_tls(result);
	System::Type ^ type = val->GetType();
	if (type == System::Boolean::typeid)
	{
		vl.result = bool_result(System::Convert::ToBoolean(val));
	}
	else if (type == System::Byte::typeid || type == System::SByte::typeid || 
		type == System::Decimal::typeid || 
		type == System::Int16::typeid || type == System::UInt16::typeid || 
		type == System::Int32::typeid || type == System::UInt32::typeid )
	{
		vl.result = Integer::intern(System::Convert::ToInt32(val));
	}
	else if (type == System::Int64::typeid || type == System::UInt64::typeid ) 
	{
		vl.result = Integer64::intern(System::Convert::ToInt64(val));
	}
	else if (type == System::IntPtr::typeid || type == System::UIntPtr::typeid ) 
	{
		void *ptr = safe_cast<System::IntPtr^>(val)->ToPointer();
		vl.result = IntegerPtr::intern((INT_PTR)ptr);
	}
	else if (type == System::Single::typeid ) 
	{
		vl.result = Float::intern(System::Convert::ToSingle(val));
	}
	else if (type == System::Double::typeid ) 
	{
		vl.result = Double::intern(System::Convert::ToDouble(val));
	}
	else if (type == System::String::typeid ) 
	{
		MSTR str(MNETStr::ToMSTR(System::Convert::ToString(val)));
		vl.result = new String(str);
	}
	else if (type == System::Char::typeid ) 
	{
		MSTR c(MNETStr::ToMSTR(System::Convert::ToChar(val).ToString()));
		vl.result = new String(c);
	}
	else if (type == DotNetMXSValue::DotNetMXSValue_proxy::typeid )
	{
		DotNetMXSValue::DotNetMXSValue_proxy ^ l_pProxy = safe_cast<DotNetMXSValue::DotNetMXSValue_proxy^>(val);
		return l_pProxy->get_value();
	}
	else
	{
		// TODO:: very special case!  For debugging of MaxPlus, want to handle class Autodesk.Max.MaxPlus.SWIGTYPE_p_WStr.. Now how??? ....
		vl.result = DotNetObjectWrapper::intern(val);
	}
	return_value_tls(vl.result);
}

Value* MXS_dotNet::DotNetObjectValueToMXSValue(dotNetBase *obj)
{
	if (!obj)
		return &undefined;
	try
	{
		return_value(Object_To_MXS_Value(obj->GetDotNetObjectWrapper()->GetObject()));
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_DOTNET_RUNTIME_EXCEPTION), MNETStr::ToMSTR(e));
	}
}

Value* MXS_dotNet::DotNetArrayToMXSValue(dotNetBase *obj, Value* type)
{
	if (!obj)
		return &undefined;
	try
	{
		MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
		one_value_local_tls(res);
		System::Type ^ l_ptype = obj->GetDotNetObjectWrapper()->GetType();
		System::Type^  l_pEleType = l_ptype->GetElementType();
		int l_rank = l_ptype->GetArrayRank();
		MSTR l_pTypeString = MXS_dotNet::MNETStr::ToMSTR(l_ptype->FullName);
		if (l_ptype->IsArray && (l_pEleType == System::Single::typeid || l_pEleType == System::Double::typeid))
		{
			System::Object ^ the_obj = obj->GetDotNetObjectWrapper()->GetObject();
			System::Array ^ l_pValArray = dynamic_cast<System::Array^>(the_obj);

			if (type == &Matrix3Value_class)
			{
				if ((l_rank == 2) && (l_pValArray->GetLength(0) == 4) && (l_pValArray->GetLength(1) == 4))
				{
					vl.res = new Matrix3Value(TRUE);
					MRow *pM = ((Matrix3Value*)vl.res)->m.GetAddr();
					for (int i = 0; i < 4; i++)
						for (int j = 0; j < 3; j++)
							pM[i][j] = System::Convert::ToSingle(l_pValArray->GetValue(j, i));
				}
				else
					throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_ARRAY44), l_pTypeString);
			}
			else if (type == &Point2Value_class)
			{
				if ((l_rank == 1) && (l_pValArray->GetLength(0) == 2))
				{
					Point2 p;
					float* data = p;
					for (int j = 0; j < 2; j++)
						data[j] = System::Convert::ToSingle(l_pValArray->GetValue(j));
					vl.res = new Point2Value(p);
				}
				else
					throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_ARRAY2), l_pTypeString);
			}
			else if (type == &Point3Value_class)
			{
				if ((l_rank == 1) && (l_pValArray->GetLength(0) == 3))
				{
					Point3 p;
					float* data = p;
					for (int j = 0; j < 3; j++)
						data[j] = System::Convert::ToSingle(l_pValArray->GetValue(j));
					vl.res = new Point3Value(p);
				}
				else
					throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_ARRAY3), l_pTypeString);
			}
			else if (type == &Point4Value_class)
			{
				if ((l_rank == 1) && (l_pValArray->GetLength(0) == 4))
				{
					Point4 p;
					float* data = p;
					for (int j = 0; j < 4; j++)
						data[j] = System::Convert::ToSingle(l_pValArray->GetValue(j));
					vl.res = new Point4Value(p);
				}
				else
					throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_ARRAY4), l_pTypeString);
			}
			else if (type == &QuatValue_class)
			{
				if ((l_rank == 1) && (l_pValArray->GetLength(0) == 4))
				{
					Quat q;
					float* data = q;
					for (int j = 0; j < 4; j++)
						data[j] = System::Convert::ToSingle(l_pValArray->GetValue(j));
					vl.res = new QuatValue(q);
				}
				else
					throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_ARRAY4), l_pTypeString);
			}
			else if (type == &EulerAnglesValue_class)
			{
				if ((l_rank == 1) && (l_pValArray->GetLength(0) == 3))
				{
					float data[3];
					for (int j = 0; j < 3; j++)
						data[j] = System::Convert::ToSingle(l_pValArray->GetValue(j));
					vl.res = new EulerAnglesValue(data[0], data[1], data[2]);
				}
				else
					throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_ARRAY3), l_pTypeString);
			}
			else
				throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_ARRAY_CONV_TYPE), type);

			return_value_tls(vl.res);
		}
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_ARRAY), l_pTypeString);
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_DOTNET_RUNTIME_EXCEPTION), MNETStr::ToMSTR(e));
	}
}

System::Type^ MXS_dotNet::ResolveTypeFromName(const MCHAR* pTypeString)
{
	using namespace System;
	using namespace System::Reflection;
	using namespace System::Diagnostics;

	TSTR typeString = MXS_dotNet::BuiltInTypeAlias::Convert (pTypeString);

	// look for type using fully qualified class name (includes assembly)
	System::String ^l_ObjectTypeString = gcnew System::String(typeString);
	Type ^l_ObjectTypeRes;

	// TODO: need to add method for specifying additional name spaces to automatically search.
	// for right now, just hacking in hard coded list
	array<System::String^>^autoNameSpaces = gcnew array<System::String^>{_T(""), _T("System."), _T("System.Windows.Forms.")};

	for (int i = 0; (!l_ObjectTypeRes) && i < autoNameSpaces->Length; i++)
	{
		System::String ^ l_pTypeString = autoNameSpaces[i] + l_ObjectTypeString;
		l_ObjectTypeRes = Type::GetType(l_pTypeString, false, true); // no throw, case insensitive
		if (l_ObjectTypeRes && (l_ObjectTypeRes->IsPublic || l_ObjectTypeRes->IsNestedPublic))
			break;
	}

	if (!l_ObjectTypeRes)
	{
		// look for in each assembly
		AppDomain^ currentDomain = AppDomain::CurrentDomain;
		//Make an array for the list of assemblies.
		array<Assembly^>^l_pAssemblies = currentDomain->GetAssemblies();

		//List the assemblies in the current application domain.
//		Debug::WriteLine( "List of assemblies loaded in current appdomain:" );
		for (int i = 0; (!l_ObjectTypeRes) && i < autoNameSpaces->Length; i++)
		{
			System::String ^ l_pTypeString = autoNameSpaces[i] + l_ObjectTypeString;
			for (int j = 0; j < l_pAssemblies->Length; j++)
			{
				Assembly^ l_pAssembly = l_pAssemblies[j];

//				array<Type^>^types = assem->GetTypes();
//				Debug::WriteLine( assem );
//				Debug::WriteLine( types->Length );

				l_ObjectTypeRes = l_pAssembly->GetType(l_pTypeString, false, true);
				if (l_ObjectTypeRes && (l_ObjectTypeRes->IsPublic || l_ObjectTypeRes->IsNestedPublic))
					break;
			}
		}
	}

	if (l_ObjectTypeRes)
	{
		if (!ManagedServices::MaxscriptSDK::IsTypeWhitelistedForSecureMode(l_ObjectTypeRes, false))
			return nullptr;
	}
	return l_ObjectTypeRes;
}

void MXS_dotNet::PrepArgList(array<System::Reflection::ParameterInfo^> ^candidateParamList, 
							 Value** arg_list, array<System::Object^> ^argArray, int count,
							 Tab<FPValue*> &backingFPValues)
{
	using namespace System::Reflection;
	for (int i = 0; i < count; i++)
	{
		// if the param is a [out] param, can just pass a null pointer.
		ParameterInfo ^ l_pParameterInfo = candidateParamList[i];
		if (l_pParameterInfo->IsOut)
			argArray[i] = nullptr;
		else
		{
			System::Type ^ l_pType = l_pParameterInfo->ParameterType;
			System::String ^ l_pName = l_pParameterInfo->Name;
			Value *val = arg_list[i];
			argArray[i] = MXS_dotNet::MXS_Value_To_Object(val, l_pType, l_pName, backingFPValues);
		}
	}
}

bool MXS_dotNet::isCompatible(array<System::Reflection::ParameterInfo^>^ candidateParamLists,
						Value**   arg_list, 
						int       count)
{
	using namespace System::Reflection;
	
	bool result = true;
	
	// Iterate through all the parameters of the .NET type
	for (int i = 0; i < candidateParamLists->Length; i++ )
	{
		ParameterInfo^ l_pParameterInfo = candidateParamLists[i];
		System::Type^ l_pType   = l_pParameterInfo->ParameterType;
		System::String ^ l_pName = l_pParameterInfo->Name;
		
		// Check if the Value type and the Type^ are compatible types
		int score = CalcParamScore(arg_list[i], l_pType, l_pName);
		if (score == kParamScore_NoMatch)
		{
			result = false;
			break;
		}
	}

	return result;
}


int MXS_dotNet::FindMethodAndPrepArgList(System::Collections::Generic::List<array<System::Reflection::ParameterInfo^>^> ^candidateParamLists, 
										 Value** arg_list, array<System::Object^> ^argArray, int count, Array* userSpecifiedParamTypesVals,
										 Tab<FPValue*> &backingFPValues)
{
	using namespace System::Reflection;

	if (userSpecifiedParamTypesVals)
	{
		// convert mxs values to Type.
		array<System::Type^> ^l_pUserSpecifiedParamTypes = gcnew array<System::Type^>(count);
		for (int i = 0; i < count; i++)
		{
			Value *val = userSpecifiedParamTypesVals->data[i];
			System::Type ^l_pType;
			if (is_dotNetObject(val) || is_dotNetControl(val) || is_dotNetClass(val))
			{
				DotNetObjectWrapper *wrapper = NULL;
				if (is_dotNetObject(val)) wrapper = ((dotNetObject*)val)->GetDotNetObjectWrapper();
				else if (is_dotNetControl(val)) wrapper = ((dotNetControl*)val)->GetDotNetObjectWrapper();
				else if (is_dotNetClass(val)) wrapper = ((dotNetClass*)val)->GetDotNetObjectWrapper();
				if (wrapper)
					l_pType = wrapper->GetType();
			}
			if (l_pType)
				l_pUserSpecifiedParamTypes[i] = l_pType;
			else
				throw ConversionError(val, _M("System::Type"));
		}
		// if we are here, userSpecifiedParamTypes now contains the Types. Find candidate ParamList that matches, if any 
		for (int j = 0; j < candidateParamLists->Count; j++)
		{
			bool match = true;
			array<ParameterInfo^> ^l_pCandidateParamList = candidateParamLists[j];
			for (int i = 0; i < count; i++)
			{
				if (l_pUserSpecifiedParamTypes[i] != l_pCandidateParamList[i]->ParameterType)
				{
					match = false;
					break;
				}
			}
			if (match)
			{
				// we have a match! try convert mxs values to .net values
				PrepArgList(l_pCandidateParamList, arg_list, argArray, count, backingFPValues);
				// if we got here, the values converted ok. Return the index of the matching candidate
				return j;
			}
		}
		// alas, no match. Return -1, caller can throw exception with good message
		return -1;
	}
	// user didn't specify ParamTypes, so we need to figure out the best match. For each
	// candidate, calculate a score based on matching each parameter type with the MXS value
	// type. If for any parameter the mxs value and type aren't compatible, rule out that candidate.
	int bestMatchIndex = -1;
	int bestMatchScore = -1;
	for (int j = 0; j < candidateParamLists->Count; j++)
	{
		int myScore = 0;
		array<ParameterInfo^> ^l_pCandidateParamList = candidateParamLists[j];
		for (int i = 0; i < count; i++)
		{
			ParameterInfo^ l_pParameterInfo = l_pCandidateParamList[i];
			int paramScore;
			if (l_pParameterInfo->IsOut)
				paramScore = kParamScore_ExactMatch;
			else
				paramScore = CalcParamScore(arg_list[i], l_pParameterInfo->ParameterType, l_pParameterInfo->Name);
			if (paramScore == kParamScore_NoMatch)
			{
				myScore = -1;
				break;
			}
			myScore += paramScore;
		}
		if (myScore > bestMatchScore)
		{
			bestMatchIndex = j;
			bestMatchScore = myScore;
		}
	}
	if (bestMatchIndex >= 0)
	{
		array<ParameterInfo^>^ l_pCandidateParamList = candidateParamLists[bestMatchIndex];
		// we have a match! try convert mxs values to .net values
		PrepArgList(l_pCandidateParamList, arg_list, argArray, count, backingFPValues);
	}

	return bestMatchIndex;
}

int MXS_dotNet::CalcParamScore(Value *val, System::Type^ type, System::String ^ parameterName)
{
	// calculate a score based on how well val's type matches the desired type.
	// using score values of 9, 6, 3, 0. This will allow easy tweaking in the future
	// a score of 0 means no match at all.
	if (type == System::Object::typeid)
	{
		return kParamScore_NoMatch+1; // lowest on totem pole, barely better than nothing....
	}
	if (type->IsArray && is_array(val))
	{
		System::Type^ l_pEleType = type->GetElementType();
		Array* theArray = (Array*)val;
		if (theArray->size == 0)
			return kParamScore_ExactMatch;
		int arrayScore = 1; // Now initialize to 1 since the outer array types match.
		for (int i = 0; i < theArray->size; i++)
		{
			arrayScore += MXS_dotNet::CalcParamScore(theArray->data[i], l_pEleType, parameterName);
		}
		arrayScore /= theArray->size;
		return arrayScore;
	}

	// for 'byref' and 'byptr' types, get the underlying type. But don't lose whether we are
	// dealing with an Array type.
	System::Type ^ l_pOrigType = type;
	if (type->HasElementType)
	{
		type = type->GetElementType();
		if (l_pOrigType->IsArray)
		{
			type = type->MakeArrayType(l_pOrigType->GetArrayRank());
		}
	}

	// exclude if arg is a pointer to POD. We don't have a way to create such a value (other than NULL)
	if (l_pOrigType->IsPointer && type->IsPrimitive && val != &undefined)
		return kParamScore_NoMatch;

	// can we exclude all pointer types? If you hit this DbgAssert let Larry Minton know...
	DbgAssert (!l_pOrigType->IsPointer);

	// see comments in MXS_Value_To_Object as to what the following is for and doing....
	// handle maxplus interface - type will be IntPtr, need parameterName to determine its class
	if (type == System::IntPtr::typeid && parameterName != nullptr && parameterName->EndsWith(MaxPlusArgNameParsingConstants::k_pointerString))
	{
		System::String ^ classname = parameterName->Substring(0, parameterName->Length-MaxPlusArgNameParsingConstants::k_pointerStringLen);
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_fpvalueClassString))
		{
			FPValue* fpValue = new FPValue;
			int res = kParamScore_NoMatch;
			try
			{
				val->to_fpvalue(*fpValue);
				res = kParamScore_ExactMatch;
			}
			finally
			{
				delete fpValue;
			}
			return res;
		}

		if (val == &undefined)
		{
			if (classname->Equals(MaxPlusArgNameParsingConstants::k_referencemakerClassString) || 
				classname->Equals(MaxPlusArgNameParsingConstants::k_referencetargetClassString)  || 
				classname->Equals(MaxPlusArgNameParsingConstants::k_mtlbaseClassString) || 
				classname->Equals(MaxPlusArgNameParsingConstants::k_mtlClassString) || 
				classname->Equals(MaxPlusArgNameParsingConstants::k_texmapClassString) ||
				classname->Equals(MaxPlusArgNameParsingConstants::k_meshClassString) ||
				classname->Equals(MaxPlusArgNameParsingConstants::k_beziershapeClassString) ||
				classname->Equals(MaxPlusArgNameParsingConstants::k_inodeClassString)
				)
			{
				return kParamScore_ExactMatch;
			}
		}

		if (classname->Equals(MaxPlusArgNameParsingConstants::k_meshClassString))
			return is_mesh(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_beziershapeClassString))
			return is_beziershape(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_inodeClassString))
			return is_node(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_matrix3ClassString))
			return is_matrix3(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_point3ClassString))
			return is_point3(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_wstrClassString))
			return (is_name(val) || is_string(val)) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_point4ClassString))
			return is_point4(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_point2ClassString))
			return is_point2(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_rayClassString))
			return is_ray(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_quatClassString))
			return is_quat(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_angaxisClassString))
			return is_angaxis(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_bitarrayClassString))
			return is_bitarray(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_intervalClassString))
			return is_interval(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_acolorClassString))
			return is_color(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_colorClassString))
			return is_color(val) ? kParamScore_ExactMatch-1 : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_referencemakerClassString))
			return val->derives_from_MAXWrapper() ? kParamScore_GoodMatch-1 : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_referencetargetClassString))
			return val->derives_from_MAXWrapper() ? kParamScore_GoodMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_mtlbaseClassString))
			return (val->is_kind_of(class_tag(MAXMaterial)) || val->is_kind_of(class_tag(MAXTexture))) ? kParamScore_GoodMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_mtlClassString))
			return val->is_kind_of(class_tag(MAXMaterial)) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_texmapClassString))
			return val->is_kind_of(class_tag(MAXTexture)) ? kParamScore_ExactMatch : kParamScore_NoMatch;
		if (classname->Equals(MaxPlusArgNameParsingConstants::k_box3ClassString))
			return is_box3(val) ? kParamScore_ExactMatch : kParamScore_NoMatch;
	}

	if (is_dotNetObject(val) || is_dotNetControl(val) )
	{
		DotNetObjectWrapper* l_wrapper = NULL;
		if (is_dotNetObject(val)) 
			l_wrapper = ((dotNetObject*)val)->GetDotNetObjectWrapper();
		else 
			l_wrapper = ((dotNetControl*)val)->GetDotNetObjectWrapper();
		if (!l_wrapper)
			throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_EMPTY_OBJECT));

		// if target type is System::Object, we have a match
		if (type == System::Object::typeid)
			return kParamScore_ExactMatch;

		System::Object^ l_pTheObj = l_wrapper->GetObject();
		// see if object's type is assignable to the target type
		if (l_pTheObj)
		{
			System::Type^ l_pTheObjType = l_pTheObj->GetType();
			if (type->IsAssignableFrom(l_pTheObjType))
				return kParamScore_ExactMatch;
			else
				return kParamScore_NoMatch; // no match
		}
		else if (type->IsPointer || type->IsArray || type->IsInterface)
			return kParamScore_ExactMatch; // assuming a nullptr is ok for these. Valid? Any more?
		else
			return kParamScore_NoMatch;
	}
	if (is_dotNetClass(val))
	{
		// if target type is System::Object, we have a match
		if (type == System::Object::typeid)
			return kParamScore_ExactMatch;
		DotNetObjectWrapper* l_wrapper = ((dotNetClass*)val)->GetDotNetObjectWrapper();
		System::Type^ l_pTheType = l_wrapper->GetType();
		// see if the type is assignable to the target type
		if (l_pTheType && type->IsAssignableFrom(l_pTheType->GetType()))
			return kParamScore_ExactMatch;
		else
			return kParamScore_NoMatch;
	}
	if (is_bool(val))
	{
		if (type == System::Boolean::typeid)
			return kParamScore_ExactMatch;
		else
			return kParamScore_NoMatch;
	}
	if (is_string(val) || is_name(val) || is_stringstream(val))
	{
		const TCHAR* str = nullptr;
		if (is_string(val) || is_name(val))
			str = val->to_string();
		else
			str = ((StringStream*)val)->content_string;
		size_t str_len = str ? _tcslen(str) : 0;
		if (type == System::String::typeid) 
			return (str_len == 1) ? kParamScore_ExactMatch -1: kParamScore_ExactMatch;
		else if (type == System::Char::typeid) 
			return (str_len == 1) ? kParamScore_ExactMatch : kParamScore_ExactMatch -2;
		else if (l_pOrigType->IsArray && type == System::Char::typeid->MakeArrayType(l_pOrigType->GetArrayRank()) )
			return (str_len == 1) ? kParamScore_ExactMatch -2 : kParamScore_ExactMatch -1;
		else
			return kParamScore_NoMatch;
	}
	if (val == &undefined)
	{
		if (type->IsPointer || type->IsArray || type->IsInterface)
			return kParamScore_ExactMatch; // assuming a nullptr is ok for these. Valid? Any more?
		else
			return kParamScore_NoMatch;
	}
	if (is_int(val))
	{
		if (type == System::Byte::typeid || type == System::SByte::typeid || 
			type == System::Decimal::typeid || 
			type == System::Int16::typeid || type == System::UInt16::typeid || 
			type == System::Int32::typeid || type == System::UInt32::typeid)
			return kParamScore_ExactMatch;
		else if (type == System::IntPtr::typeid || type == System::UIntPtr::typeid)
			return kParamScore_GoodMatch;
		else if (type == System::Int64::typeid || type == System::UInt64::typeid) 
			return kParamScore_GoodMatch;
		else if (type == System::Single::typeid || type == System::Double::typeid)
			return kParamScore_PoorMatch;
		else
			return kParamScore_NoMatch;
	}
	if (is_int64(val))
	{
		if (type == System::Byte::typeid || type == System::SByte::typeid || 
			type == System::Int16::typeid || type == System::UInt16::typeid || 
			type == System::Int32::typeid || type == System::UInt32::typeid)
			return kParamScore_GoodMatch;
		else if (type == System::IntPtr::typeid || type == System::UIntPtr::typeid)
			return kParamScore_GoodMatch;
		else if (type == System::Int64::typeid || type == System::UInt64::typeid ||
			type == System::Decimal::typeid)
			return kParamScore_ExactMatch;
		else if (type == System::Single::typeid || type == System::Double::typeid)
			return kParamScore_PoorMatch;
		else
			return kParamScore_NoMatch;
	}
	if (is_intptr(val))
	{
		if (type == System::Byte::typeid || type == System::SByte::typeid || 
			type == System::Decimal::typeid || 
			type == System::Int16::typeid || type == System::UInt16::typeid || 
			type == System::Int32::typeid || type == System::UInt32::typeid)
			return kParamScore_GoodMatch;
		else if (type == System::IntPtr::typeid || type == System::UIntPtr::typeid)
			return kParamScore_ExactMatch;
		else if (type == System::Int64::typeid || type == System::UInt64::typeid) 
			return kParamScore_GoodMatch;
		else if (type == System::Single::typeid || type == System::Double::typeid)
			return kParamScore_PoorMatch;
		else
			return kParamScore_NoMatch;
	}
	if (is_float(val))
	{
		if (type == System::Byte::typeid || type == System::SByte::typeid || 
			type == System::Decimal::typeid || 
			type == System::Int16::typeid || type == System::UInt16::typeid || 
			type == System::Int32::typeid || type == System::UInt32::typeid)
			return kParamScore_PoorMatch;
		else if (type == System::IntPtr::typeid || type == System::UIntPtr::typeid)
			return kParamScore_PoorMatch;
		else if (type == System::Int64::typeid || type == System::UInt64::typeid) 
			return kParamScore_PoorMatch;
		else if (type == System::Single::typeid)
			return kParamScore_ExactMatch;
		else if (type == System::Double::typeid)
			return kParamScore_GoodMatch;
		else
			return kParamScore_NoMatch;
	}
	if (is_double(val))
	{
		if (type == System::Byte::typeid || type == System::SByte::typeid || 
			type == System::Decimal::typeid || 
			type == System::Int16::typeid || type == System::UInt16::typeid || 
			type == System::Int32::typeid || type == System::UInt32::typeid)
			return kParamScore_PoorMatch;
		else if (type == System::IntPtr::typeid || type == System::UIntPtr::typeid)
			return kParamScore_PoorMatch;
		else if (type == System::Int64::typeid || type == System::UInt64::typeid) 
			return kParamScore_PoorMatch;
		else if (type == System::Single::typeid)
			return kParamScore_GoodMatch;
		else if (type == System::Double::typeid)
			return kParamScore_ExactMatch;
		else
			return kParamScore_NoMatch;
	}
	return kParamScore_NoMatch;
}

bool MXS_dotNet::CompareEnums(Value** arg_list, int count)
{
	try
	{
		System::UInt64 l_bitfield[] = {0, 0};

		for (int i = 0; i < 2; i++)
		{
			Value* val = arg_list[i];
			if (is_dotNetObject(val))
			{
				System::Object ^ l_pObj = ((dotNetObject*)val)->GetDotNetObjectWrapper()->GetObject();
				System::Type ^ l_pType_tmp = l_pObj->GetType();
				if (!l_pType_tmp->IsEnum)
					throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_DOTNET_OBJ_NOT_ENUM), val);
				l_bitfield[i] = System::Convert::ToUInt64(l_pObj);
			}
			else if (is_number(val))
			l_bitfield[i] = val->to_int64();

			else
				throw ConversionError(val, _M("dotNetObject"));
		}

		return (l_bitfield[0] & l_bitfield[1]) != 0L;
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_DOTNET_RUNTIME_EXCEPTION), MNETStr::ToMSTR(e));
	}
}

Value* MXS_dotNet::CombineEnums(Value** arg_list, int count)
{
	using namespace System::Reflection;

	try
	{
		if (count == 0)
			check_arg_count(dotNet.combineEnums, 1, count);

		System::UInt64 l_bitfield = 0;

		System::Type ^ l_pType;
		for (int i = 0; i < count; i++)
		{
			Value* val = arg_list[i];
			if (is_dotNetObject(val))
			{
				System::Object ^ l_pObj = ((dotNetObject*)val)->GetDotNetObjectWrapper()->GetObject();
				System::Type ^ l_pType_tmp = l_pObj->GetType();
				if (!l_pType_tmp->IsEnum)
					throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_DOTNET_OBJ_NOT_ENUM), val);
				if (!l_pType)
				{
					l_pType = l_pType_tmp;
					bool hasFlagsAttribute = false;
					// Iterate through all the Attributes for each method.
					System::Collections::IEnumerator^ myEnum1 = System::Attribute::GetCustomAttributes( l_pType )->GetEnumerator();
					while ( myEnum1->MoveNext() )
					{
						System::Attribute^ attr = safe_cast<System::Attribute^>(myEnum1->Current);

						// Check for the FlagsTypeAttribute attribute.
						if ( attr->GetType() == System::FlagsAttribute::typeid )
						{
							hasFlagsAttribute = true;
							break;
						}
					}

					if (!hasFlagsAttribute)
						throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_ENUM_NOT_BIT_FIELD), val);
				}
				if (l_pType != l_pType_tmp)
					throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_ENUMS_NOT_SAME_TYPE), val);
				l_bitfield += System::Convert::ToUInt64(l_pObj);

			}
			else if (is_number(val))
				l_bitfield += val->to_int64();

			else
				throw ConversionError(val, _M("dotNetObject"));
		}
		if (!l_pType)
			throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_ONE_ARG_MUST_BE_ENUM));
		System::Object ^ l_pRes = System::Enum::ToObject(l_pType, l_bitfield);
		return_value (DotNetObjectWrapper::intern(l_pRes));
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_DOTNET_RUNTIME_EXCEPTION), MNETStr::ToMSTR(e));
	}
}

Value* MXS_dotNet::LoadAssembly(const MCHAR* assyDll, bool returnPassFail)
{
	using namespace System::Reflection;

	try
	{
		System::String ^ l_pAssyDll = gcnew System::String(assyDll);

		if (!ManagedServices::MaxscriptSDK::IsAssemblyNameWhitelistedForSecureMode(l_pAssyDll))
		{
			throw RuntimeError(_T("Loading of assembly not available: "), assyDll);
		}

        // verify assembly digital signature only in secure mode
        if (ManagedServices::AppSDK::InSecureMode() &&
            !ManagedServices::AppSDK::HasValidDigitalSignature(l_pAssyDll))
        {
            throw RuntimeError(_T("Missing digital signature: "), assyDll);
        }
#pragma warning(disable:4947)
		Assembly ^ l_pAssy;
		try {l_pAssy = Assembly::LoadWithPartialName(l_pAssyDll);}
		catch (System::IO::FileLoadException ^) {}
		if (!l_pAssy)
		{
			int i = l_pAssyDll->LastIndexOf(_T(".dll"));
			if (i == (l_pAssyDll->Length - 4))
			{
				try {l_pAssy = Assembly::LoadWithPartialName(l_pAssyDll->Substring(0,i));}
				catch (System::IO::FileLoadException ^) {}
			}
		}
#pragma warning(default:4947)
		if (!l_pAssy)
		{
			int i = l_pAssyDll->LastIndexOf(_T(".dll"));
			if (i != (l_pAssyDll->Length - 4))
				l_pAssyDll = l_pAssyDll + _T(".dll");
			try {l_pAssy = Assembly::LoadFrom(l_pAssyDll);}
			catch (System::IO::FileNotFoundException ^) {}
			catch (System::BadImageFormatException ^) {}
		}
		if (!l_pAssy)
		{
			// look in the directory holding the dll for the WindowsForm Assembly
			Assembly ^mscorlibAssembly = System::String::typeid->Assembly;
			System::String ^ l_pAssyLoc = mscorlibAssembly->Location;
			int i = l_pAssyLoc->LastIndexOf(_T('\\'))+1;
			l_pAssyLoc = l_pAssyLoc->Substring(0,i);
			try {l_pAssy = Assembly::LoadFrom(l_pAssyLoc + l_pAssyDll);}
			catch (System::IO::FileNotFoundException ^) {}
			catch (System::BadImageFormatException ^) {}
		}
		if (returnPassFail)
		{
			return (l_pAssy) ? &true_value : &false_value;
		}
		return_value (DotNetObjectWrapper::intern(l_pAssy));

	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_DOTNET_RUNTIME_EXCEPTION), MNETStr::ToMSTR(e));
	}
}

// converts maxscript value to a dotNetObject value of type specified by argument 'type'
Value* MXS_dotNet::MXSValueToDotNetObjectValue(Value *val, dotNetBase *type)
{
	try
	{
		Value* result = &undefined;
		System::Type ^ the_type = type->GetDotNetObjectWrapper()->GetType();
		Tab<FPValue*> backingFPValues;
		System::Object ^ the_res = MXS_Value_To_Object(val, the_type, nullptr, backingFPValues);
		DbgAssert(backingFPValues.Count() == 0);
		if (the_res)
			result = DotNetObjectWrapper::intern(the_res);
		return result;
	}
	catch (MAXScriptException&)
	{
		throw;
	}
	catch (System::Exception ^ e)
	{
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_DOTNET_RUNTIME_EXCEPTION), MNETStr::ToMSTR(e));
	}
}

Value* MXS_dotNet::CreateIconFromMaxMultiResIcon(const MSTR& iconName, int width, int height, bool enabled, bool on)
{
	HICON hicon = MaxSDK::CreateHICONFromMaxMultiResIcon( iconName, width, height, enabled, on ); 
	if (hicon)
	{
		System::Drawing::Icon^ newIcon = System::Drawing::Icon::FromHandle( System::IntPtr(hicon) );
		MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
		return_value_tls(DotNetObjectWrapper::intern(newIcon));
	}
	else
		return &undefined;
}

void MXS_dotNet::BuiltInTypeAlias::Initialize()
{
	if (spBuiltInTypeMap == nullptr)
	{
		spBuiltInTypeMap = new std::unordered_map<std::wstring, const TCHAR*>;
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("bool"), _T("System.Boolean")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("byte"), _T("System.Byte")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("sbyte"), _T("System.SByte")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("char"), _T("System.Char")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("decimal"), _T("System.Decimal")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("double"), _T("System.Double")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("float"), _T("System.Single")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("int"), _T("System.Int32")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("uint"), _T("System.UInt32")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("long"), _T("System.Int64")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("ulong"), _T("System.UInt64")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("object"), _T("System.Object")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("short"), _T("System.Int16")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("ushort"), _T("System.UInt16")));
		spBuiltInTypeMap->insert(std::pair<std::wstring, const TCHAR*>(_T("string"), _T("System.String")));
	}
}

void MXS_dotNet::BuiltInTypeAlias::Release()
{
	delete spBuiltInTypeMap;
	spBuiltInTypeMap = nullptr;
}

TSTR MXS_dotNet::BuiltInTypeAlias::Convert(const MCHAR* pTypeString)
{
	TSTR typeString = pTypeString;
	TSTR arraySpec;
	Initialize();
	if (spBuiltInTypeMap)
	{
		int arraySpecIndex = typeString.first(_T('['));
		if (arraySpecIndex != -1)
		{
			arraySpec = &(typeString.data()[arraySpecIndex]);
			typeString.remove(arraySpecIndex);
		}
		auto it = spBuiltInTypeMap->find(typeString.data());
		if (it != spBuiltInTypeMap->end())
		{
			typeString = it->second;
		}
		typeString += arraySpec;
	}

	return typeString;
}
