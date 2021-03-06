
#include "stdafx.h"
#include "DefaultPropertyMaker.h"
#include "PropertyMaker.h"
#include "../Lib/DiaWrapper.h"
#include "../DataProperty.h"
#include "VisualizerDefine.h"


namespace visualizer
{
	using namespace sharedmemory;
	using namespace parser;

	CDataProperty *n_pProperty = NULL;

	// make property
	void		MakeProperty_Root(CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol, const int depth );

	bool		MakeProperty_Child(  CMFCPropertyGridProperty *pParentProp,  const SSymbolInfo &symbol, const int depth );

	void		MakeProperty_UDTChild(CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol, const int depth );

	void		MakeProperty_BaseClass(CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol, const int depth );

	void		MakeProperty_Pointer(CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol, const int depth );

	void		MakeProperty_Data(CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol, const int depth );

	void		MakeProperty_Array(CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol, const int depth );

	CMFCPropertyGridProperty* MakeProperty_BaseType(CMFCPropertyGridProperty *pParentProp, 
		const std::string valueName,  const SSymbolInfo &symbol );

	CMFCPropertyGridProperty* MakeProperty_PointerData(CMFCPropertyGridProperty *pParentProp, 
		const SSymbolInfo &symbol );

 	CMFCPropertyGridProperty* MakeProperty_ArrayData(CMFCPropertyGridProperty *pParentProp, 
 		const SSymbolInfo &symbol );

	CMFCPropertyGridProperty* MakeProperty_UDTData(CMFCPropertyGridProperty *pParentProp, 
		const SSymbolInfo &symbol );

	CMFCPropertyGridProperty* MakeProperty_BaseClassData(CMFCPropertyGridProperty *pParentProp, 
		const SSymbolInfo &symbol);

	void		AddProperty(CMFCPropertyGridProperty *pParentProp, 
		CMFCPropertyGridProperty *prop,  const SSymbolInfo *pSymbol, STypeData *pTypeData);

	_variant_t GetValue(IDiaSymbol *pSymbol, void *pMemPtr);

}

using namespace dia;
using namespace std;
using namespace visualizer;


//------------------------------------------------------------------------
// 기본형식으로 property창에 심볼을 출력한다.
// symbolName : 공유메모리에 저장된 심볼이름
//------------------------------------------------------------------------
bool visualizer::MakeProperty_DefaultForm( CDataProperty *pProperties,  const string &symbolName )
{
	const std::string str = sharedmemory::ParseObjectName(symbolName);

	IDiaSymbol *pSymbol = CDiaWrapper::Get()->FindType(str);
	RETV(!pSymbol, false);

	sharedmemory::SMemoryInfo memInfo;
	if (!sharedmemory::FindMemoryInfo(symbolName, memInfo))
	{
		pSymbol->Release();
		return false;
	}

	memInfo.name = symbolName;
	MakeProperty_DefaultForm(pProperties, NULL, SSymbolInfo(pSymbol, memInfo));
	return true;
}


//------------------------------------------------------------------------
// 
//------------------------------------------------------------------------
bool visualizer::MakeProperty_DefaultForm(  CDataProperty *pProperties, 
								 CMFCPropertyGridProperty *pParentProp,  const string &symbolName )
{

	return true;
}


//------------------------------------------------------------------------
// 
//------------------------------------------------------------------------
bool visualizer::MakeProperty_DefaultForm(  CDataProperty *pProperties,
				CMFCPropertyGridProperty *pParentProp,  
				 const SSymbolInfo &symbol )
{
	n_pProperty = pProperties;
	MakeProperty_Root(pParentProp, symbol, 2);
	return true;
}


//------------------------------------------------------------------------
//  symbol.pSym 의 자식을 pParentProp에 추가한다.
//------------------------------------------------------------------------
bool	 visualizer::MakePropertyChild_DefaultForm(  CDataProperty *pProperties, 
												   CMFCPropertyGridProperty *pParentProp,  const SSymbolInfo &symbol)
{
	n_pProperty = pProperties;
	const bool isVisualizerType = visualizer::MakeVisualizerProperty( pProperties, pParentProp, symbol );
	if (!isVisualizerType)
		MakeProperty_Child(pParentProp, symbol, 2);
	return true;
}


