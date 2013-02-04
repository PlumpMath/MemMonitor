
#include "stdafx.h"
#include "DiaWrapper.h"
#include "dia2.h"

using namespace std;
using namespace dia;


CDiaWrapper::CDiaWrapper() :
	m_pDiaDataSource(NULL)
,	m_pDiaSession(NULL)
,	m_pGlobalSymbol(NULL)
,	m_dwMachineType(CV_CFL_80386)
{

}

CDiaWrapper::~CDiaWrapper()
{
	SAFE_RELEASE(m_pGlobalSymbol);
	SAFE_RELEASE(m_pDiaSession);
	SAFE_RELEASE(m_pDiaDataSource);

	CoUninitialize();
}


//------------------------------------------------------------------------
// DIA �ʱ�ȭ
// pdbFileName : PDB ���ϸ�
//------------------------------------------------------------------------
bool CDiaWrapper::Init(const string &pdbFileName)
{
//	wchar_t wszExt[MAX_PATH];
	string wszSearchPath = "SRV**\\\\symbols\\symbols"; // Alternate path to search for debug data

	HRESULT hr = CoInitialize(NULL);

	// Obtain access to the provider
	hr = CoCreateInstance(__uuidof(DiaSource),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IDiaDataSource),
		(void **) &m_pDiaDataSource);
	if (FAILED(hr)) 
	{
// 		wprintf(L"CoCreateInstance failed - HRESULT = %08X\n", hr);
 		return false;
 	}

	hr = m_pDiaDataSource->loadDataFromPdb( common::string2wstring(pdbFileName).c_str() );
	if (FAILED(hr)) 
		return false;

	// Open a session for querying symbols
	hr = m_pDiaDataSource->openSession(&m_pDiaSession);
	if (FAILED(hr)) {
//	    wprintf(L"openSession failed - HRESULT = %08X\n", hr);
		return false;
	}

	// Retrieve a reference to the global scope
	hr = m_pDiaSession->get_globalScope(&m_pGlobalSymbol);
	if (hr != S_OK) {
//		wprintf(L"get_globalScope failed\n");
		return false;
	}

	// Set Machine type for getting correct register names
	DWORD dwMachType = 0;
	if (m_pGlobalSymbol->get_machineType(&dwMachType) == S_OK) {
		switch (dwMachType) {
	  case IMAGE_FILE_MACHINE_I386 : m_dwMachineType = CV_CFL_80386; break;
	  case IMAGE_FILE_MACHINE_IA64 : m_dwMachineType = CV_CFL_IA64; break;
	  case IMAGE_FILE_MACHINE_AMD64 : m_dwMachineType = CV_CFL_AMD64; break;
		}
	}

	return true;
}


//------------------------------------------------------------------------
// 
//------------------------------------------------------------------------
IDiaSymbol* CDiaWrapper::FindType(const std::string &typeName)
{
	CComPtr<IDiaEnumSymbols> pEnumSymbols;
	if (FAILED(m_pGlobalSymbol->findChildren(SymTagNull, common::string2wstring(typeName).c_str(), 
		nsRegularExpression, &pEnumSymbols))) 
		return NULL;

	IDiaSymbol *pSymbol;
	ULONG celt = 0;
	// ù��°�� �߰ߵǴ� ������ ã�Ƽ� �����Ѵ�.
	while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) 
	{
		return pSymbol;
	}
	return NULL;
}


//------------------------------------------------------------------------
// pSymbol�� ����Ÿ ���̸� �����Ѵ�.
//------------------------------------------------------------------------
ULONGLONG CDiaWrapper::GetSymbolLength(IDiaSymbol *pSymbol)
{
	ULONGLONG len = 0;
	CComPtr<IDiaSymbol> psymType;
	if (pSymbol->get_type(&psymType) == S_OK)
	{
		HRESULT hr = psymType->get_length(&len);
		assert( hr == S_OK );
	}
	else
	{
		HRESULT hr = pSymbol->get_length(&len);
		assert( hr == S_OK );
	}
	return len;
}