//------------------------------------------------------------------------
//  symbol을 pParentProp의 자식으로 추가한다.
//------------------------------------------------------------------------
bool	 visualizer::MakeProperty_Child(  CMFCPropertyGridProperty *pParentProp,  
									   const SSymbolInfo &symbol, const int depth )
{
	if (depth <= 0)
		return true;
	enum SymTagEnum symtag;
	if (S_OK != symbol.pSym->get_symTag((DWORD*)&symtag))
		return true;

	switch (symtag)
	{
	case SymTagArrayType:
		MakeProperty_Array(pParentProp, symbol, depth);
		break;

	case SymTagPointerType:
		MakeProperty_Pointer(pParentProp, symbol, depth);
		break;

	case SymTagBaseClass:
		MakeProperty_BaseClass(pParentProp, symbol, depth);
		break;

	case SymTagUDT:
		MakeProperty_UDTChild(pParentProp, symbol, depth);
		break;

	case SymTagData: 
		{
			IDiaSymbol *pBaseType;
			HRESULT hr = symbol.pSym->get_type(&pBaseType);
			ASSERT_RETV(S_OK == hr, false);
			MakeProperty_Child(pParentProp, SSymbolInfo(pBaseType,symbol.mem), depth);
		}
		break;

	case SymTagTypedef:
	case SymTagEnum:
	default:
		break;
	}

	return true;
}


//------------------------------------------------------------------------
// Property 생성
//------------------------------------------------------------------------
void visualizer::MakeProperty_Root(CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol, 
								   const int depth )
{
	if (depth <= 0)
		return;
	enum SymTagEnum symtag;
	if (S_OK != symbol.pSym->get_symTag((DWORD*)&symtag))
		return;

	CMFCPropertyGridProperty *pProp = NULL;
	switch (symtag)
	{
	case SymTagData: 
		MakeProperty_Data(pParentProp, symbol, depth);
		break;

	case SymTagArrayType:
		MakeProperty_Array(pParentProp, symbol, depth);
		break;

	case SymTagPointerType:
		MakeProperty_Pointer(pParentProp, symbol, depth);
		break;

	case SymTagBaseClass:
		pProp = MakeProperty_BaseClassData(pParentProp, symbol);
		if (pProp)
			MakeProperty_Child(pProp, symbol, depth-1);
		break;

	case SymTagUDT:
		pProp = MakeProperty_UDTData(pParentProp, symbol);
		if (pProp)
			MakeProperty_Child(pProp, symbol, depth);
		break;

	case SymTagTypedef:
		break;

	case SymTagEnum:
		break;

	default:
		break;
	}
}


//------------------------------------------------------------------------
// 기본 클래스 
//------------------------------------------------------------------------
void	visualizer ::MakeProperty_BaseClass(CMFCPropertyGridProperty *pParentProp, 
											const SSymbolInfo &symbol, const int depth )
{
	IDiaSymbol* pBaseType;
	HRESULT hr = symbol.pSym->get_type(&pBaseType);
	ASSERT_RET(hr == S_OK);

	LocationType locType;
	const LONG offset = dia::GetSymbolLocation(symbol.pSym, &locType);
	BYTE *ptr = (BYTE*)symbol.mem.ptr + offset;
	SMemoryInfo newMemInfo(symbol.mem.name.c_str(), ptr, 0);
	MakeProperty_Child(pParentProp, SSymbolInfo(pBaseType, newMemInfo), depth);
}

//------------------------------------------------------------------------
// baseclass type preview
//------------------------------------------------------------------------
CMFCPropertyGridProperty* visualizer::MakeProperty_BaseClassData(
	CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol)
{
	CMFCPropertyGridProperty *pProp = new CMFCPropertyGridProperty(
		common::str2wstr(symbol.mem.name).c_str());
	AddProperty(pParentProp, pProp, &symbol, &STypeData(SymTagBaseClass, VT_EMPTY, NULL));
	return pProp;
}