//------------------------------------------------------------------------
// Ÿ���̸��� �����Ѵ�.
//------------------------------------------------------------------------
// std::string	CDiaWrapper::GetSymbolTypeName(IDiaSymbol *pSymbol)
// {
// 	std::string reval;
// 
// 	CComPtr<IDiaSymbol> pBaseType;
// 	if (pSymbol->get_type( &pBaseType ) == S_OK)
// 	{
// 		BSTR name;
// 		if (pBaseType->get_name(&name) == S_OK)
// 		{
// 			reval = common::wstring2string(name);	
// 			SysFreeString(name);
// 		}
// 	}
// 	if (reval.empty())
// 	{
// 		BSTR name;
// 		if (pSymbol->get_name(&name) == S_OK)
// 		{
// 			reval = common::wstring2string(name);	
// 			SysFreeString(name);
// 		}
// 	}
// 	return reval;
// }


//------------------------------------------------------------------------
// �ɺ��� Ÿ�������� �����Ѵ�.
// pSymbol �� ����Ÿ�� ����Ű��, Ÿ���� ����Ű�� 
// �� �ɺ��� �ش��ϴ� Ÿ���̸��� �����Ѵ�.
//------------------------------------------------------------------------
std::string	dia::GetSymbolTypeName(IDiaSymbol *pSymbol)
{
	HRESULT hr;
	std::string typeName;

 	enum SymTagEnum symtag;
 	hr =  pSymbol->get_symTag((DWORD*)&symtag);
	RETV(S_FALSE == hr, typeName);

	switch (symtag)
	{
	case SymTagBaseType:
		{
			BasicType btype;
			hr = pSymbol->get_baseType((DWORD*)&btype);
			RETV(S_FALSE == hr, typeName);

			ULONGLONG length;
			hr = pSymbol->get_length(&length);
			RETV(S_FALSE == hr, typeName);

			typeName = GetBasicTypeName(btype, length);
		}
		break;

	case SymTagEnum:
	case SymTagUDT:
		typeName = GetSymbolName(pSymbol);
		break;

	case SymTagData:
	case SymTagArrayType:
	case SymTagPointerType:
		{
			CComPtr<IDiaSymbol> pBaseType;
			hr = pSymbol->get_type(&pBaseType);
			RETV(S_FALSE == hr, typeName);

			typeName = GetSymbolTypeName(pBaseType);
		}
		break;

	default:
 		typeName = "NoType";
		break;
	}

 	if (SymTagArrayType == symtag)
 		typeName += " array";
	else if (SymTagPointerType == symtag)
		typeName += " *";

	return typeName;
}


//------------------------------------------------------------------------
// 
//------------------------------------------------------------------------
std::string dia::GetBasicTypeName(BasicType btype, ULONGLONG length)
{
	std::string typeName;
	switch (btype)
	{
	case btNoType:	typeName = "NoType"; break;
	case btVoid:	typeName = "void";  break;
	case btChar:	typeName = "char";  break;
	case btWChar:	typeName = "wchar";  break;
	case btInt:
		switch(length)
		{
		case 1: typeName = "char";  break;
		case 2: typeName = "short";  break;
		default: typeName = "int";  break;
		}
		break;

	case btUInt:
		switch(length)
		{
		case 1: typeName = "BYTE";  break;
		case 2: typeName = "u_short";  break;
		default: typeName = "u_int";  break;
		}
		break;

	case btFloat:	
		switch(length)
		{
		case 4: typeName = "float";  break;
		case 8: typeName = "double";  break;
		default: typeName = "float";  break;
		}
		break;
	case btBCD:		typeName = "bcd";  break;
	case btBool:	typeName = "bool";  break;
	case btLong:	typeName = "long";  break;
	case btULong:	typeName = "u_long";  break;
	case btCurrency:typeName = "currency";  break;
	case btDate:	typeName = "date";  break;
	case btVariant:	typeName = "variant";  break;
	case btComplex:	typeName = "complex";  break;
	case btBit:		typeName = "bit";  break;
	case btBSTR:	typeName = "bstr";  break;
	case btHresult:	typeName = "hresult";  break;
	default: typeName = "NoType";  break;
	}
	return typeName;
}