//------------------------------------------------------------------------
// User Define Type 
//------------------------------------------------------------------------
void visualizer ::MakeProperty_UDTChild(CMFCPropertyGridProperty *pParentProp, 
										const SSymbolInfo &symbol, const int depth)
{
	RET (!pParentProp);

	if (depth > 0)
	{
		CComPtr<IDiaEnumSymbols> pEnumChildren;
		if (SUCCEEDED(symbol.pSym->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren))) 
		{
			IDiaSymbol *pChild;
			ULONG celt = 0;
			while (SUCCEEDED(pEnumChildren->Next(1, &pChild, &celt)) && (celt == 1)) 
			{
				LONG offset = dia::GetSymbolLocation(pChild);
				SMemoryInfo memberMemInfo;
				memberMemInfo.name = dia::GetSymbolName(pChild);
				memberMemInfo.ptr = (BYTE*)symbol.mem.ptr + offset;
				MakeProperty_Root(pParentProp, SSymbolInfo(pChild, memberMemInfo), depth-1);
			}
		}
	}
	pParentProp->Expand(FALSE);
}


//------------------------------------------------------------------------
// Pointer 타입 출력 
//------------------------------------------------------------------------
void	visualizer ::MakeProperty_Pointer(CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol,
										  const int depth)
{
	IDiaSymbol* pBaseType;
	HRESULT hr = symbol.pSym->get_type(&pBaseType);
	ASSERT_RET(hr == S_OK);

	enum SymTagEnum baseSymTag;
	hr = pBaseType->get_symTag((DWORD*)&baseSymTag);

	void *srcPtr = (void*)*(DWORD*)symbol.mem.ptr;
	void *newPtr = sharedmemory::MemoryMapping(srcPtr);

	CMFCPropertyGridProperty *pProp = NULL;
	if (SymTagUDT == baseSymTag)
	{
		if (newPtr) 
		{
			SMemoryInfo ptrMemInfo(symbol.mem.name.c_str(), newPtr, (size_t)0);
			MakeProperty_Root(pParentProp, SSymbolInfo(pBaseType, ptrMemInfo, false), depth);
		}
	}
	pBaseType->Release();
}


//------------------------------------------------------------------------
// Property창에 Control을 추가한다.
// 변수 이름과 타입, 값을 설정한다.
//------------------------------------------------------------------------
void visualizer ::MakeProperty_Data(CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol,
									const int depth)
{
	IDiaSymbol* pBaseType;
	HRESULT hr = symbol.pSym->get_type(&pBaseType);
	ASSERT_RET(hr == S_OK);

	enum SymTagEnum baseSymTag;
	hr = pBaseType->get_symTag((DWORD*)&baseSymTag);
	ASSERT_RET(hr == S_OK);

	CMFCPropertyGridProperty *pProp;
	switch (baseSymTag)
	{
	case SymTagBaseType:
		{
			string valueTypeName = symbol.mem.name + " (" + dia::GetSymbolTypeName(symbol.pSym) + ")";
			MakeProperty_BaseType( pParentProp, valueTypeName, symbol);
		}
		break;

	case SymTagEnum:
		{
			std::string typeName = dia::GetSymbolTypeName(pBaseType);
			std::string valueTypeName =  symbol.mem.name + " (" +  typeName + ")";
			pProp = new CMFCPropertyGridProperty( common::str2wstr(valueTypeName).c_str() , _T(" ") ); 
			AddProperty(pParentProp, pProp, &symbol, &STypeData(baseSymTag, VT_UI4, symbol.mem.ptr));

			CComPtr<IDiaEnumSymbols> pEnumChildren;
			IDiaSymbol *pChild;
			if (SUCCEEDED(pBaseType->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren))) 
			{
				ULONG celt = 0;
				while (SUCCEEDED(pEnumChildren->Next(1, &pChild, &celt)) && (celt == 1)) 
				{
					std::string e_name = dia::GetSymbolName(pChild);
					pProp->AddOption( common::str2wstr(e_name).c_str() );
					pChild->Release();
				}
			}
			pProp->AllowEdit(FALSE);

			ULONGLONG length = 0;
			HRESULT hr = pBaseType->get_length(&length);
			_variant_t value = dia::GetValueFromAddress( symbol.mem.ptr, btUInt, length );
			SetPropertyValue( pProp, value);
		}
		break;

	case SymTagUDT:
		pProp = MakeProperty_UDTData(pParentProp, symbol);
		if (pProp)
			MakeProperty_Child(pProp, SSymbolInfo(pBaseType, symbol.mem), depth-1);
		break;

	case SymTagArrayType:
		pProp = MakeProperty_ArrayData(pParentProp, symbol);
		if (pProp)
			MakeProperty_Child(pProp, SSymbolInfo(pBaseType, symbol.mem), depth-1);
		break;

	case SymTagPointerType:
		pProp = MakeProperty_PointerData(pParentProp, symbol);
		if (pProp)
			MakeProperty_Child(pProp, SSymbolInfo(pBaseType, symbol.mem), depth-1);
		break;

	default:
		pBaseType->Release();
		break;
	}
}