// pSymbol �� ����Ÿ�� ����Ű�� �ɺ��̾�� �Ѵ�.
_variant_t dia::GetValueFromAddress(void *srcPtr, const BasicType btype, const ULONGLONG length )
{
	_variant_t value;	
	switch (btype)
	{
	case btBool: value = *(bool*)srcPtr; break;
	case btChar: value = *(char*)srcPtr; break;
	case btInt:
		switch(length)
		{
		case 1: value = *(char*)srcPtr; break;
		case 2: value = *(short*)srcPtr; break;
		default: value = *(int*)srcPtr; break;
		}
		break;		

	case btUInt:
		switch(length)
		{
		case 1: value = *(unsigned char*)srcPtr; break;
		case 2: value = *(unsigned short*)srcPtr; break;
		default: value = *(unsigned int*)srcPtr; break;
		}
		break;

	case btLong: value = *(long*)srcPtr; break;
	case btULong: value = *(unsigned long*)srcPtr; break;
	case btFloat: 
		switch(length)
		{
		case 4: value = *(float*)srcPtr; break;
		case 8: value = *(double*)srcPtr; break;
		default: value = *(float*)srcPtr; break;
		}
		break;

	case btBSTR:
	default:
		{
		}
		break;
	}	
	return value;
}


//------------------------------------------------------------------------
// srcPtr �޸𸮿� �ִ� ����Ÿ�� varType ���·� ���� �����Ѵ�.
//------------------------------------------------------------------------
_variant_t dia::GetValue(void *srcPtr, VARTYPE varType)
{
	_variant_t value;
	switch (varType)
	{
	case VT_I1: value  = *(char*)srcPtr; break;
	case VT_I2: value  = *(short*)srcPtr; break;
	case VT_I4: value  = *(long*)srcPtr ; break;
	case VT_R4: value  = *(float*)srcPtr; break;
	case VT_R8: value  = *(double*)srcPtr; break;
	case VT_BOOL: value  = *(bool*)srcPtr; break;
	case VT_DECIMAL: break;
	case VT_UI1: value  = *(unsigned char*)srcPtr; break;
	case VT_UI2: value  = *(unsigned short*)srcPtr; break;
	case VT_UI4: value  = *(unsigned long*)srcPtr; break;
	case VT_INT: value  = *(int*)srcPtr; break;
	case VT_UINT: value  = *(unsigned int*)srcPtr; break;
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
		break;
	}
	assert( value.vt == varType);
	return value;
}


//------------------------------------------------------------------------
// destPtr �� value�� �����Ѵ�.
//------------------------------------------------------------------------
void	dia::SetValue(void *destPtr, _variant_t value)
{
	switch (value.vt)
	{
	case VT_I2: *(short*)destPtr = value.iVal; break;
	case VT_I4: *(long*)destPtr = value.lVal; break;
	case VT_R4: *(float*)destPtr = value.fltVal; break;
	case VT_R8: *(double*)destPtr = value.dblVal; break;
	case VT_BOOL: *(bool*)destPtr = value.boolVal? true : false; break;
	case VT_DECIMAL: break;
	case VT_UI2: *(unsigned short*)destPtr = value.uiVal; break;
	case VT_UI4: *(unsigned int*)destPtr = value.uintVal; break;
	case VT_INT: *(int*)destPtr = value.intVal; break;
	case VT_UINT: *(unsigned int*)destPtr = value.uintVal; break;
	case VT_I1: *(char*)destPtr = value.cVal; break;
	case VT_UI1: *(unsigned char*)destPtr = value.bVal; break;
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
		break;
	}
}


//------------------------------------------------------------------------
// �ɺ� �̸��� �����Ѵ�.
//------------------------------------------------------------------------
std::string dia::GetSymbolName(IDiaSymbol *pSymbol)
{
	BSTR bstrName;
	BSTR bstrUndName;

	if (pSymbol->get_name(&bstrName) == S_FALSE)
		return "";

	std::string name;
	if (pSymbol->get_undecoratedName(&bstrUndName) == S_OK) {
		if (wcscmp(bstrName, bstrUndName) == 0) {
			name = common::wstring2string(bstrName);
		}
		else {
			name = common::wstring2string(bstrName) +
				"(" + common::wstring2string(bstrName) + ")";
		}

		SysFreeString(bstrUndName);
	}
	else {
		name = common::wstring2string(bstrName);
	}

	SysFreeString(bstrName);
	return name;
}


//------------------------------------------------------------------------
// offset ���� �����Ѵ�.
// SymTagData Ÿ���� �ɺ��� ����� �۵��Ѵ�.
//------------------------------------------------------------------------
LONG dia::GetSymbolLocation(IDiaSymbol *pSymbol, OUT LocationType *pLocType) // pLocType=NULL
{
	// 	DWORD dwRVA, dwSect, dwOff, dwReg, dwBitPos, dwSlot;
	LONG lOffset = 0;
	ULONGLONG ulLen = 0;
	VARIANT vt = { VT_EMPTY };

	if (pLocType)
		*pLocType = LocIsNull;

	// Ÿ�Կ� ���� ������ �� �ִ�. ������� BaseClass Type
	LocationType locType;
	HRESULT hr = pSymbol->get_locationType((DWORD*)&locType); 

	switch (locType) 
	{
	case LocIsStatic:
		// 		if ((pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK) &&
		// 			(pSymbol->get_addressSection(&dwSect) == S_OK) &&
		// 			(pSymbol->get_addressOffset(&dwOff) == S_OK)) {
		// 				wprintf(L"%s, [%08X][%04X:%08X]", SafeDRef(rgLocationTypeString, dwLocType), dwRVA, dwSect, dwOff);
		// 		}
		break;

	case LocIsTLS:
	case LocInMetaData:
	case LocIsIlRel:
		// 		if ((pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK) &&
		// 			(pSymbol->get_addressSection(&dwSect) == S_OK) &&
		// 			(pSymbol->get_addressOffset(&dwOff) == S_OK)) {
		// 				wprintf(L"%s, [%08X][%04X:%08X]", SafeDRef(rgLocationTypeString, dwLocType), dwRVA, dwSect, dwOff);
		// 		}
		break;

	case LocIsRegRel:
		// 		if ((pSymbol->get_registerId(&dwReg) == S_OK) &&
		// 			(pSymbol->get_offset(&lOffset) == S_OK)) {
		// 				wprintf(L"%s Relative, [%08X]", SzNameC7Reg((USHORT) dwReg), lOffset);
		// 		}
		break;

	case LocIsThisRel:
		if (pSymbol->get_offset(&lOffset) == S_OK) {
//			info.offset = lOffset;

// 			IDiaSymbol *psymType;
// 			if (pSymbol->get_type(&psymType) == S_OK)
// 			{
// 				if (psymType->get_length(&ulLen) == S_OK)
// 				{
// 					info.length = ulLen;
// 				}
// 				psymType->Release();
// 			}
		}
		break;

	case LocIsBitField:
		// 		if ((pSymbol->get_offset(&lOffset) == S_OK) &&
		// 			(pSymbol->get_bitPosition(&dwBitPos) == S_OK) &&
		// 			(pSymbol->get_length(&ulLen) == S_OK)) {
		// 				wprintf(L"this(bf)+0x%X:0x%X len(0x%X)", lOffset, dwBitPos, ulLen);
		// 		}
		break;

	case LocIsEnregistered:
		// 		if (pSymbol->get_registerId(&dwReg) == S_OK) {
		// 			wprintf(L"enregistered %s", SzNameC7Reg((USHORT) dwReg));
		// 		}
		break;

	case LocIsNull:
		//		wprintf(L"pure");
		break;

	case LocIsSlot:
		// 		if (pSymbol->get_slot(&dwSlot) == S_OK) {
		// 			wprintf(L"%s, [%08X]", SafeDRef(rgLocationTypeString, dwLocType), dwSlot);
		//		}
		break;

	case LocIsConstant:
		//		wprintf(L"constant");

		if (pSymbol->get_value(&vt) == S_OK) {
			//			PrintVariant(vt);
		}
		break;

	default :
		//		wprintf(L"Error - invalid location type: 0x%X", dwLocType);
		return false;
		break;
	}

	if (pLocType)
		*pLocType = locType;

	return lOffset;
}