//------------------------------------------------------------------------
// Pointer type Preview
//------------------------------------------------------------------------
CMFCPropertyGridProperty* visualizer::MakeProperty_ArrayData(CMFCPropertyGridProperty *pParentProp, 
												 const SSymbolInfo &symbol )
{
	CMFCPropertyGridProperty *pProp = NULL;

	CComPtr<IDiaSymbol> pArrayType;
	HRESULT hr = symbol.pSym->get_type(&pArrayType); // ArrayDataType
	ASSERT_RETV(S_OK == hr, pProp);

	CComPtr<IDiaSymbol> pElementType;	// 배열 개개의 타입
	hr = pArrayType->get_type(&pElementType);
	ASSERT_RETV(S_OK == hr, pProp);

	enum SymTagEnum elemSymTag;
	hr = pElementType->get_symTag((DWORD*)&elemSymTag);
	ASSERT(S_OK == hr);

	const string typeName = dia::GetSymbolTypeName(symbol.pSym);
	stringstream ss;

	if (//SymTagData == elemSymTag ||
		SymTagBaseType == elemSymTag) // BaseType Array
	{
		BasicType btype;
		hr = pElementType->get_baseType((DWORD*)&btype);
		ASSERT_RETV(S_OK == hr, pProp);

		// char*, char[] 타입이라면 스트링을 출력한다.
		if (btChar == btype)
			ss << symbol.mem.name << " {\"" << (char*)symbol.mem.ptr << "\"}";
		else
			ss << symbol.mem.name << " " << symbol.mem.ptr << " (" << typeName << ")";
	}
	else // UDT Array
	{
		ss << symbol.mem.name << " (" << typeName << ")";
	}

	pProp = new CMFCPropertyGridProperty( common::str2wstr(ss.str()).c_str() ); 
	AddProperty(pParentProp, pProp, &symbol,  &STypeData(SymTagArrayType, VT_EMPTY, NULL) );

	return pProp;
}


//------------------------------------------------------------------------
// Pointer Type Preview 
//------------------------------------------------------------------------
CMFCPropertyGridProperty* visualizer::MakeProperty_PointerData(
	CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol )
{
	CMFCPropertyGridProperty *pProp = NULL;

	CComPtr<IDiaSymbol> pPointerType;
	HRESULT hr = symbol.pSym->get_type(&pPointerType);
	ASSERT_RETV(hr == S_OK, pProp);  // PointerType

	CComPtr<IDiaSymbol> pBaseType;
	hr = pPointerType->get_type(&pBaseType);
	ASSERT_RETV(hr == S_OK, pProp);  // BasicDataType or UDTDataType

	void *srcPtr = (void*)*(DWORD*)symbol.mem.ptr;
	void *newPtr = sharedmemory::MemoryMapping(srcPtr);
	if (!newPtr)
		newPtr = srcPtr; // 공유메모리에 없는 데이타일경우 주소만 출력한다.

	enum SymTagEnum baseSymTag;
	hr = pBaseType->get_symTag((DWORD*)&baseSymTag);

	const string name = dia::GetSymbolName(symbol.pSym); // debug용
	const string typeName = dia::GetSymbolTypeName(symbol.pSym);
	stringstream ss;

	if (SymTagUDT == baseSymTag)
	{
		ss << symbol.mem.name << " 0x" << newPtr;
		ss << (char*)(sharedmemory::CheckValidAddress(newPtr)? " " : " not shared memory");
		ss << " (" << typeName << ")";
	}
	else if (SymTagBaseType == baseSymTag)
	{
		BasicType btype;
		hr = pBaseType->get_baseType((DWORD*)&btype);
		ASSERT_RETV(hr == S_OK, pProp);

		// char* 타입이라면 스트링을 출력한다.
		if (btChar == btype)
		{
			ss << symbol.mem.name << " 0x" << newPtr << " {\"";
			ss << (char*)(sharedmemory::CheckValidAddress(newPtr)? newPtr : " not shared memory")  << "\"}";
		}
	}

	if (ss.str().empty()) // default pointer 작업
	{
		ss << symbol.mem.name << " 0x" << newPtr;
		ss << (char*)(sharedmemory::CheckValidAddress(newPtr)? " " : " not shared memory");
		ss << " (" << typeName << ")";
	}

	pProp = new CMFCPropertyGridProperty( common::str2wstr(ss.str()).c_str() );
	AddProperty( pParentProp, pProp, &symbol, &STypeData(SymTagPointerType, VT_EMPTY, NULL));

	return pProp;
}