//------------------------------------------------------------------------
// BasicType�� �����Ѵ�.
//------------------------------------------------------------------------
// BasicType dia::GetSymbolBasicType(IDiaSymbol *pSymbol)
// {
// 	BasicType reval = btNoType;
// 	HRESULT hr;
// 	CComPtr<IDiaSymbol> pBaseType;
// 	if (pSymbol->get_type( &pBaseType ) == S_OK)
// 	{
// 		hr = pBaseType->get_baseType((DWORD *)&reval);
// 	}
// 	return reval;
// }


//------------------------------------------------------------------------
// pSymbol Ÿ���� ������ pMemPtr �ּҿ��� �����´�.
// pSymbol �� SymTagData Ÿ���̾�� �Ѵ�.
// isApplyOffset : false �̸� ������ offset�� �������� �ʴ´�.
//                         �̹� ���� ���¶�� ���ʿ� ����
// srcPtr : ����Ÿ�� ����� �ּ��ε�, pSymbol �� offset ��
//    ����Ǳ� ������ �� srcPtr�ּҴ� pSymbol�� �����ϰ� 
//    �ִ� ����ü��, Ŭ������ �ּҸ� ����Ű�� �־�� �Ѵ�.
//------------------------------------------------------------------------
_variant_t dia::GetValueFromSymbol(void *srcPtr, IDiaSymbol *pSymbol, bool isApplyOffset) // isApplyOffset=true
{
	_variant_t value;

	LONG offset = dia::GetSymbolLocation(pSymbol);
	if (!isApplyOffset) offset = 0;
	void *ptr = (BYTE*)srcPtr + offset;

	CComPtr<IDiaSymbol> pBaseType;
	HRESULT hr = pSymbol->get_type(&pBaseType);
	ASSERT_RETV((S_OK == hr), value);

	enum SymTagEnum baseSymTag;
	hr = pBaseType->get_symTag((DWORD*)&baseSymTag);
	ASSERT_RETV(S_OK==hr, value);

	BasicType btype;
	switch (baseSymTag)
	{
	case SymTagBaseType:
		hr = pBaseType->get_baseType((DWORD*)&btype);
		ASSERT_RETV((S_OK == hr), value );
		break;

	case SymTagPointerType:
		btype = btULong;
		break;

	default:
		return value;
	}

	ULONGLONG length = 0;
	hr = pBaseType->get_length(&length);
	ASSERT_RETV((S_OK == hr), value );

	value = dia::GetValueFromAddress(ptr, btype, length);
	return value;
}


//------------------------------------------------------------------------
// pParentSymbol ���� �ڽ��߿� symbolName �� �ɺ��� �����Ѵ�.
// ã�� �ɺ��� Offset���� �����Ѵ�.
// pParentSymbol �� Data, UDT, TypeDef Ÿ���̾�� �Ѵ�.
//------------------------------------------------------------------------
IDiaSymbol* dia::FindChildSymbol( const std::string &symbolName, 
								 IDiaSymbol *pParentSymbol, OUT LONG *pOffset ) // pOffset =NULL
{
	RETV( !pParentSymbol, NULL );

	const string name = GetSymbolName(pParentSymbol); //debug��
	const wstring searchSymbolName = common::string2wstring(symbolName).c_str();

	// ����Ÿ �ɺ��̸�, Ÿ�� �ɺ��� ��ü�Ѵ�.
	enum SymTagEnum symTag;
	HRESULT hr = pParentSymbol->get_symTag((DWORD*)&symTag);

	bool isNewTypeSymbol = false;
	IDiaSymbol* pTypeSymbol = NULL;
	switch (symTag)
	{
	case SymTagData:
		isNewTypeSymbol = true;
		hr = pParentSymbol->get_type(&pTypeSymbol);
		break;
	case SymTagTypedef:
		{
			hr = pParentSymbol->get_type(&pTypeSymbol);
			IDiaSymbol *reval = FindChildSymbol(symbolName, pTypeSymbol, pOffset);
			if (pTypeSymbol)
				pTypeSymbol->Release();
			return reval;
		}
		break;
	default:
		{
			pTypeSymbol = pParentSymbol;
		}
		break;
	}

	// Enumeration
	CComPtr<IDiaEnumSymbols> pEnumSymbols;
	if (FAILED(pTypeSymbol->findChildren(SymTagNull, searchSymbolName.c_str(), 
		nsRegularExpression, &pEnumSymbols)))
	{
		if (isNewTypeSymbol)
			pTypeSymbol->Release();
		return NULL;
	}

	IDiaSymbol *pFindSymbol;
	ULONG celt = 0;
	pEnumSymbols->Next(1, &pFindSymbol, &celt);
	// ã�Ҵٸ� ���� 
	if (1 == celt)
	{
		if (isNewTypeSymbol)
			pTypeSymbol->Release();
		if (pOffset)
			hr = pFindSymbol->get_offset(pOffset);
		return pFindSymbol;
	}

	// ��ã�Ҵٸ�, �ڽ� �߿��� ã�ƺ���. enum  ���� ����� ���� �̷��� ã�ƾ���
	CComPtr<IDiaEnumSymbols> pAnotherEnumSymbols;
	if (FAILED(pTypeSymbol->findChildren(SymTagNull, NULL, nsNone, &pAnotherEnumSymbols)))
	{
		if (isNewTypeSymbol)
			pTypeSymbol->Release();
		return NULL;
	}

	pFindSymbol = NULL;
	IDiaSymbol* pChildSymbol;
	celt = 0;
	while (SUCCEEDED(pAnotherEnumSymbols->Next(1, &pChildSymbol, &celt)) && (celt == 1)) 
	{
		enum SymTagEnum symTag;
		pChildSymbol->get_symTag((DWORD*)&symTag);

		if (SymTagBaseClass == symTag)
		{
			LONG childOffset = 0;
			pFindSymbol = FindChildSymbol( symbolName, pChildSymbol, &childOffset );
			if (pFindSymbol)
			{
				LONG baseClassOffset = 0;
				hr = pChildSymbol->get_offset(&baseClassOffset);
				if (pOffset)
					*pOffset = baseClassOffset + childOffset;
				break;
			}
		}

		if (SymTagEnum != symTag) 
		{ 
			pChildSymbol->Release(); 
			continue; 
		}

		CComPtr<IDiaEnumSymbols> pSubEnumSymbols;
		if (FAILED(pChildSymbol->findChildren(SymTagNull, searchSymbolName.c_str(), 
			nsRegularExpression, &pSubEnumSymbols)))
		{
			pChildSymbol->Release(); 
			continue; 
		}

		celt = 0;
		pSubEnumSymbols->Next( 1, &pFindSymbol, &celt );
		if (1 == celt)
		{
			pChildSymbol->Release();
			const string name = GetSymbolName(pFindSymbol); //debug��
			if (pOffset)
				*pOffset = 0; // ������� offset�� ����.
			break; // ã������ ����
		}
	}

	if (isNewTypeSymbol)
		pTypeSymbol->Release();
	return pFindSymbol;
}