//------------------------------------------------------------------------
// UDT type Preview
//------------------------------------------------------------------------
CMFCPropertyGridProperty* visualizer::MakeProperty_UDTData(
	CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol )
{
	const string typeName = dia::GetSymbolTypeName(symbol.pSym);

	// 최상위 UDT가 아닐때만 타입을 출력한다.
	stringstream ss;
	ss << symbol.mem.name;
	if (pParentProp)
		ss << "  (" << typeName << ")";

	CMFCPropertyGridProperty *pProp = 
		new CMFCPropertyGridProperty( common::str2wstr(ss.str()).c_str() ); 
	AddProperty(pParentProp, pProp, &symbol, &STypeData(SymTagUDT, VT_EMPTY, symbol.mem.ptr));

	const bool isVisualizerType = visualizer::MakeVisualizerProperty( n_pProperty, 
		pProp, symbol.mem,  symbol.mem.name);

	return (isVisualizerType)? NULL : pProp;
}


//------------------------------------------------------------------------
// pSymbol : Array Type을 가리킨다. 
//------------------------------------------------------------------------
void visualizer ::MakeProperty_Array(CMFCPropertyGridProperty *pParentProp, const SSymbolInfo &symbol,
									 const int depth)
{
	ULONGLONG length=0;
	HRESULT hr = symbol.pSym->get_length(&length);
	ASSERT_RET(S_OK == hr);

	IDiaSymbol* pElementType;	// 배열 개개의 타입
	hr = symbol.pSym->get_type(&pElementType);
	ASSERT_RET(S_OK == hr);

	ULONGLONG element_length=0;
	hr = pElementType->get_length(&element_length);
	ASSERT(S_OK == hr);

	enum SymTagEnum elemSymTag;
	hr = pElementType->get_symTag((DWORD*)&elemSymTag);
	ASSERT(S_OK == hr);

	char valueName[ 128];
	basic_bufferstream<char> formatter(valueName, sizeof(valueName));

	if (//SymTagData == elemSymTag ||
		SymTagBaseType == elemSymTag)
	{
		for (ULONGLONG i=0; i < length; i += element_length)
		{
			formatter.seekp(0);
			formatter << "[" << i / element_length << "]" << std::ends;

			void *ptr = (BYTE*)symbol.mem.ptr + i;
			SMemoryInfo arrayElem(valueName, ptr, (size_t)element_length);
			MakeProperty_BaseType(pParentProp, valueName, SSymbolInfo(pElementType, arrayElem, false));
		}
		pParentProp->Expand(FALSE); // 일단 접어놓는다.
	}
	else // UDT Array
	{
		for (ULONGLONG i=0; i < length; i += element_length)
		{
			formatter.seekp(0);
			formatter << "[" << i / element_length << "]" << std::ends;

			void *ptr = (BYTE*)symbol.mem.ptr + i;
			SMemoryInfo arrayElem(valueName, ptr, (size_t)element_length);
			SSymbolInfo arraySymbol(pElementType, arrayElem, false);

 			CMFCPropertyGridProperty *pProp =
 				new CMFCPropertyGridProperty( common::str2wstr(valueName).c_str() );
			AddProperty( pParentProp, pProp, &arraySymbol, &STypeData(SymTagUDT,VT_EMPTY,NULL));

			MakeProperty_Root(pProp, arraySymbol, depth);
		}
		pParentProp->Expand(FALSE); // 일단 접어놓는다.
	}
	pElementType->Release();
}


//------------------------------------------------------------------------
// m_wndPropList 에 Row 를 추가한다.
// pSymbol 은 데이타를 가르키는 심볼이어야 한다.
//------------------------------------------------------------------------
CMFCPropertyGridProperty* visualizer ::MakeProperty_BaseType(
	CMFCPropertyGridProperty *pParentProp, const std::string valueName, 
	const SSymbolInfo &symbol )
{
	_variant_t value = dia::GetValueFromSymbol(symbol.mem.ptr, symbol.pSym);

	// static 변수는 프로세스끼리 공유할 수없으므로 0으로 출력한다.
	bool IsNotPrint = false;
	LocationType locType;
	dia::GetSymbolLocation(symbol.pSym, &locType);
	if (LocIsStatic == locType)
	{
		value = 0;
		IsNotPrint  = true;
	}

	CMFCPropertyGridProperty *pProp = NULL;
	switch (value.vt)
	{
	case VT_I2:
	case VT_I4:
	case VT_R4:
	case VT_R8:
	case VT_BOOL:
	case VT_DECIMAL:
	case VT_UI2:
	case VT_UI4:
	case VT_INT:
	case VT_UINT:
		{
			pProp = new CMFCPropertyGridProperty( common::str2wstr(valueName).c_str(), value, _T("") );

			_variant_t v1 = pProp->GetValue();
			ASSERT(v1.vt == value.vt);
		}
		break;

	case VT_I1:
	case VT_UI1:
		pProp = new CMFCPropertyGridProperty( common::str2wstr(valueName).c_str(), 
			(_variant_t)(int)0, _T("") );
		break;

	default:
		{
		}
		break;
	}
	RETV(!pProp, NULL);

	if (!SetPropertyValue(pProp, value ))
	{
		delete pProp;
		return NULL;
 	}

	if (IsNotPrint)
	{
		// 화면에 출력되지 않는 값은 메모리주소를 NULL로 설정해서
		// Refresh 되지 않게 한다.
 		pProp->AllowEdit(FALSE);
 		pProp->Enable(FALSE);
		AddProperty(pParentProp, pProp, &symbol, &STypeData(SymTagData, VT_EMPTY, NULL));
	}
	else
	{
 		AddProperty(pParentProp, pProp, &symbol, &STypeData(SymTagData, value.vt, symbol.mem.ptr));
	}

	return pProp;
}


//------------------------------------------------------------------------
// Property 데이타에 value 값을 설정한다. 
//------------------------------------------------------------------------
bool visualizer ::SetPropertyValue(CMFCPropertyGridProperty *pProp, _variant_t value)
{
	switch (value.vt)
	{
	case VT_I2:
	case VT_I4:
	case VT_R4:
	case VT_R8:
	case VT_BOOL:
	case VT_DECIMAL:
	case VT_UI2:
	case VT_UI4:
	case VT_INT:
	case VT_UINT:
	case VT_I1:
	case VT_UI1:
		{
			const size_t optionCnt = pProp->GetOptionCount();
			if (optionCnt > 0)
			{
				//ASSERT_RETV((int)value < optionCnt, false);
				if ((size_t)value < optionCnt)
				{
					_variant_t var = (_variant_t)pProp->GetOption( (int)value );
					pProp->SetValue(var);
				}
			}
			else
			{
				_variant_t var;
				var.ChangeType(pProp->GetValue().vt, &value);
				pProp->SetValue(var);
			}
		}
		break;

		//		{
		//			_variant_t tmp  = (VT_I1 == value.vt)? (int)value.cVal : (int)value.bVal;
		// 			_variant_t var;
		// 			var.ChangeType(pProp->GetValue().vt, &value);
		// 			pProp->SetValue(tmp);
		//  		}
		//  		break;

		// 	case VT_BSTR:
		// 		{
		// 			std::string str;
		// 			operator>>(str);
		// #ifdef _UNICODE
		// 			var.bstrVal = (_bstr_t)common::string2wstring(str).c_str();
		// #else
		// 			var.bstrVal = (_bstr_t)str.c_str();
		// #endif
		// 		}
		// 		break;

	default:
		{
			// Err!!, Property에 value 타입의 값을 넣을 수 없습니다. 
			ASSERT(0);
			return false;
		}
		break;
	}
	return true;
}


//------------------------------------------------------------------------
// Property추가
//------------------------------------------------------------------------
void visualizer ::AddProperty(
								 CMFCPropertyGridProperty *pParentProp, 
								 CMFCPropertyGridProperty *prop, 
								 const SSymbolInfo *pSymbol,
								 STypeData *typeData)
{
	RET(!prop);
	RET(!n_pProperty);

	n_pProperty->AddProperty(pParentProp, prop,  pSymbol, typeData);
}
